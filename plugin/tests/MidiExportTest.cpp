#include "../Source/MidiExport.h"
#include <iostream>
void check(bool v,const char* message){if(!v){std::cerr<<message<<'\n';std::abort();}}
int main(){
 auto idea=juce::JSON::parse("{\"metadata\":{\"bars\":4,\"bpm\":120},\"layers\":{\"bass\":[{\"pitch\":60,\"start\":0,\"duration\":0.001,\"velocity\":100}],\"chords\":[],\"melody\":[]}}");
 auto bytes=encodeMidi(idea);check(bytes.getSize()>14,"empty MIDI");juce::MemoryInputStream input(bytes,false);juce::MidiFile decoded;int type=0;check(decoded.readFrom(input,false,&type),"MIDI read failed");check(type==1&&decoded.getNumTracks()==4&&decoded.getTimeFormat()==480,"track/type/PPQ changed");
 const auto* bass=decoded.getTrack(1);double on=-1,off=-1;for(auto* event:*bass){auto m=event->message;if(m.isNoteOn())on=m.getTimeStamp();if(m.isNoteOff())off=m.getTimeStamp();}
 bool okay=true;auto expect=[&](bool value,const char* message){if(!value){std::cerr<<message<<"\n";okay=false;}};
 expect(on==0&&off==1,"short note vanished in MIDI tick quantization");
 for(int i=0;i<4;++i)expect(decoded.getTrack(i)->getEndTime()==7680,"trailing pattern rest was lost");
 bool meter=false,tempo=false;for(auto* e:*decoded.getTrack(0)){meter|=e->message.isTimeSignatureMetaEvent();tempo|=e->message.isTempoMetaEvent();}expect(meter&&tempo,"tempo or meter missing");if(!okay)return 1;

 idea["metadata"].getDynamicObject()->setProperty("bars",8);auto* note=idea["layers"]["bass"][0].getDynamicObject();note->setProperty("start",31.999);note->setProperty("duration",0.001);
 bytes=encodeMidi(idea);juce::MemoryInputStream tailInput(bytes,false);juce::MidiFile tail;check(tail.readFrom(tailInput,false),"tail MIDI read failed");on=off=-1;for(auto* event:*tail.getTrack(1)){if(event->message.isNoteOn())on=event->message.getTimeStamp();if(event->message.isNoteOff())off=event->message.getTimeStamp();}check(on==15359&&off==15360&&tail.getTrack(0)->getEndTime()==15360,"final boundary quantization lost note or pattern length");
 check(encodeMidi(juce::var{}).isEmpty(),"invalid composition exported");
 std::cout<<"native MIDI type1/PPQ, one-tick notes, full pattern length and meter passed\n";
}
