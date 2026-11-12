#include "../Source/Processor.h"
#include "../Source/MidiExport.h"
#include <iostream>
#define check(x) do{if(!(x)){std::cerr<<"editing assertion "<<__LINE__<<'\n';std::abort();}}while(false)
int main(){juce::ScopedJuceInitialiser_GUI init;Processor p;
 p.request("{\"provider\":\"offline\",\"seed\":42}");for(int i=0;i<500&&p.busy;++i)juce::Thread::sleep(10);check(!p.busy);
 auto original=p.snapshot();auto text=juce::JSON::toString(original.idea);int count=original.idea["layers"]["bass"].size();
 check(p.edit(original.revision,"bass",-1,60,.13,.8));auto added=p.snapshot();check(added.idea["layers"]["bass"].size()==count+1);auto n=added.idea["layers"]["bass"][count];check(double(n["start"])==.25&&double(n["duration"])==.75&&int(n["velocity"])==90);
 check(juce::JSON::toString(original.idea)==text);check(!p.edit(original.revision,"bass",count,61,1,1));check(p.canUndo());check(p.undoEdit());check(juce::JSON::toString(p.getIdea())==text);check(p.canRedo());
 auto snap=p.snapshot();auto old=snap.idea["layers"]["bass"][0];check(!p.edit(snap.revision,"bass",0,int(old["pitch"]),double(old["start"]),double(old["duration"])));check(p.canRedo());check(p.redoEdit());
 snap=p.snapshot();check(p.edit(snap.revision,"bass",count,127,100,1));snap=p.snapshot();n=snap.idea["layers"]["bass"][count];double end=double(snap.idea["metadata"]["bars"])*4;check(double(n["start"])+double(n["duration"])<=end);
 check(p.edit(snap.revision,"bass",count,60,0,1,true));check(p.undoEdit());check(p.redoEdit());
 snap=p.snapshot();check(!p.edit(snap.revision,"wrong",-1,60,0,1));check(!p.edit(snap.revision,"bass",99999,60,0,1));check(!p.edit(snap.revision,"bass",-1,60,std::nan(""),1));
 for(int i=0;i<70;++i){snap=p.snapshot();check(p.edit(snap.revision,"bass",0,40+i%2,1,1));}int undone=0;while(p.undoEdit())++undone;check(undone==64);
 p.choose(1);check(!p.canUndo()&&!p.canRedo());snap=p.snapshot();check(p.edit(snap.revision,"melody",-1,127,end-.01,.001));auto bytes=encodeMidi(p.getIdea());check(bytes.getSize()>0);juce::MemoryInputStream input(bytes,false);juce::MidiFile midi;check(midi.readFrom(input,false));bool exported=false;for(auto* e:*midi.getTrack(3))if(e->message.isNoteOn()&&e->message.getNoteNumber()==127)exported=true;check(exported&&midi.getTrack(0)->getEndTime()==end*480);
 juce::MemoryBlock state;p.getStateInformation(state);p.setStateInformation(state.getData(),int(state.getSize()));check(!p.canUndo()&&!p.canRedo());check(!p.edit(snap.revision,"bass",0,60,0,1));
 snap=p.snapshot();check(p.edit(snap.revision,"bass",-1,60,0,1));p.request("{\"provider\":\"offline\"}");for(int i=0;i<500&&p.busy;++i)juce::Thread::sleep(10);check(!p.canUndo()&&!p.canRedo());

 auto one=juce::JSON::parse("{\"version\":1,\"idea\":{\"metadata\":{\"bars\":4,\"bpm\":120},\"layers\":{\"bass\":[{\"pitch\":60,\"start\":0.13,\"duration\":0.2,\"velocity\":90}],\"chords\":[],\"melody\":[]}}}");auto raw=juce::JSON::toString(one);p.setStateInformation(raw.toRawUTF8(),int(raw.getNumBytesAsUTF8()));snap=p.snapshot();check(!p.edit(snap.revision,"bass",0,0,0,1,true));check(!p.canUndo());check(p.quantize(snap.revision,"bass"));snap=p.snapshot();check(double(snap.idea["layers"]["bass"][0]["start"])==.25&&double(snap.idea["layers"]["bass"][0]["duration"])==.25);check(!p.quantize(snap.revision,"bass"));check(p.undoEdit());

 auto grid=one.clone();grid["idea"]["layers"]["bass"][0].getDynamicObject()->setProperty("start",0);grid["idea"]["layers"]["bass"][0].getDynamicObject()->setProperty("duration",1);raw=juce::JSON::toString(grid);p.setStateInformation(raw.toRawUTF8(),int(raw.getNumBytesAsUTF8()));snap=p.snapshot();check(p.edit(snap.revision,"bass",0,61,0,1));check(p.undoEdit());snap=p.snapshot();check(!p.quantize(snap.revision,"bass"));check(p.canRedo());
 auto full=one.clone();auto* list=full["idea"]["layers"]["bass"].getArray();auto note=(*list)[0];for(int i=1;i<800;++i)list->add(note.clone());raw=juce::JSON::toString(full);p.setStateInformation(raw.toRawUTF8(),int(raw.getNumBytesAsUTF8()));snap=p.snapshot();check(!p.edit(snap.revision,"bass",-1,60,0,1));check(!p.canUndo());
 auto both=full.clone();both["idea"]["layers"].getDynamicObject()->setProperty("chords",both["idea"]["layers"]["bass"].clone());raw=juce::JSON::toString(both);p.setStateInformation(raw.toRawUTF8(),int(raw.getNumBytesAsUTF8()));snap=p.snapshot();check(!p.edit(snap.revision,"melody",-1,60,0,1));check(!p.canUndo());
 std::cout<<"native validated edits, isolation, revision, bounded undo and replacement passed\n";
}
