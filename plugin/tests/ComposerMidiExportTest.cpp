#include "../Source/ComposerMidiExport.h"
#include "../Source/Processor.h"
#include "../Source/ProjectFiles.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <iostream>
#include <cstring>
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
using namespace composer;
const std::string legacy=R"({"version":1,"idea":{"metadata":{"bars":4,"bpm":120},"layers":{"bass":[{"pitch":38,"start":0,"duration":4,"velocity":90}],"chords":[],"melody":[]}}})";
Composition fixture(){
 Composition c;c.id="export snapshot";c.endTick={3840};c.tempos={{{0},500000},{{1920},400000}};c.meters={{{0},4,4},{{1920},3,4}};c.keys={{{0},0,Scale::major},{{1920},2,Scale::naturalMinor}};
 c.sections={{300,"Intro",{0},{1920},SectionRole::intro},{301,"Outro",{1920},{3840},SectionRole::outro}};
 for(int ch=1;ch<=16;++ch){Track t;t.id=Id(ch);t.name="Track "+std::to_string(ch);t.channel=ch;t.role=ch==10?Role::drums:Role::lead;t.program=ch;t.bank=Bank{2,7};t.notes={{Id(100+ch),35+ch,90,{(ch-1)*120},{240}}};t.controls={{Id(200+ch),{1200},1,70}};c.tracks.push_back(std::move(t));}return c;
}
std::string project(const Composition& composition){ProjectState p;p.activeMode=ProjectMode::composer;p.hostStartQuarterBeat=123.5;p.composition=composition;auto encoded=encodeProjectJson(p);check(encoded);return encoded.json;}
MidiResult waitFor(ComposerMidiExport& worker){for(int i=0;i<2000;++i){if(auto result=worker.takeResult())return std::move(*result);juce::Thread::sleep(5);}check(false);return {};}
juce::MidiFile decode(const MidiResult& result){check(result&&result.bytes.size()<=jsonDetail::maximumBytes);juce::MemoryInputStream input(result.bytes.data(),result.bytes.size(),false);juce::MidiFile midi;int type=0;check(midi.readFrom(input,false,&type)&&type==1&&midi.getTimeFormat()==480);return midi;}
void snapshotAndRecovery(){
 ComposerMidiExport worker;auto original=project(fixture());check(!worker.busy()&&!worker.takeResult());
 auto processor=std::make_unique<Processor>();check(processor->restoreStateInformation(original.data(),int(original.size())));auto captured=processor->presentation(UINT64_MAX);check(captured&&captured->composer&&worker.request(std::move(captured->json)));check(worker.busy()&&!worker.request("{}"));
 check(processor->restoreStateInformation(legacy.data(),int(legacy.size())));check(processor->edit(processor->snapshot().revision,"bass",0,39,0,4));processor.reset();
 original="{}"; // Changing the caller's project after clicking cannot change the captured MIDI.
 auto result=waitFor(worker);check(!worker.busy()&&!worker.takeResult());auto midi=decode(result);check(midi.getNumTracks()==17);
 int tempos=0,meters=0,keys=0,sections=0;for(auto* event:*midi.getTrack(0)){const auto& m=event->message;
  if(m.isTempoMetaEvent()){check((m.getTimeStamp()==0&&m.getTempoSecondsPerQuarterNote()==0.5)||(m.getTimeStamp()==1920&&m.getTempoSecondsPerQuarterNote()==0.4));++tempos;}
  if(m.isTimeSignatureMetaEvent()){int n=0,d=0;m.getTimeSignatureInfo(n,d);check((m.getTimeStamp()==0&&n==4&&d==4)||(m.getTimeStamp()==1920&&n==3&&d==4));++meters;}
  if(m.isKeySignatureMetaEvent())++keys;if(m.getMetaEventType()==6)++sections;
 }check(tempos==2&&meters==2&&keys==2&&sections==2);
 for(int ch=1;ch<=16;++ch){int ons=0,offs=0,controls=0;check(midi.getTrack(ch)->getEndTime()==3840);
  for(auto* event:*midi.getTrack(ch)){const auto& m=event->message;if(!m.isMetaEvent())check(m.getChannel()==ch);
   if(m.isNoteOn()){check(m.getNoteNumber()==35+ch&&m.getTimeStamp()==(ch-1)*120);++ons;}
   if(m.isNoteOff()){check(m.getTimeStamp()==(ch-1)*120+240);++offs;}
   if(m.isController()&&m.getControllerNumber()==1){check(m.getTimeStamp()==1200&&m.getControllerValue()==70);++controls;}
  }check(ons==1&&offs==1&&controls==1);
 }
 auto file=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("loopforge-composer-export",".mid");
 check(projectFiles::replaceChecked(file,{reinterpret_cast<const char*>(result.bytes.data()),result.bytes.size()}).isEmpty());juce::MemoryBlock bytes;check(file.loadFileAsData(bytes)&&bytes.getSize()==result.bytes.size()&&std::memcmp(bytes.getData(),result.bytes.data(),result.bytes.size())==0);check(file.deleteFile());
 for(const std::string bad:{"{}","{\"version\":2}","not JSON"}){check(worker.request(bad));auto rejected=waitFor(worker);check(!rejected&&rejected.bytes.empty()&&!rejected.error.empty()&&!worker.busy());}
 check(!worker.request("")&&!worker.request(std::string(jsonDetail::maximumBytes+1,' '))&&!worker.busy());
 const auto valid=project(fixture());check(worker.request(valid));for(int i=0;i<100&&worker.busy();++i)juce::Thread::sleep(1);
 check(!worker.request("{}")); // A completed but unconsumed result still owns the sole job slot.
 auto recovered=waitFor(worker);check(recovered&&recovered.bytes==result.bytes);
 auto inactive=decodeProjectJson(valid);check(inactive.project);inactive.project->activeMode=ProjectMode::legacy;inactive.project->legacyStateJson=legacy;auto encoded=encodeProjectJson(*inactive.project);check(encoded&&worker.request(encoded.json));check(!waitFor(worker));
 check(worker.request(valid)&&waitFor(worker));
}
void closeWithOutstandingWork(){
 auto dense=fixture();dense.tracks.resize(1);dense.tracks[0].notes.clear();dense.tracks[0].controls.clear();for(size_t i=0;i<131072;++i)dense.tracks[0].notes.push_back({1000+i,60,90,{0},{3840}});const auto text=project(dense);
 for(int i=0;i<4;++i){auto worker=std::make_unique<ComposerMidiExport>();check(worker->request(text));juce::Thread::sleep(1);check(worker->busy());worker.reset();}
}
int main(){juce::ScopedJuceInitialiser_GUI init;snapshotAndRecovery();closeWithOutstandingWork();std::cout<<"Composer export worker: bounded snapshots,17track/channel16/maps/CC MIDI, single ready job, error recovery, checked save and close during work passed\n";}
