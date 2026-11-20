#include "../Source/Processor.h"
#include "../Source/engine/ProjectJson.h"
#include "AllocationGuard.h"
#include <thread>
#include <iostream>
using namespace composer;
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
#if LOOPFORGE_WRAP_HEAP
extern "C" void* __real_malloc(size_t);extern "C" void* __real_calloc(size_t,size_t);extern "C" void* __real_realloc(void*,size_t);extern "C" void __real_free(void*);
extern "C" void* __wrap_malloc(size_t n){if(allocationGuard::active)++allocationGuard::allocations;return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t w){if(allocationGuard::active)++allocationGuard::allocations;return __real_calloc(n,w);}
extern "C" void* __wrap_realloc(void* p,size_t n){if(allocationGuard::active)++allocationGuard::allocations;return __real_realloc(p,n);}
extern "C" void __wrap_free(void* p){if(p&&allocationGuard::active)++allocationGuard::frees;__real_free(p);}
#endif
struct HostClock:juce::AudioPlayHead{double beat=0,bpm=120;bool playing=true;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(bpm);p.setPpqPosition(beat);p.setIsPlaying(playing);return p;}};
const std::string legacy=R"({"version":1,"extension":{"precise":1.234567890123e-2},"idea":{"metadata":{"bars":4,"bpm":120},"layers":{"bass":[{"pitch":38,"start":0,"duration":4,"velocity":90}],"chords":[],"melody":[]}}})";
Composition fixture(int pitch=60){Composition c;c.id="processor";c.endTick={1920};c.tempos={{{0},500000}};c.meters={{{0},4,4}};c.keys={{{0},0,Scale::major}};Track t;t.id=1;t.name="Track";t.channel=16;t.program=33;t.bank=Bank{2,7};t.notes={{2,pitch,90,{0},{960}},{3,pitch,100,{240},{960}}};t.controls={{4,{0},1,70},{5,{1920},64,0}};c.tracks={t};return c;}
std::string project(bool mode=true,double origin=8,int pitch=60){ProjectState p;p.activeMode=mode?ProjectMode::composer:ProjectMode::legacy;p.hostStartQuarterBeat=origin;p.legacyStateJson=legacy;p.composition=fixture(pitch);auto r=encodeProjectJson(p);check(r);return r.json;}
void restore(Processor& p,const std::string& text){p.setStateInformation(text.data(),int(text.size()));}
std::string saved(Processor& p){juce::MemoryBlock b;p.getStateInformation(b);return {static_cast<const char*>(b.getData()),b.getSize()};}
void process(Processor& p,juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi){allocationGuard::allocations=allocationGuard::frees=0;allocationGuard::active=true;p.processBlock(audio,midi);allocationGuard::active=false;check(allocationGuard::allocations==0&&allocationGuard::frees==0);check(midi.getNumEvents()<=2048&&midi.data.size()<=18432);}
void bypass(Processor& p,juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi){
 allocationGuard::allocations=allocationGuard::frees=0;
 allocationGuard::active=true;
 p.processBlockBypassed(audio,midi);
 allocationGuard::active=false;
 check(allocationGuard::allocations==0&&allocationGuard::frees==0);
}
int count(const juce::MidiBuffer& midi,bool on,int pitch=-1){int n=0;for(auto e:midi){auto m=e.getMessage();n+=(on?m.isNoteOn():m.isNoteOff())&&(pitch<0||pitch==m.getNoteNumber());}return n;}
void waitIdle(Processor& p){for(int i=0;i<500&&p.busy;++i)juce::Thread::sleep(10);check(!p.busy);}
void waitFile(const juce::File& f){for(int i=0;i<500&&!f.existsAsFile();++i)juce::Thread::sleep(10);check(f.existsAsFile());}
void workerCases(const juce::File& root){
 auto owner=std::make_unique<Processor>(5000);auto& p=*owner;restore(p,legacy);const auto v2=project();
 p.request("{\"provider\":\"ollama\"}");waitFile(root.getChildFile("request1"));restore(p,v2);check(saved(p)==v2);check(root.getChildFile("release1").replaceWithText("ready"));waitIdle(p);check(saved(p)==v2);
 restore(p,legacy);p.request("{\"provider\":\"ollama\"}");waitFile(root.getChildFile("request2"));auto snap=p.snapshot();check(p.edit(snap.revision,"bass",0,39,0,4));const auto edited=saved(p);check(root.getChildFile("release2").replaceWithText("ready"));waitIdle(p);check(saved(p)==edited&&p.getStatus().contains("cancelled"));
 p.request("{\"provider\":\"ollama\"}");waitFile(root.getChildFile("request3"));snap=p.snapshot();restore(p,"{\"version\":2}");check(p.snapshot().revision==snap.revision);check(root.getChildFile("release3").replaceWithText("ready"));waitIdle(p);check(p.ideaCount()==1&&saved(p)!=edited&&p.getStatus().contains("ready"));
 restore(p,legacy);p.request("{\"provider\":\"offline\"}");waitIdle(p);check(p.ideaCount()==8);
 std::cout<<"late-response restore/edit rejection, invalid restore preservation and worker recovery passed\n";
}
void lifecycleCases(){
 for(int size:{32,64})for(bool composer:{false,true})for(bool preparing:{false,true})for(bool resumed:{false,true}){
  auto owner=std::make_unique<Processor>();auto& p=*owner;HostClock clock;p.setPlayHead(&clock);p.prepareToPlay(48000,size);
  ProjectState candidate;candidate.activeMode=ProjectMode::composer;candidate.composition=fixture();
  candidate.composition->tracks[0].controls={Control{4,{0},64,127},Control{5,{0},66,127},Control{6,{0},69,127}};
  const auto encoded=encodeProjectJson(candidate);check(encoded);restore(p,composer?encoded.json:legacy);
  juce::AudioBuffer<float> audio(2,size),zero(2,0);juce::MidiBuffer midi;midi.ensureSize(32768);
  process(p,audio,midi);check(count(midi,true)==1);
  for(int cycle=0;cycle<2;++cycle){
   for(int repeat=0;repeat<2;++repeat){if(preparing)p.prepareToPlay(44100,size);else p.releaseResources();}
   clock.playing=resumed;clock.beat=0;process(p,zero,midi);check(midi.isEmpty());
   process(p,audio,midi);check(count(midi,false)==1&&count(midi,true)==0);
   int pedals=0;for(auto e:midi){check(e.samplePosition==0);auto m=e.getMessage();
    if(m.isNoteOff())check(m.getChannel()==(composer?16:1)&&m.getNoteNumber()==(composer?60:38));
    if(m.isController()){check(composer&&m.getChannel()==16&&m.getControllerValue()==0);check(m.getControllerNumber()==64||m.getControllerNumber()==66||m.getControllerNumber()==69);++pedals;}
   }
   check(pedals==(composer?3:0)&&midi.getNumEvents()==(composer?4:1));
   process(p,audio,midi);check(count(midi,false)==0&&count(midi,true)==(resumed?1:0));
   if(!resumed){check(midi.isEmpty());clock.playing=true;process(p,audio,midi);check(count(midi,true)==1&&count(midi,false)==0);}
   clock.beat=size*120/(60*(preparing?44100.0:48000.0));process(p,audio,midi);check(midi.isEmpty());
  }
  clock.playing=false;process(p,audio,midi);check(count(midi,false)==1);process(p,audio,midi);check(midi.isEmpty());
 }
 std::cout<<"release/reprepare retains notes and pedals until one allocation-free valid-block drain, then restarts at32/64samples\n";
}
void variableBlocksAndBypass(){
 for(bool composer:{false,true}){
  auto p=std::make_unique<Processor>();HostClock clock;p->setPlayHead(&clock);
  p->prepareToPlay(48000,64);
  restore(*p,composer?project(true,0):legacy);
  juce::AudioBuffer<float> audio(2,257),zero(2,0);
  juce::MidiBuffer midi;midi.ensureSize(32768);
  auto inject=[&]{midi.addEvent(juce::MidiMessage::noteOn(composer?16:1,composer?60:38,juce::uint8(80)),0);};
  int starts=0;
  for(int n:{32,64,1,127,17,257}){
   audio.setSize(2,n,false,false,true);
   inject();process(*p,audio,midi);starts+=count(midi,true);
   for(auto event:midi)check(event.samplePosition>=0&&event.samplePosition<n);
   clock.beat+=n*clock.bpm/(60*48000.0);clock.bpm=clock.bpm==120?96:120;
  }
  check(starts==1);
  midi.clear();bypass(*p,zero,midi);check(midi.isEmpty());
  inject();bypass(*p,audio,midi);
  check(count(midi,false)==1&&count(midi,true)==0&&audio.getMagnitude(0,audio.getNumSamples())==0);
  inject();bypass(*p,audio,midi);check(midi.isEmpty());
  clock.beat=0;process(*p,audio,midi);check(count(midi,true)==1&&count(midi,false)==0);
 }
}
void regionCases(){
 Config ambient;ambient.bars=4;ambient.style=Style::ambient;
 auto generated=generateLoop(ambient);check(generated.composition);
 const auto& ambientLoop=*generated.composition;
 const auto chord=std::find_if(ambientLoop.tracks.begin(),ambientLoop.tracks.end(),[](const Track& track){return track.role==Role::chords;});
 check(chord!=ambientLoop.tracks.end());
 auto innerBar=regenerateRegion(ambientLoop,chord->id,2,2,45);
 check(innerBar.composition);
 const auto* variedChords=loopEditDetail::track(*innerBar.composition,chord->id);
 check(std::any_of(variedChords->notes.begin(),variedChords->notes.end(),[](const Note& note){return note.start.value>=1920&&note.start.value<3840;}));
 Config fast;fast.bars=4;fast.style=Style::trap;fast.feel=Feel::doubleTime;fast.numerator=6;fast.denominator=8;
 auto fastLoop=generateLoop(fast);check(fastLoop.composition);
 const auto& lead=fastLoop.composition->tracks[2];
 auto variation=regenerateRegion(*fastLoop.composition,lead.id,2,2,8);check(variation.composition);
 const auto* leadVariation=loopEditDetail::track(*variation.composition,lead.id);
 auto attacks=[](const Track& t){return std::count_if(t.notes.begin(),t.notes.end(),[](const Note& n){return n.start.value>=1440&&n.start.value<2880;});};
 check(attacks(lead)==12&&attacks(*leadVariation)==attacks(lead));
 auto p=std::make_unique<Processor>();
 ManualOverrides manual;manual.pinned.set(size_t(Field::energy));manual.values.mood.energy=47;
 check(p->requestComposer("4 bars D minor trap bass chords lead drums",123,manual));waitIdle(*p);
 auto before=saved(*p);auto source=decodeProjectJson(before);check(source.project&&source.project->composition);
 auto& from=*source.project->composition;check(from.tracks.size()==4);
 const auto index=size_t(std::find_if(from.tracks.begin(),from.tracks.end(),[](const Track& t){return t.role==Role::lead;})-from.tracks.begin());
 check(index<from.tracks.size());
 const auto key="track-"+juce::String(from.tracks[index].id);
 const auto revision=p->compositionRevision();
 check(p->requestRegion(revision,key,2,3));waitIdle(*p);
 auto modified=saved(*p);auto result=decodeProjectJson(modified);
 check(result.project&&result.project->composition&&modified!=before);
 check(result.project->generationRecipeJson==source.project->generationRecipeJson);
 const auto& to=*result.project->composition;
 check(to.endTick==from.endTick&&to.tracks.size()==from.tracks.size());
 auto unchanged=[&](const Track& track,bool filter){
  std::vector<std::tuple<Id,int,int,int64_t,int64_t>> notes;
  for(const auto& n:track.notes)if(!filter||n.start.value<1920||n.start.value>=5760)
   notes.emplace_back(n.id,n.pitch,n.velocity,n.start.value,n.duration.value);
  std::sort(notes.begin(),notes.end());return notes;
 };
 for(size_t t=0;t<from.tracks.size();++t)check(unchanged(from.tracks[t],t==index)==unchanged(to.tracks[t],t==index));
 auto baseline=from,varied=to;
 baseline.tracks[index].notes.clear();varied.tracks[index].notes.clear();
 check(encodeCompositionJson(baseline).json==encodeCompositionJson(varied).json);
 check(!p->requestRegion(revision,key,2,3));
 check(p->requestRegion(p->compositionRevision(),key,3,99));waitIdle(*p);check(saved(*p)==modified);
 check(p->undoEdit()&&saved(*p)==before);
 check(p->redoEdit()&&saved(*p)==modified);
 bool cancelledRegion=false;
 for(int attempt=0;attempt<16&&!cancelledRegion;++attempt){
  check(p->requestRegion(p->compositionRevision(),key,2,3));
  cancelledRegion=p->cancel();waitIdle(*p);
  if(cancelledRegion)check(saved(*p)==modified);
  else restore(*p,modified);
 }
 check(cancelledRegion);
 check(p->requestRegion(p->compositionRevision(),key,2,3));restore(*p,before);
 waitIdle(*p);check(saved(*p)==before);
}
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;
 if(argc>1){check(argc==2);workerCases(juce::File(argv[1]));return 0;}
 lifecycleCases();variableBlocksAndBypass();regionCases();
 for(int size:{32,64}){
  auto owner=std::make_unique<Processor>();auto& p=*owner;HostClock clock;p.setPlayHead(&clock);p.prepareToPlay(48000,size);restore(p,legacy);
  juce::AudioBuffer<float> audio(2,size),zero(2,0);juce::MidiBuffer midi;midi.ensureSize(32768);
  process(p,audio,midi);check(count(midi,true,38)==1);
  const auto v2=project();restore(p,v2);check(saved(p)==v2); // RED: current Processor ignores v2
  process(p,zero,midi);check(midi.isEmpty());clock.beat=8;
  process(p,audio,midi);check(count(midi,false,38)==1&&count(midi,true)==0); // drain old mode
  process(p,audio,midi);check(count(midi,true,60)==1&&audio.getMagnitude(0,size)>0);
  auto beforeInvalid=p.snapshot();restore(p,"{\"version\":99}");check(p.snapshot().revision==beforeInvalid.revision&&saved(p)==v2);
  bool program=false,bank=false,cc=false;for(auto e:midi){auto m=e.getMessage();check(m.getChannel()==16);program|=m.isProgramChange()&&m.getProgramChangeNumber()==33;bank|=m.isController()&&m.getControllerNumber()==0&&m.getControllerValue()==2;cc|=m.isController()&&m.getControllerNumber()==1&&m.getControllerValue()==70;}check(program&&bank&&cc);
  clock.beat+=size*120/(60*48000.0);process(p,audio,midi);check(count(midi,true)==0&&count(midi,false)==0);
  clock.beat=8.5;process(p,audio,midi);check(count(midi,false,60)==1&&count(midi,true,60)==1);
  clock.beat=10.1;process(p,audio,midi);check(count(midi,true,60)==1); // second overlap still spans
  clock.playing=false;process(p,audio,midi);check(count(midi,false,60)==1);
  auto snap=p.snapshot();restore(p,"{\"version\":2}");check(saved(p)==v2&&p.snapshot().revision==snap.revision);
  restore(p,std::string(32*1024*1024+1,' '));check(saved(p)==v2);
  check(!p.edit(snap.revision,"bass",0,70,0,1)&&!p.quantize(snap.revision,"bass")&&!p.undoEdit()&&!p.redoEdit());
  {std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());int disabled=0;bool title=false,exportEnabled=false;
   for(int i=0;i<editor->getNumChildComponents();++i){auto* child=editor->getChildComponent(i);if(auto* b=dynamic_cast<juce::TextButton*>(child)){if(b->getButtonText()=="GENERATE 8 IDEAS"||b->getButtonText().startsWith("QUANTIZE")){check(!b->isEnabled());++disabled;}if(b->getButtonText()=="EXPORT MIDI")exportEnabled=b->isVisible()&&b->isEnabled();}if(auto* label=dynamic_cast<juce::Label*>(child))title|=label->getText()=="LoopForge";}
   check(disabled==2&&title&&exportEnabled);
  }
  p.choose(0);p.request("{\"provider\":\"offline\"}");check(!p.busy&&saved(p)==v2&&p.getIdea().isVoid());
  clock.playing=true;clock.beat=8;process(p,audio,midi);check(count(midi,true,60)==1);
  restore(p,project(true,8,61));process(p,audio,midi);check(count(midi,false,60)==1&&count(midi,true,61)==1);
  restore(p,legacy);check(saved(p)==legacy);clock.beat=0;
  process(p,audio,midi);check(count(midi,false,61)==1&&count(midi,true)==0);
  process(p,audio,midi);check(count(midi,true,38)==1);
  restore(p,project(false));snap=p.snapshot();check(p.edit(snap.revision,"bass",0,39,0,4));
  auto edited=decodeProjectJson(saved(p));check(edited.project&&edited.project->activeMode==ProjectMode::legacy&&edited.project->composition->tracks[0].notes[0].pitch==60);
  check(int(wire::document(*edited.project->legacyStateJson)["idea"]["layers"]["bass"][0]["pitch"])==39);
  const auto beforeBad=saved(p);snap=p.snapshot();check(p.canUndo());restore(p,"{\"version\":2}");check(saved(p)==beforeBad&&p.snapshot().revision==snap.revision&&p.canUndo());check(p.undoEdit());check(decodeProjectJson(saved(p)).project->composition->tracks[0].notes[0].pitch==60);check(p.redoEdit());
  clock.beat=8;clock.playing=true;restore(p,v2);int ons=0,offs=0;
  for(int i=0;clock.beat<8.6;++i){clock.bpm=i%2?90:180;process(p,audio,midi);ons+=count(midi,true,60);offs+=count(midi,false,60);clock.beat+=size*clock.bpm/(60*48000.0);}
  clock.playing=false;process(p,audio,midi);offs+=count(midi,false,60);check(ons==2&&offs==2);
 }
 for(bool host:{false,true}){
  auto owner=std::make_unique<Processor>();auto& p=*owner;HostClock clock;if(host)p.setPlayHead(&clock);p.prepareToPlay(48000,64);p.play=true;
  ProjectState state;state.activeMode=ProjectMode::composer;state.composition=fixture();state.composition->tracks[0].notes={Note{2,60,90,{0},{1}}};
  auto text=encodeProjectJson(state);check(text);restore(p,text.json);juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.ensureSize(32768);process(p,audio,midi);
  check(count(midi,true,60)==1&&count(midi,false,60)==1); // cold mode activation must not skip a complete short note
 }
 {auto owner=std::make_unique<Processor>();auto& p=*owner;p.prepareToPlay(48000,64);restore(p,project(true,999));p.play=true;juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.ensureSize(32768);
  int ons=0,offs=0;for(int i=0;i<1700;++i){process(p,audio,midi);ons+=count(midi,true);offs+=count(midi,false);}check(ons==2&&offs==2); // standalone ignores host origin; one shot
 }
 {
  auto owner=std::make_unique<Processor>();auto& p=*owner;HostClock clock;clock.beat=0.5;p.setPlayHead(&clock);p.prepareToPlay(48000,32);
  const std::string states[]={project(true,0,60),project(true,100,61),legacy,project(false,0)};
  std::atomic<bool> done=false;std::atomic<int> blocks=0;
  std::thread consumer([&]{juce::AudioBuffer<float> audio(2,32);juce::MidiBuffer midi;midi.ensureSize(32768);int held[16][128]{};
   auto tick=[&]{process(p,audio,midi);for(auto e:midi){auto m=e.getMessage();check(e.samplePosition>=0&&e.samplePosition<32);if(m.isNoteOn()){check(m.getNoteNumber()!=61);check(held[m.getChannel()-1][m.getNoteNumber()]++==0);}if(m.isNoteOff())check(--held[m.getChannel()-1][m.getNoteNumber()]==0);}};
   while(!done.load()){tick();++blocks;}
   clock.playing=false;tick();tick();for(auto& channel:held)for(int count:channel)check(count==0);
  });
  for(int i=0;i<1000;++i){restore(p,states[i%4]);}
  done=true;consumer.join();check(blocks>0);
 }
 {
  ProjectState candidate;candidate.activeMode=ProjectMode::composer;candidate.composition=fixture();auto& c=*candidate.composition;c.endTick={262144};auto& t=c.tracks[0];t.notes.clear();t.controls.clear();
  for(size_t i=0;i<131072;++i)t.notes.push_back({i+2,60,90,{int64_t(i*2)},{1}});
  for(size_t i=0;i<65536;++i)t.controls.push_back({200000+i,{int64_t(i*4)},1,int(i%128)});
  auto text=encodeProjectJson(candidate);check(text);auto owner=std::make_unique<Processor>();auto& p=*owner;p.prepareToPlay(48000,32);juce::AudioBuffer<float> audio(2,32);juce::MidiBuffer midi;midi.ensureSize(32768);
  for(int i=0;i<4;++i){restore(p,text.json);check(saved(p)==text.json);if(i!=1)process(p,audio,midi);} // replace both active and pending large sources
 }
 std::cout<<"Composer Processor restore/mode/host/standalone checks passed\n";
}
