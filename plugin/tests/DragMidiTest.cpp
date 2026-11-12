#include "../Source/DragMidi.h"
#include "../Source/Offline.h"
#include <iostream>
#include <thread>
#include <barrier>
#include <atomic>
#define check(x) do{if(!(x)){std::cerr<<"drag MIDI assertion "<<__LINE__<<'\n';std::abort();}}while(false)
int main(){auto root=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("loopforge-test-"+juce::Uuid().toString());auto now=juce::Time::currentTimeMillis();DragMidiCache cache(root);
 check(!cache.prepare({},now).lease);auto idea=offline::generate(juce::JSON::parse("{\"seed\":42}"))[0];auto r=cache.prepare(idea,now);check(r.lease&&r.lease->file.existsAsFile());auto path=r.lease->file;{juce::FileInputStream input(path);juce::MidiFile midi;int type=0;check(midi.readFrom(input,false,&type)&&type==1&&midi.getNumTracks()==4&&midi.getTimeFormat()==480);check(midi.getTrack(0)->getEndTime()==7680);}
 auto future=now+25*60*60*1000LL;auto other=cache.prepare(idea,future);check(other.lease&&path.existsAsFile());r.lease.reset();auto next=cache.prepare(idea,future);check(next.lease&&!path.existsAsFile());auto retained=other.lease->file;other.lease.reset();check(retained.existsAsFile());
 auto unknown=root.getChildFile("user.mid");unknown.replaceWithText("keep");unknown.setLastModificationTime(juce::Time(1));check(cache.prepare(idea,now).lease&&unknown.existsAsFile());
 auto full=root.getSiblingFile(root.getFileName()+"-full");full.createDirectory();for(int i=0;i<256;++i)full.getChildFile(juce::Uuid().toString()+".mid").replaceWithText("x");check(!DragMidiCache(full).prepare(idea,now).lease);

 auto concurrent=root.getSiblingFile(root.getFileName()+"-concurrent");concurrent.createDirectory();for(int i=0;i<255;++i)concurrent.getChildFile(juce::Uuid().toString()+".mid").replaceWithText("x");std::barrier gate(8);std::atomic<int> success=0;std::vector<std::thread> workers;for(int i=0;i<8;++i)workers.emplace_back([&]{gate.arrive_and_wait();if(DragMidiCache(concurrent).prepare(idea,now).lease)++success;});for(auto& t:workers)t.join();check(success==1);concurrent.deleteRecursively();
 auto large=root.getSiblingFile(root.getFileName()+"-large");large.createDirectory();{juce::FileOutputStream output(large.getChildFile(juce::Uuid().toString()+".mid"));check(output.openedOk()&&output.setPosition(16*1024*1024-1));output.writeByte(0);}check(!DragMidiCache(large).prepare(idea,now).lease);large.deleteRecursively();
#if !JUCE_WINDOWS
 auto outside=root.getSiblingFile(root.getFileName()+"-outside");outside.replaceWithText("keep");auto link=root.getChildFile(juce::Uuid().toString()+".mid");std::filesystem::create_symlink(outside.getFullPathName().toStdString(),link.getFullPathName().toStdString());check(cache.prepare(idea,future).lease&&link.isSymbolicLink()&&outside.loadFileAsString()=="keep");auto linked=root.getSiblingFile(root.getFileName()+"-link");std::filesystem::create_directory_symlink(root.getFullPathName().toStdString(),linked.getFullPathName().toStdString());check(!DragMidiCache(linked).prepare(idea,now).lease);linked.deleteFile();outside.deleteFile();
#endif
 auto bad=root.getSiblingFile(root.getFileName()+"-file");bad.replaceWithText("not a directory");check(!DragMidiCache(bad).prepare(idea,now).lease);bad.deleteFile();next.lease.reset();root.deleteRecursively();full.deleteRecursively();std::cout<<"MIDI drag cache decode, retention, active lease and bounds passed\n";}
