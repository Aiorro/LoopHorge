#include "../Source/Processor.h"
#include <iostream>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
void check(bool value){if(!value)std::abort();}
struct Transport:juce::AudioPlayHead {
 bool playing=true;double beat=0;
 juce::Optional<PositionInfo> getPosition()const override{PositionInfo info;info.setIsPlaying(playing);info.setPpqPosition(beat);info.setBpm(100);return info;}
};
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;
 std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());Transport transport;p->setPlayHead(&transport);p->prepareToPlay(48000,512);
 juce::AudioBuffer<float> audio(2,512);juce::MidiBuffer midi;p->processBlock(audio,midi);
 int ons=0;for(auto m:midi)if(m.getMessage().isNoteOn())++ons;check(ons>=4);
 transport.playing=false;p->processBlock(audio,midi);int offs=0;for(auto m:midi)if(m.getMessage().isNoteOff())++offs;check(offs>=4);
 if(argc>1){auto idea=juce::JSON::parse(juce::File(argv[1]));check(idea.isObject());auto* state=new juce::DynamicObject;state->setProperty("version",1);state->setProperty("idea",idea);
  auto text=juce::JSON::toString(juce::var(state));p->setStateInformation(text.toRawUTF8(),int(text.getNumBytesAsUTF8()));
  juce::MemoryBlock saved;p->getStateInformation(saved);auto decoded=juce::JSON::parse(saved.toString());check(int(decoded["version"])==1);check(decoded["idea"]["layers"]["bass"].size()>0);
  const char bad[]="{\"version\":1,\"idea\":{}}";p->setStateInformation(bad,sizeof(bad)-1);juce::MemoryBlock after;p->getStateInformation(after);check(saved==after);
 }
 if(argc>2){auto* composer=dynamic_cast<Processor*>(p.get());check(composer!=nullptr);
  composer->request("{\"provider\":\"offline\",\"count\":8,\"key\":\"D\",\"bars\":4,\"bpm\":100}");
  for(int i=0;i<200&&composer->busy;++i)juce::Thread::sleep(50);
  std::cerr<<"API status: "<<composer->getStatus()<<", count="<<composer->ideaCount()<<"\n";check(!composer->busy);check(composer->ideaCount()==8);std::cout<<"native worker -> HTTP API -> eight ideas passed\n";
 }
 if(argc>3){std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());
  auto image=editor->createComponentSnapshot(editor->getLocalBounds());check(image.isValid());
  juce::FileOutputStream out{juce::File(argv[3])};check(out.openedOk());juce::PNGImageFormat format;check(format.writeImageToStream(image,out));
 }
 p->releaseResources();std::cout<<"processor MIDI, stop note-offs, state round-trip and invalid state passed\n";
}
