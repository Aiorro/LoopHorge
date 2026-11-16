#pragma once
#include "engine/ProjectJson.h"
#include "engine/Midi.h"
#include <juce_core/juce_core.h>
#include <mutex>

// One editor-owned snapshot job; no processor, component or callback crosses the worker boundary.
class ComposerMidiExport final:private juce::Thread {
public:
 ComposerMidiExport():Thread("LoopForge MIDI export"),started(startThread()){}
 ~ComposerMidiExport()override{signalThreadShouldExit();notify();stopThread(-1);}
 bool request(std::string project){
  std::lock_guard lock(mutex);
  if(!started||state!=State::idle||project.empty()||project.size()>composer::jsonDetail::maximumBytes)return false;
  input=std::move(project);state=State::queued;notify();return true;
 }
 bool busy()const{std::lock_guard lock(mutex);return state!=State::idle;}
 std::optional<composer::MidiResult> takeResult(){
  std::lock_guard lock(mutex);if(state!=State::ready)return {};
  auto result=std::move(completed);completed.reset();state=State::idle;return result;
 }
private:
 enum class State{idle,queued,working,ready};
 mutable std::mutex mutex;State state=State::idle;std::string input;std::optional<composer::MidiResult> completed;const bool started;
 composer::MidiResult prepare(const std::string& project){
  auto decoded=composer::decodeProjectJson(project);
  if(!decoded.project)return {{},decoded.error};
  if(decoded.project->activeMode!=composer::ProjectMode::composer||!decoded.project->composition)return {{},"MIDI export requires a Composer project"};
  if(threadShouldExit())return {{},"MIDI export cancelled"};
  return composer::encodeCompositionMidi(*decoded.project->composition);
 }
 void run()override{
  while(!threadShouldExit()){
   wait(-1);if(threadShouldExit())return;std::string project;
   {std::lock_guard lock(mutex);if(state!=State::queued)continue;project.swap(input);state=State::working;}
   composer::MidiResult result;
   try{result=prepare(project);}catch(const std::bad_alloc&){result.error="Insufficient memory for MIDI export";}
   catch(const std::exception&){result.error="MIDI export failed";}
   // The bounded encoder cannot be interrupted mid-track; destruction joins it before releasing storage.
   std::lock_guard lock(mutex);if(threadShouldExit())return;completed=std::move(result);state=State::ready;
  }
 }
};
