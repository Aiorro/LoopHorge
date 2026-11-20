#include "../Source/Processor.h"
#include "../Source/engine/Midi.h"
#include "../Source/engine/ModelNotes.h"
#include <iostream>
#include <thread>
using namespace composer;
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
std::string saved(Processor& p){juce::MemoryBlock b;p.getStateInformation(b);return {static_cast<const char*>(b.getData()),b.getSize()};}
void idle(Processor& p){for(int i=0;i<1000&&p.busy;++i)juce::Thread::sleep(5);check(!p.busy);}
const juce::String direction="4 bars C minor ambient bass chords lead drums";
const std::string legacy=R"({"version":1,"extension":{"precise":1.234567890123e-2},"idea":{"metadata":{"bars":4,"bpm":120},"layers":{"bass":[{"pitch":38,"start":0,"duration":4,"velocity":90}],"chords":[],"melody":[]}}})";
void modelTokens(){
 const auto config=LocalPromptInterpreter{}.interpret("4 bars D minor 120 bpm",17);
 const int v=model::textVocab;
 const std::array notes{v,v+10050,v+11060,
                        v+50,v+10010,v+11000+128*128+36,
                        v+799,v+10099,v+11000+128*32+48,
                        v+800,v+10099,v+11061};
 auto result=model::decode(notes,config);
 check(result.composition&&result.composition->source=="midi-llm-llama-3.2-1b");
 const auto& song=*result.composition;
 check(song.endTick.value==7680&&song.tracks.size()==3);
 check(song.tracks[0].program==0&&song.tracks[0].notes[0].duration.value==480);
 check(song.tracks[1].channel==10&&song.tracks[1].notes[0].pitch==36);
 check(song.tracks[2].program==32&&song.tracks[2].notes[0].start.value+song.tracks[2].notes[0].duration.value==7680);
 check(encodeCompositionMidi(song));
 check(!model::decode(std::span(notes).first(5),config).composition);
 auto invalid=notes;invalid[2]=model::midiBos;
 check(!model::decode(invalid,config).composition);
 const auto longRequest=LocalPromptInterpreter{}.interpret("32 bars",17);
 check(!model::decode(notes,longRequest).composition);
}
int main(int argc,char** argv){
 modelTokens();
 juce::ScopedJuceInitialiser_GUI init;
 if(argc==2){
  const auto config=LocalPromptInterpreter{}.interpret("4 bars D minor 120 bpm",17);
  const auto root=juce::File(argv[1]);
  const auto result=midiLlm::run("Jazz \"guitar\" and bass",config,[]{return true;},root,3000);
  if(!result.composition)std::cerr<<result.error<<"\n";
  check(result.composition&&result.composition->tracks.size()==2&&result.composition->endTick.value==7680);
  check(!midiLlm::run("Cancelled",config,[]{return false;},root,3000).composition);
  check(!midiLlm::run("Slow",config,[]{return true;},root,200).composition);
  std::cout<<"MIDI-LLM worker round-trip and cancellation passed\n";return 0;
 }
auto owner=std::make_unique<Processor>();auto& p=*owner;
 check(saved(p)=="null"&&p.ideaCount()==0&&!p.canSwitchMode());
 {auto pinnedStorage=std::make_unique<Processor>();auto& pinned=*pinnedStorage;ManualOverrides manual;manual.pinned.set(size_t(Field::energy));manual.values.mood.energy=0;
  const auto expected=generateResolvedLoop(LocalPromptInterpreter{}.interpret("4 bars energy100",42,manual));check(expected.composition);
  check(pinned.requestComposer("4 bars energy100",42,manual));manual.values.mood.energy=100;idle(pinned);
  auto actual=decodeProjectJson(saved(pinned));check(actual.project&&encodeCompositionJson(*actual.project->composition).json==encodeCompositionJson(*expected.composition).json);
  check(pinned.composerSummary().contains("energy=0 [manual]"));
  auto prior=saved(pinned);manual.values.mood.energy=101;check(pinned.requestComposer("4 bars",42,manual));idle(pinned);check(saved(pinned)==prior);
  check(pinned.requestComposer("4 bars energy100",42));idle(pinned);check(pinned.composerSummary().contains("energy=100 [text]"));
 }


 {auto capStorage=std::make_unique<Processor>();auto& cap=*capStorage;Config config;config.bars=16;auto loop=generateLoop(config);check(loop.composition);
  ProjectState old;old.activeMode=ProjectMode::composer;old.composition=std::move(loop.composition);old.composition->endTick={61440};old.composition->tracks[0].notes.push_back({999999,36,90,{60000},{480}});
  auto encoded=encodeProjectJson(old);check(encoded&&cap.restoreStateInformation(encoded.json.data(),int(encoded.json.size())));const auto original=saved(cap);
  auto midi=encodeCompositionMidi(*decodeProjectJson(original).project->composition);check(midi);juce::MemoryInputStream input(midi.bytes.data(),midi.bytes.size(),false);juce::MidiFile file;check(file.readFrom(input));for(int i=0;i<file.getNumTracks();++i)check(file.getTrack(i)->getEndTime()==61440);
  for(const auto text:{"32 bars","17 bars","100 bars"}){check(cap.requestComposer(text,42));idle(cap);check(saved(cap)==original);check(cap.getStatus().contains("16"));}
  ManualOverrides manual;manual.pinned.set(size_t(Field::duration));manual.values.duration={DurationUnit::bars,32};check(cap.requestComposer("4 bars",42,manual));idle(cap);check(saved(cap)==original);
  auto restored=decodeProjectJson(saved(cap));check(restored.project->composition->endTick.value==61440&&restored.project->composition->tracks[0].notes.back().start.value==60000);
  check(cap.requestComposer("16 bars",42));idle(cap);check(decodeProjectJson(saved(cap)).project->composition->endTick.value==30720);
 }
 check(p.requestComposer(direction,42));idle(p);check(p.isComposerMode());
 auto first=decodeProjectJson(saved(p));check(first.project&&first.project->composition->tracks.size()==4&&first.project->legacyStateJson);
 auto demo=*first.project->legacyStateJson;auto raw=wire::document(demo);Schedule checked;check(wire::schedule(raw["idea"],checked));
 check(raw["idea"]["layers"]["bass"].size()==16&&raw["idea"]["layers"]["chords"].size()==12);
 auto json=encodeCompositionJson(*first.project->composition);check(json);
 check(p.composerSummary().contains("seed")&&p.composerSummary().contains("text"));
 check(p.requestComposer(direction,42));idle(p);auto same=decodeProjectJson(saved(p));check(encodeCompositionJson(*same.project->composition).json==json.json);
 check(p.requestComposer(direction,43));idle(p);check(encodeCompositionJson(*decodeProjectJson(saved(p)).project->composition).json!=json.json);
 for(auto text:{juce::String("3 minutes song"),juce::String("4 bars 8 bars"),juce::String("4 bars pads")}){
  auto before=saved(p);check(p.requestComposer(text,42));idle(p);std::cerr<<text<<": "<<p.getStatus()<<"\n";check(saved(p)==before&&!p.getStatus().contains("ready"));
 }
 auto before=saved(p);check(!p.requestComposer(juce::String::repeatedString("a",4097),42));check(saved(p)==before&&!p.busy);
 check(p.selectMode(ProjectMode::legacy));check(!p.isComposerMode()&&p.getIdea().isObject());check(*decodeProjectJson(saved(p)).project->legacyStateJson==demo);
 check(p.selectMode(ProjectMode::composer));check(p.isComposerMode());
 check(p.restoreStateInformation(legacy.data(),int(legacy.size())));check(!p.canSwitchMode());
 check(p.requestComposer(juce::String::fromUTF8("4 такта до минор бас аккорды мелодия ударные"),42));idle(p);auto ru=decodeProjectJson(saved(p));check(ru.project&&ru.project->composition->tracks.size()==4&&*ru.project->legacyStateJson==legacy);
 ru.project->hostStartQuarterBeat=17.25;auto origin=encodeProjectJson(*ru.project);check(origin);check(p.restoreStateInformation(origin.json.data(),int(origin.json.size())));
 check(p.selectMode(ProjectMode::legacy));check(p.selectMode(ProjectMode::composer));check(decodeProjectJson(saved(p)).project->hostStartQuarterBeat==17.25);
 check(p.requestComposer(direction,44));idle(p);check(decodeProjectJson(saved(p)).project->hostStartQuarterBeat==17.25);
 p.prepareToPlay(48000,64);p.play=true;juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.ensureSize(32768);p.processBlock(audio,midi);check(!midi.isEmpty()&&audio.getMagnitude(0,64)>0);
 int cancelled=0;for(int i=0;i<40;++i){auto prior=saved(p);check(p.requestComposer("16 bars ambient bass chords lead drums",uint32_t(i)));if(p.cancel()){++cancelled;idle(p);check(saved(p)==prior);}else idle(p);}
 check(cancelled>0);
 for(int i=0;i<20;++i){check(p.requestComposer("16 bars ambient bass chords lead drums",uint32_t(i)));check(p.restoreStateInformation(legacy.data(),int(legacy.size())));idle(p);check(saved(p)==legacy);}
 check(p.requestComposer(direction,42));idle(p);check(p.selectMode(ProjectMode::legacy));
 for(int i=0;i<20;++i){check(p.requestComposer("16 bars ambient bass chords lead drums",uint32_t(i)));auto snapshot=p.snapshot();check(p.edit(snapshot.revision,"bass",0,39+i%2,0,4));auto edited=saved(p);idle(p);check(saved(p)==edited);}
 check(p.requestComposer(direction,42));idle(p);for(int i=0;i<20;++i){check(p.requestComposer("16 bars ambient bass chords lead drums",uint32_t(i)));check(p.selectMode(ProjectMode::legacy));auto selected=saved(p);idle(p);check(saved(p)==selected);check(p.selectMode(ProjectMode::composer));}
 std::cout<<"Native direction/seed, preservation, mode, MIDI, cancellation and stale-result checks passed\n";
}
