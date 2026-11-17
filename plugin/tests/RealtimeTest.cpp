#include "../Source/Processor.h"
#include <juce_audio_processors/format_types/juce_VST3Headers.h>
#include <juce_audio_processors/utilities/juce_FlagCache.h>
#include <juce_audio_processors/format_types/juce_VST3Common.h>
#include <cstdlib>
#include <iostream>
thread_local bool tracking=false;thread_local int allocations=0;
extern "C" void* __real_malloc(size_t);extern "C" void* __real_calloc(size_t,size_t);extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){if(tracking)++allocations;return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t s){if(tracking)++allocations;return __real_calloc(n,s);}
extern "C" void* __wrap_realloc(void* p,size_t n){if(tracking)++allocations;return __real_realloc(p,n);}
void* operator new(size_t n){if(tracking)++allocations;if(auto* p=std::malloc(n))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,size_t)noexcept{std::free(p);}void operator delete[](void* p,size_t)noexcept{std::free(p);}
void check(bool v){if(!v)std::abort();}
struct Clock:juce::AudioPlayHead{double beat=0,bpm=1000;bool playing=true;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(bpm);p.setPpqPosition(beat);p.setIsPlaying(playing);return p;}};
int main(){juce::ScopedJuceInitialiser_GUI init;Processor p;Clock clock;p.setPlayHead(&clock);
 auto* layers=new juce::DynamicObject;
 for(const char* layer:{"bass","chords","melody"}){juce::Array<juce::var> notes;int count=juce::String(layer)=="melody"?0:800;for(int i=0;i<count;++i){auto* n=new juce::DynamicObject;n->setProperty("pitch",i%128);n->setProperty("velocity",100);n->setProperty("start",0);n->setProperty("duration",.01);notes.add(juce::var(n));}layers->setProperty(layer,notes);}
 auto* meta=new juce::DynamicObject;meta->setProperty("bars",4);meta->setProperty("bpm",240);auto* idea=new juce::DynamicObject;idea->setProperty("metadata",juce::var(meta));idea->setProperty("layers",juce::var(layers));auto* state=new juce::DynamicObject;state->setProperty("version",1);state->setProperty("idea",juce::var(idea));auto text=juce::JSON::toString(juce::var(state));
 for(int size:{32,64,2048,48000*4}){
  p.prepareToPlay(48000,size);p.setStateInformation(text.toRawUTF8(),int(text.getNumBytesAsUTF8()));juce::AudioBuffer<float> audio(2,size);clock.beat=0;
  for(int block=0;block<8;++block){juce::MidiBuffer midi;midi.ensureSize(32768);clock.bpm=block%2?90:1000;clock.playing=block!=3;allocations=0;tracking=true;p.processBlock(audio,midi);tracking=false;
   if(allocations){std::cerr<<"callback allocations="<<allocations<<" size="<<size<<'\n';return 1;}check(midi.data.size()<=32768);
   juce::MidiEventList delivered;juce::MidiEventList::pluginToHostEventList(delivered,midi);
   if(delivered.getEventCount()!=midi.getNumEvents()){std::cerr<<"VST3 discarded MIDI events\n";return 1;}
   if(p.getStatus().contains("limit reached")){int balance[16][128]{};for(auto event:midi){auto msg=event.getMessage();balance[msg.getChannel()-1][msg.getNoteNumber()]+=msg.isNoteOn()?1:-1;}for(auto& ch:balance)for(int held:ch)check(held==0);}

   clock.beat+=size*clock.bpm/(60*48000.0);
  }
 }
 for(int size:{32,64,2048}){composer::Composition c;c.id="preview-RT";c.endTick={7680};c.tempos={{{0},500000}};c.meters={{{0},4,4}};c.keys={{{0},0,composer::Scale::naturalMinor}};composer::Track t;t.id=1;t.name="Dense preview";t.channel=16;for(int i=0;i<128;++i)t.notes.push_back({uint64_t(i+2),i,100,{0},{240}});c.tracks={t};composer::ProjectState project;project.activeMode=composer::ProjectMode::composer;project.composition=c;auto encoded=composer::encodeProjectJson(project);check(bool(encoded));p.prepareToPlay(48000,size);check(p.restoreStateInformation(encoded.json.data(),int(encoded.json.size())));clock.beat=0;juce::AudioBuffer<float> audio(2,size);
  for(int block=0;block<8;++block){juce::MidiBuffer midi;midi.ensureSize(32768);clock.bpm=block%2?90:180;clock.playing=block!=3;allocations=0;tracking=true;p.processBlock(audio,midi);tracking=false;if(allocations){std::cerr<<"Composer preview allocated "<<allocations<<" size="<<size<<'\n';return 1;}check(midi.data.size()<=32768);check(audio.getMagnitude(0,size)<=1);clock.beat+=size*clock.bpm/(60*48000.0);}
 }
 std::cout<<"no callback allocations: cold/dense/tempo/stop at 32..192000 samples, fresh prepared host buffers\n";
}
