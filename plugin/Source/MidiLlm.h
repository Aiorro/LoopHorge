#pragma once

#include "engine/ModelNotes.h"
#include "ProjectFiles.h"
#include "Wire.h"
#include <mutex>

namespace midiLlm {
inline std::mutex inferenceMutex;
inline juce::File directory() {
#if JUCE_WINDOWS
 return juce::File::getSpecialLocation(juce::File::windowsLocalAppData).getChildFile("LoopForge/MIDI-LLM");
#else
 return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LoopForge/MIDI-LLM");
#endif
}

inline juce::File setupLauncher() {
 auto location = juce::File::getSpecialLocation(juce::File::currentApplicationFile).getParentDirectory();
 for (int parent=0; parent<5; ++parent) {
#if JUCE_WINDOWS
  auto launch = location.getChildFile("MIDI-LLM/Setup MIDI-LLM.cmd");
#else
  auto launch = location.getChildFile("MIDI-LLM/setup-midi-llm.py");
#endif
  if (launch.existsAsFile()) return launch;
  location = location.getParentDirectory();
 }
 return {};
}

inline bool installed() { return directory().getChildFile("runtime.json").existsAsFile(); }

template<class Continue>
composer::model::Result run(const std::string& direction, const composer::ResolvedConfig& config, Continue keepGoing,
                            const juce::File& root=directory(), int timeoutMs=300000) {
 const auto settings = projectFiles::read(root.getChildFile("runtime.json"), 16384);
 if (!settings.error.isEmpty()) return {{}, "Set up MIDI-LLM before generating"};
 const auto manifest = wire::document(settings.text);
 if (!wire::integer(manifest["version"],1,1)) return {{}, "MIDI-LLM runtime settings are invalid"};
 const auto python = manifest["python"].toString();
 const auto device = manifest["device"].toString();
 if(device!="cpu"&&device!="cu126")return {{}, "MIDI-LLM device setting is invalid"};
 const auto worker = root.getChildFile("worker.py");
 const auto revision = manifest["revision"].toString();
 if (revision.length()!=40 || !revision.containsOnly("0123456789abcdef"))
  return {{}, "MIDI-LLM revision is invalid"};
 const auto model = root.getChildFile("models").getChildFile(revision);
 if (!juce::File::isAbsolutePath(python) || !juce::File(python).existsAsFile() ||
     !worker.existsAsFile() || !model.getChildFile("config.json").existsAsFile() ||
     !model.getChildFile("model.safetensors").existsAsFile())
  return {{}, "MIDI-LLM setup is incomplete. Run Setup MIDI-LLM."};

 // Reject unsupported lengths before a model load. Input is data in a private
 // per-request path. Prompt content never forms part of a shell command.
 const auto& request = config.config;
 if (request.mode!=composer::RequestMode::loop || request.duration.unit!=composer::DurationUnit::bars ||
     (request.duration.amount!=4 && request.duration.amount!=8 && request.duration.amount!=16))
  return {{}, "MIDI-LLM supports 4, 8 or 16 bar loops"};
 juce::TemporaryFile temporary(root.getChildFile("request.json"), juce::TemporaryFile::useHiddenFile);
 const auto input = temporary.getFile();
 juce::TemporaryFile output(root.getChildFile("result.json"), juce::TemporaryFile::useHiddenFile);
 juce::var data(new juce::DynamicObject);
 auto* object = data.getDynamicObject();
 object->setProperty("version",1);
 object->setProperty("device",device);
 object->setProperty("direction",juce::String::fromUTF8(direction.data(),int(direction.size())));
 object->setProperty("seed",juce::int64(request.recipe.seed));
 object->setProperty("bars",int(request.duration.amount));
 object->setProperty("bpm",request.recipe.bpm);
 object->setProperty("key",juce::String(composer::fieldValue(request,composer::Field::root)+" "+composer::fieldValue(request,composer::Field::scale)));
 object->setProperty("meter",juce::String(composer::fieldValue(request,composer::Field::meter)));
 juce::StringArray hints;
 for (const auto field : {composer::Field::style,composer::Field::instruments,composer::Field::darkness,composer::Field::energy,
                          composer::Field::melancholy,composer::Field::tension,composer::Field::aggression,
                          composer::Field::atmosphere,composer::Field::complexity}) {
  if (config.origins[size_t(field)]!=composer::Origin::defaultValue)
   hints.add(juce::String(composer::fieldValue(request,field)));
 }
 object->setProperty("hints",hints.joinIntoString(", "));
 if (!input.replaceWithText(juce::JSON::toString(data,true))) return {{}, "Could not prepare MIDI-LLM request"};
 if (!keepGoing()) return {{}, "Generation cancelled"};
 const std::unique_lock guard(inferenceMutex,std::try_to_lock);
 if(!guard.owns_lock())return {{}, "MIDI-LLM is already generating"};
 juce::InterProcessLock lock("LoopForge MIDI-LLM "+juce::String(root.getFullPathName().hashCode64()));
 if(!lock.enter(0))return {{}, "MIDI-LLM is already generating in another LoopForge instance"};
 struct Unlock { juce::InterProcessLock& lock; ~Unlock(){lock.exit();} } unlock{lock};
 juce::ChildProcess process;
 if (!process.start(juce::StringArray{python,"-I",worker.getFullPathName(),"generate",model.getFullPathName(),
                     input.getFullPathName(),output.getFile().getFullPathName()}, 0))
  return {{}, "Could not start MIDI-LLM"};
 const auto deadline=juce::Time::getMillisecondCounterHiRes()+timeoutMs;
 while (!process.waitForProcessToFinish(40)) {
  if (!keepGoing() || juce::Time::getMillisecondCounterHiRes()>=deadline) {
   process.kill();
   process.waitForProcessToFinish(1000);
   return {{}, "MIDI-LLM generation cancelled or timed out"};
  }
 }
 if (!keepGoing()) return {{}, "Generation cancelled"};
 if (process.getExitCode()!=0) return {{}, "MIDI-LLM failed to generate. Check model setup and available memory."};
 const auto reply = projectFiles::read(output.getFile(), 32768);
 if (!reply.error.isEmpty()) return {{}, "MIDI-LLM response is missing or too large"};
 const auto parsed = wire::document(reply.text);
 const auto tokens = parsed["tokens"];
 if (!wire::integer(parsed["version"],1,1) || !tokens.isArray() || tokens.size()>2046)
  return {{}, "MIDI-LLM returned an invalid response"};
 std::vector<int> events;
 events.reserve(size_t(tokens.size()));
 for (const auto& token : *tokens.getArray()) {
  if (!wire::integer(token,composer::model::textVocab,composer::model::midiBos-1))
   return {{}, "MIDI-LLM returned an invalid token"};
  events.push_back(int(token));
 }
 return composer::model::decode(events,config);
}
}
