#include "../Source/Processor.h"
#include <iostream>
#include <thread>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
void check(bool v){if(!v)std::abort();}
struct Clock:juce::AudioPlayHead{
 double beat=0,bpm=240;bool playing=true;
 juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setPpqPosition(beat);p.setBpm(bpm);p.setIsPlaying(playing);return p;}
};
int main(){
 juce::ScopedJuceInitialiser_GUI init;std::unique_ptr<juce::AudioProcessor> processor(createPluginFilter());Clock clock;processor->setPlayHead(&clock);
 auto* layers=new juce::DynamicObject;int total=0;
 for(const char* name:{"bass","chords","melody"}){juce::Array<juce::var> notes;int count=total<1200?600:400;
  for(int i=0;i<count;++i){auto* n=new juce::DynamicObject;n->setProperty("pitch",24+i%72);n->setProperty("velocity",100);n->setProperty("start",(i%128)*0.125);n->setProperty("duration",0.125);notes.add(juce::var(n));}total+=count;layers->setProperty(name,notes);}
 auto* meta=new juce::DynamicObject;meta->setProperty("bars",4);meta->setProperty("bpm",240);
 auto* idea=new juce::DynamicObject;idea->setProperty("metadata",juce::var(meta));idea->setProperty("layers",juce::var(layers));
 auto* state=new juce::DynamicObject;state->setProperty("version",1);state->setProperty("idea",juce::var(idea));const auto text=juce::JSON::toString(juce::var(state));
 std::vector<double> times;
 for(int size:{32,64,128,512,2048}){
  processor->prepareToPlay(48000,size);processor->setStateInformation(text.toRawUTF8(),int(text.getNumBytesAsUTF8()));
  juce::AudioBuffer<float> audio(2,size);juce::MidiBuffer midi;midi.ensureSize(65536);times.clear();clock.beat=0;clock.playing=true;
  for(int block=0;block<300;++block){double start=juce::Time::getMillisecondCounterHiRes();processor->processBlock(audio,midi);times.push_back(juce::Time::getMillisecondCounterHiRes()-start);
   for(auto event:midi){check(event.samplePosition>=0&&event.samplePosition<size);check(event.getMessage().getNoteNumber()<=127);}
   for(int ch=0;ch<2;++ch)for(int s=0;s<size;++s)check(std::isfinite(audio.getSample(ch,s)));
   clock.beat+=size*clock.bpm/(60*48000);if(block%57==0)clock.beat=0;
  }
  std::sort(times.begin(),times.end());double sum=0;for(auto v:times)sum+=v;
  std::cout<<"1600 notes block="<<size<<" mean_ms="<<sum/times.size()<<" p99_ms="<<times[297]<<" max_ms="<<times.back()<<" budget_ms="<<size/48.0<<"\n";
 }
 auto dense=juce::JSON::parse(text);
 for(const char* layer:{"bass","chords","melody"}){auto notes=dense["idea"]["layers"][layer];int pitch=0;for(auto& note:*notes.getArray()){note.getDynamicObject()->setProperty("pitch",pitch++%128);note.getDynamicObject()->setProperty("velocity",127);note.getDynamicObject()->setProperty("start",0);note.getDynamicObject()->setProperty("duration",4);}}
 auto denseText=juce::JSON::toString(dense);processor->prepareToPlay(48000,2048);processor->setStateInformation(denseText.toRawUTF8(),int(denseText.getNumBytesAsUTF8()));clock.beat=0;
 {juce::AudioBuffer<float> denseAudio(2,2048);juce::MidiBuffer denseMidi;processor->processBlock(denseAudio,denseMidi);float peak=denseAudio.getMagnitude(0,2048);std::cout<<"dense audition peak="<<peak<<std::endl;check(peak<=1.0f);}
 // Concurrent producer updates must never mutate the schedule being consumed by audio.
 std::thread producer([&]{for(int i=0;i<500;++i)processor->setStateInformation(text.toRawUTF8(),int(text.getNumBytesAsUTF8()));});
 juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.ensureSize(65536);
 for(int i=0;i<3000;++i){processor->processBlock(audio,midi);clock.beat+=64*4.0/48000;}
 producer.join();clock.playing=false;processor->processBlock(audio,midi);processor->releaseResources();
 std::cout<<"500 concurrent state replacements / 3000 audio blocks passed\n";
}
