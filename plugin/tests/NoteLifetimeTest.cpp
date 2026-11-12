#include "../Source/Processor.h"
#include <iostream>
void check(bool v,const char* message){if(!v){std::cerr<<message<<'\n';std::abort();}}
struct Clock:juce::AudioPlayHead {
 double beat=0,bpm=120;bool playing=true;
 juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setPpqPosition(beat);p.setBpm(bpm);p.setIsPlaying(playing);return p;}
};
void install(Processor& p,const char* notes){
 auto text=juce::String("{\"version\":1,\"idea\":{\"metadata\":{\"bars\":4,\"bpm\":120},\"layers\":{\"bass\":")+notes+",\"chords\":[],\"melody\":[]}}}";
 p.setStateInformation(text.toRawUTF8(),int(text.getNumBytesAsUTF8()));
}
int main(){
 juce::ScopedJuceInitialiser_GUI init;
 for(int size:{32,64}){
  Processor p;Clock clock;p.setPlayHead(&clock);p.prepareToPlay(48000,size);
  install(p,"[{\"pitch\":60,\"start\":0,\"duration\":1,\"velocity\":100},{\"pitch\":60,\"start\":0.5,\"duration\":1,\"velocity\":80}]");
  juce::AudioBuffer<float> audio(2,size);juce::MidiBuffer midi;midi.ensureSize(65536);
  int held=0,ons=0,offs=0;bool heardAfterFirstRelease=false;
  for(int block=0;clock.beat<1.6;++block){
   clock.bpm=block%2?90:180;
   p.processBlock(audio,midi);
   for(auto e:midi){auto m=e.getMessage();check(e.samplePosition>=0&&e.samplePosition<size,"MIDI offset out of block");
    if(m.isNoteOff()){check(held==1,"unmatched note off");--held;++offs;}
    if(m.isNoteOn()){check(held==0,"retrigger must release previous MIDI voice");++held;++ons;}
   }
   if(clock.beat>1.05&&clock.beat<1.4){check(held==1,"first release cut overlapping note");heardAfterFirstRelease|=audio.getMagnitude(0,size)>0;}
   clock.beat+=size*clock.bpm/(60*48000.0);
  }
  check(ons==2&&offs==2&&held==0,"repeated notes must have balanced edges");check(heardAfterFirstRelease,"audition cut overlapping note");
  for(int mode=0;mode<4;++mode){
   clock.beat=0;clock.playing=true;p.processBlock(audio,midi);
   clock.beat=size*clock.bpm/(60*48000.0);p.processBlock(audio,midi);
   if(mode==0)clock.playing=false;
   if(mode==1)clock.beat=4;
   if(mode==2)clock.beat=16;
   if(mode==3)install(p,"[{\"pitch\":61,\"start\":2,\"duration\":1,\"velocity\":100}]");
   p.processBlock(audio,midi);int released=0;for(auto e:midi)if(e.getMessage().isNoteOff()&&e.getMessage().getNoteNumber()==60)++released;
   if(released!=1)std::cerr<<"mode="<<mode<<" released="<<released<<"\n";check(released==1,"stop/seek/loop/state replacement must release once");
  }
 }

 for(bool faster:{true,false}){
  Processor p;Clock clock;clock.bpm=faster?90:180;p.setPlayHead(&clock);p.prepareToPlay(48000,32);
  install(p,faster?"[{\"pitch\":60,\"start\":0.00095,\"duration\":0.2,\"velocity\":100}]":"[{\"pitch\":60,\"start\":0.00195,\"duration\":0.2,\"velocity\":100}]");
  juce::AudioBuffer<float> audio(2,32);juce::MidiBuffer midi;midi.ensureSize(32768);p.processBlock(audio,midi);int first=0;for(auto e:midi)first+=e.getMessage().isNoteOn();check(first==(faster?1:0),"boundary note in wrong first block");
  clock.beat=32*clock.bpm/(60*48000.0);clock.bpm=faster?180:90;p.processBlock(audio,midi);int second=0;for(auto e:midi)second+=e.getMessage().isNoteOn();check(second==(faster?0:1),"tempo change duplicated or skipped boundary note");
 }
 {
  Processor p;Clock clock;clock.beat=-16;p.setPlayHead(&clock);p.prepareToPlay(48000,64);install(p,"[{\"pitch\":60,\"start\":0,\"duration\":2,\"velocity\":100}]");
  juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.ensureSize(32768);p.processBlock(audio,midi);int ons=0;for(auto event:midi)ons+=event.getMessage().isNoteOn();check(ons==1,"negative PPQ onset missing");
  clock.beat=-10;p.processBlock(audio,midi);int offs=0;for(auto event:midi)offs+=event.getMessage().isNoteOff();check(offs==1,"seek during negative PPQ left a held note");
 }
 std::cout<<"overlap, retrigger, tempo at 32/64, stop/seek/loop/state replacement passed\n";
}
