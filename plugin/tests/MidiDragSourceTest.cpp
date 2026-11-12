#include "../Source/MidiDragSource.h"
#include "../Source/Offline.h"
#include <iostream>
#include <thread>
#define check(x) do{if(!(x)){std::cerr<<"drag gesture assertion "<<__LINE__<<'\n';std::abort();}}while(false)
int main(){juce::ScopedJuceInitialiser_GUI init;auto root=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("loopforge-gesture-"+juce::Uuid().toString());auto idea=offline::generate(juce::JSON::parse("{\"seed\":42}"))[0];auto original=encodeMidi(idea);int calls=0;juce::File file;std::function<void()> complete;juce::String status;

 auto special=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("cache%20literal # space/"+juce::String::charToString(0x4f60)+".mid");auto adapted=MidiDragSource::platformFiles({special.getFullPathName()});
#if JUCE_LINUX
 check(adapted[0].startsWith("file://")&&juce::URL(adapted[0]).getLocalFile()==special);check(adapted[0].contains("%2520")&&adapted[0].contains("%23")&&adapted[0].contains("%20"));
#else
 check(adapted[0]==special.getFullPathName());
#endif
 auto source=std::make_unique<MidiDragSource>([&]{return idea;},DragMidiCache(root),[&](const juce::StringArray& files,bool move,juce::Component*,std::function<void()> done){++calls;check(!move&&files.size()==1);file=juce::File(files[0]);complete=std::move(done);juce::MemoryBlock actual;check(file.loadFileAsData(actual)&&actual==original);return true;},[&](juce::String message){status=message;});source->setSize(110,40);
 auto event=[&](float x,int mods=juce::ModifierKeys::leftButtonModifier){auto* c=source.get();return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),{x,10},juce::ModifierKeys(mods),1,0,0,0,0,c,c,juce::Time::getCurrentTime(),{0,10},juce::Time::getCurrentTime(),1,false);};
 source->mouseDown(event(0));source->mouseDrag(event(5));source->mouseUp(event(5));check(calls==0&&!root.exists());source->mouseDown(event(0));idea["layers"]["bass"][0].getDynamicObject()->setProperty("pitch",127);source->mouseDrag(event(6));source->mouseDrag(event(9));check(calls==1&&file.existsAsFile()&&status.contains("started"));source.reset();check(file.existsAsFile());std::thread finisher([&]{complete();complete={};});finisher.join();check(file.existsAsFile());root.deleteRecursively();

 source=std::make_unique<MidiDragSource>([&]{return idea;},DragMidiCache(root),[&](auto&,bool,auto*,auto){++calls;return false;},[&](auto message){status=message;});source->setSize(110,40);calls=0;source->mouseDown(event(0,juce::ModifierKeys::rightButtonModifier));source->mouseDrag(event(20));check(calls==0&&!root.exists());source.reset();
 auto bad=root.getSiblingFile(root.getFileName()+"-bad");bad.replaceWithText("file");source=std::make_unique<MidiDragSource>([&]{return idea;},DragMidiCache(bad),[&](auto&,bool,auto*,auto){++calls;return true;},[&](auto message){status=message;});source->mouseDown(event(0));source->mouseDrag(event(6));check(calls==0&&status.contains("Export MIDI"));source.reset();bad.deleteFile();
 source=std::make_unique<MidiDragSource>([]{return juce::var{};},DragMidiCache(root),[&](auto&,bool,auto*,auto){++calls;return true;},[&](auto message){status=message;});source->mouseDown(event(0));source->mouseDrag(event(6));check(calls==0&&!root.exists()&&status.contains("valid idea"));source.reset();
 auto failed=std::make_unique<MidiDragSource>([&]{return idea;},DragMidiCache(root),[](auto&,bool,auto*,auto){return false;},[&](auto message){status=message;});source=std::move(failed);source->setSize(110,40);source->mouseDown(event(0));source->mouseDrag(event(6));check(status.contains("Export MIDI"));source.reset();root.deleteRecursively();std::cout<<"MIDI drag snapshot, threshold, copy-only dispatch and delayed completion passed\n";}
