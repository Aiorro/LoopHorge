#include "../Source/Processor.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>

namespace faults {thread_local long remaining=-1;thread_local size_t count=0;thread_local bool hit=false;}
void* operator new(size_t n){++faults::count;if(faults::remaining==0){faults::remaining=-1;faults::hit=true;throw std::bad_alloc();}if(faults::remaining>0)--faults::remaining;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}
void* operator new(size_t n,const std::nothrow_t&)noexcept{try{return ::operator new(n);}catch(...){return nullptr;}}
void* operator new[](size_t n,const std::nothrow_t& tag)noexcept{return ::operator new(n,tag);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,size_t)noexcept{std::free(p);}void operator delete[](void* p,size_t)noexcept{std::free(p);}
void operator delete(void* p,const std::nothrow_t&)noexcept{std::free(p);}void operator delete[](void* p,const std::nothrow_t&)noexcept{std::free(p);}
#if LOOPFORGE_WRAP_ASYNC_NOTIFICATION
namespace faults {std::atomic<bool> notification{false};}
extern "C" void __real__ZN4juce12AsyncUpdater18triggerAsyncUpdateEv(juce::AsyncUpdater*);
extern "C" void __wrap__ZN4juce12AsyncUpdater18triggerAsyncUpdateEv(juce::AsyncUpdater* updater){
 if(faults::notification.exchange(false)){faults::remaining=0;auto* p=::operator new(1);::operator delete(p);}
 __real__ZN4juce12AsyncUpdater18triggerAsyncUpdateEv(updater);
}
#endif
#define check(x) do{if(!(x)){faults::remaining=-1;std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
using namespace composer;
const std::string legacy=R"({"version":1,"idea":{"metadata":{"bars":4,"bpm":120},"layers":{"bass":[{"pitch":38,"start":0,"duration":4,"velocity":90}],"chords":[],"melody":[]}}})";
std::string project(int pitch){ProjectState p;p.activeMode=ProjectMode::composer;p.legacyStateJson=legacy;Composition c;c.id="safety";c.endTick={1920};c.tempos={{{0},500000}};c.meters={{{0},4,4}};c.keys={{{0},0,Scale::major}};Track t;t.id=1;t.name="Track";t.channel=16;t.notes={{2,pitch,90,{0},{960}}};t.controls={{3,{0},64,127}};c.tracks={t};p.composition=std::move(c);auto encoded=encodeProjectJson(p);check(encoded);return encoded.json;}
std::string saved(Processor& p){juce::MemoryBlock b;p.getStateInformation(b);return {static_cast<const char*>(b.getData()),b.getSize()};}
struct HostClock:juce::AudioPlayHead{bool playing=true;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(120);p.setPpqPosition(0);p.setIsPlaying(playing);return p;}};
struct Rig {
 std::unique_ptr<Processor> owner=std::make_unique<Processor>();Processor& p=*owner;HostClock clock;juce::AudioBuffer<float> audio{2,64};juce::MidiBuffer midi;
 Rig(bool composer,bool history=true){p.setPlayHead(&clock);p.prepareToPlay(48000,64);midi.ensureSize(32768);check(p.restoreStateInformation(legacy.data(),int(legacy.size())));if(composer){auto s=project(60);check(p.restoreStateInformation(s.data(),int(s.size())));}else if(history){auto revision=p.snapshot().revision;check(p.edit(revision,"bass",0,39,0,4));check(p.edit(revision+1,"bass",0,40,0,4));check(p.undoEdit());}p.processBlock(audio,midi);}
 bool render(int pitch){clock.playing=false;p.processBlock(audio,midi);clock.playing=true;int ons=0;bool correct=true;for(int i=0;i<2;++i){p.processBlock(audio,midi);for(auto e:midi)if(e.getMessage().isNoteOn()){++ons;correct&=e.getMessage().getNoteNumber()==pitch;}}return correct&&ons>=1;}
};
int failures=0;
void expect(bool value,const char* what,const char* path,long position){if(!value){++failures;std::cerr<<path<<" allocation "<<position<<": "<<what<<"\n";}}
void attempt(const char* path,bool composer,const std::string& target,long position,bool history=true){
 Rig rig(composer,history);auto& p=rig.p;const auto before=saved(p);const auto snap=p.snapshot();const bool undo=p.canUndo(),redo=p.canRedo();
 faults::hit=false;faults::remaining=position;faults::count=0;bool result=false,escaped=false;try{result=p.restoreStateInformation(target.data(),int(target.size()));}catch(const std::bad_alloc&){escaped=true;}faults::remaining=-1;const auto count=faults::count;
 expect(!escaped,"allocation escaped restore",path,position);
 if(!result){expect(saved(p)==before,"rejected restore changed saved bytes",path,position);expect(p.snapshot().revision==snap.revision&&p.snapshot().replacement==snap.replacement,"rejected restore changed revision",path,position);expect(p.isComposerMode()==composer,"rejected restore changed mode",path,position);expect(p.canUndo()==undo&&p.canRedo()==redo,"rejected restore changed history",path,position);expect(rig.render(composer?60:history?39:38),"rejected restore changed rendered source",path,position);}
 else{expect(saved(p)==target,"success changed original saved bytes",path,position);expect(p.snapshot().revision==snap.revision+1,"success did not advance revision once",path,position);expect(!p.canUndo()&&!p.canRedo(),"success retained prior history",path,position);expect(rig.render(target==legacy?38:61),"success did not publish matching source",path,position);}
 // Replace an unconsumed pending candidate: a leaked writing slot makes the second restore fail.
 for(int i=0;i<2;++i){auto a=project(62),b=project(63);expect(p.restoreStateInformation(a.data(),int(a.size())),"recovery first restore failed",path,position);expect(p.restoreStateInformation(b.data(),int(b.size())),"recovery pending replacement failed",path,position);expect(rig.render(63),"recovery did not publish latest source",path,position);}
 if(position<0)std::cout<<path<<" successful restore allocations="<<count<<"\n";
}
size_t allocationCount(bool composer,const std::string& target){Rig rig(composer);faults::count=0;check(rig.p.restoreStateInformation(target.data(),int(target.size())));return faults::count;}
void selectModeCases(){
 for(bool composer:{false,true}){
  size_t positions=0;
  for(long position=-1;position<long(positions);++position){
   Rig rig(true);auto& p=rig.p;if(!composer)check(p.selectMode(ProjectMode::legacy));
   const auto before=saved(p);const auto snap=p.snapshot();const auto mode=composer?ProjectMode::legacy:ProjectMode::composer;
   faults::count=0;faults::remaining=position;bool result=false,escaped=false;try{result=p.selectMode(mode);}catch(const std::bad_alloc&){escaped=true;}faults::remaining=-1;if(position<0)positions=faults::count;
   expect(!escaped,"allocation escaped mode selection","selectMode",position);
   if(!result){expect(saved(p)==before&&p.snapshot().revision==snap.revision&&p.snapshot().replacement==snap.replacement,"rejected mode selection changed state","selectMode",position);expect(p.isComposerMode()==composer&&rig.render(composer?60:38),"rejected mode selection changed source","selectMode",position);}
   else{expect(p.isComposerMode()!=composer&&p.snapshot().revision==snap.revision+1,"successful mode selection did not commit once","selectMode",position);expect(rig.render(composer?38:60),"successful mode selection did not publish source","selectMode",position);}
   check(p.selectMode(p.isComposerMode()?ProjectMode::legacy:ProjectMode::composer));
  }
  std::cout<<"selectMode "<<(composer?"Composer->legacy":"legacy->Composer")<<" swept "<<positions<<" allocation positions\n";
 }
}
void notificationCases(){
#if LOOPFORGE_WRAP_ASYNC_NOTIFICATION
 {Rig rig(true);auto& p=rig.p;const auto revision=p.snapshot().revision;faults::notification=true;const bool result=p.selectMode(ProjectMode::legacy);expect(!faults::notification,"notification injection not reached","selectMode notify",0);expect(result&&!p.isComposerMode()&&p.snapshot().revision==revision+1&&rig.render(38),"notification failure rejected committed mode/source","selectMode notify",0);check(p.selectMode(ProjectMode::composer));}
 {Rig rig(false);auto& p=rig.p;const auto before=saved(p);const auto revision=p.snapshot().revision;faults::notification=true;check(p.requestComposer("C major 4 bars seed 7",7));for(int i=0;i<500&&p.busy;++i)juce::Thread::sleep(10);check(!p.busy);juce::Thread::sleep(10);expect(!faults::notification,"notification injection not reached","generated notify",0);expect(p.isComposerMode()&&saved(p)!=before&&p.snapshot().revision==revision+1&&p.getStatus().contains("Native Composer ready"),"notification failure rejected generated commit","generated notify",0);auto decoded=decodeProjectJson(saved(p));check(decoded.project&&decoded.project->composition);const auto pitch=decoded.project->composition->tracks[0].notes[0].pitch;rig.clock.playing=false;p.processBlock(rig.audio,rig.midi);rig.clock.playing=true;p.processBlock(rig.audio,rig.midi);bool on=false;for(auto e:rig.midi)on|=e.getMessage().isNoteOn();expect(on,"generated notification failure lost playback publication","generated notify",pitch);check(p.requestComposer("C major 4 bars seed 8",8));for(int i=0;i<500&&p.busy;++i)juce::Thread::sleep(10);check(!p.busy&&p.getStatus().contains("Native Composer ready"));}
 std::cout<<"generated/selectMode notification bad_alloc preserves successful commit and recovery\n";
#endif
}
int main(){juce::ScopedJuceInitialiser_GUI init;const auto target=project(61);
 attempt("historical106",false,target,106,false);
 for(bool composer:{false,true}){const auto n=allocationCount(composer,target);for(size_t i=0;i<n;++i)attempt(composer?"Composer->Composer":"legacy->Composer",composer,target,long(i));std::cout<<(composer?"Composer->Composer":"legacy->Composer")<<" swept "<<n<<" allocation positions\n";}
 for(bool composer:{false,true}){const auto n=allocationCount(composer,legacy);for(size_t i=0;i<n;++i)attempt(composer?"Composer->legacy":"legacy->legacy",composer,legacy,long(i));std::cout<<(composer?"Composer->legacy":"legacy->legacy")<<" swept "<<n<<" allocation positions\n";}
 selectModeCases();notificationCases();
 std::cout<<"restore allocation failures="<<failures<<"\n";return failures?1:0;
}
