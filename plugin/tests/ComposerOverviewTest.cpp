#include "../Source/Processor.h"
#include "../Source/ComposerOverview.h"
#include "../Source/ProjectFiles.h"
#include <iostream>
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
using namespace composer;
Composition fixture(){
 Composition c;c.id="overview";c.seed=4294967295;c.style="ambient";c.source="native";c.endTick={3840};
 c.tempos={{{0},500000},{{1920},400000}};c.meters={{{0},4,4},{{1920},3,4}};c.keys={{{0},0,Scale::major},{{1920},2,Scale::naturalMinor}};
 c.sections={{300,"Intro",{0},{1920},SectionRole::intro},{301,"Outro - Финал",{1920},{3840},SectionRole::outro}};
 for(int i=0;i<16;++i){Track t;t.id=i+1;t.name=i==15?"Трек 16":"Track "+std::to_string(i+1);t.channel=i+1;t.role=i==9?Role::drums:Role::lead;if(i!=9)t.program=i;t.notes={{i==15?UINT64_MAX:Id(100+i),36+i,90,{i*120},{240}}};t.controls={{Id(200+i),{0},1,70}};c.tracks.push_back(std::move(t));}return c;
}
std::string project(const Composition& c){ProjectState p;p.activeMode=ProjectMode::composer;p.hostStartQuarterBeat=7.125;p.composition=c;auto json=encodeProjectJson(p);check(json);return json.json;}
std::string saved(Processor& p){juce::MemoryBlock b;p.getStateInformation(b);return {static_cast<const char*>(b.getData()),b.getSize()};}
juce::Viewport* viewport(juce::Component& editor){for(int i=0;i<editor.getNumChildComponents();++i)if(auto* v=dynamic_cast<juce::Viewport*>(editor.getChildComponent(i)))return v;return nullptr;}
juce::TextButton* button(juce::Component& editor,const char* text){for(int i=0;i<editor.getNumChildComponents();++i)if(auto* b=dynamic_cast<juce::TextButton*>(editor.getChildComponent(i)))if(b->getButtonText()==text)return b;return nullptr;}
juce::String details(juce::Component& view){for(int i=0;i<view.getNumChildComponents();++i)if(auto* t=dynamic_cast<juce::TextEditor*>(view.getChildComponent(i))){check(t->isReadOnly());return t->getText();}return {};}
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;auto owner=std::make_unique<Processor>();auto& p=*owner;
 auto first=p.presentation(UINT64_MAX);check(first&&!first->composer&&!first->hasState&&first->json.empty());check(!p.presentation(first->revision));
 auto c=fixture();const auto json=project(c);check(p.restoreStateInformation(json.data(),int(json.size())));auto next=p.presentation(first->revision);check(next&&next->composer&&next->hasState&&next->json==json);check(!p.presentation(next->revision));
 check(!p.restoreStateInformation("{}",2)&&saved(p)==json);check(!p.presentation(next->revision));
 auto root=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("loopforge-overview-"+juce::Uuid().toString());check(root.createDirectory());auto file=root.getChildFile("project.json");
 check(projectFiles::write(file,json).isEmpty());auto loaded=projectFiles::read(file);check(loaded.error.isEmpty()&&loaded.text==json);check(p.restoreStateInformation(loaded.text.data(),int(loaded.text.size())));
 check(projectFiles::write(file,"{}").isNotEmpty()&&file.loadFileAsString().toStdString()==json);check(projectFiles::read(root.getChildFile("absent.json")).error.isNotEmpty());
 auto oversized=root.getChildFile("large.json");{juce::FileOutputStream out(oversized);check(out.openedOk()&&out.setPosition(32*1024*1024));check(out.writeByte(' '));}check(projectFiles::read(oversized).error.isNotEmpty());
 check(projectFiles::write(root,json).isNotEmpty());auto broken=root.getChildFile("broken.json");check(broken.replaceWithText("{\"version\":2"));loaded=projectFiles::read(broken);check(loaded.error.isEmpty()&&!p.restoreStateInformation(loaded.text.data(),int(loaded.text.size()))&&saved(p)==json);
 check(p.restoreStateInformation(json.data(),int(json.size())));
 std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());auto* vp=viewport(*editor);check(vp&&dynamic_cast<ComposerOverview*>(vp->getViewedComponent()));check(vp->getViewedComponent()->getWidth()==vp->getMaximumVisibleWidth());
 check(vp->getViewedComponent()->keyPressed(juce::KeyPress(juce::KeyPress::endKey)));check(vp->getViewPositionY()>0);check(vp->getViewedComponent()->keyPressed(juce::KeyPress(juce::KeyPress::homeKey)));check(vp->getViewPositionY()==0&&saved(p)==json);
 auto text=details(*vp->getViewedComponent());check(text.contains("Трек 16")&&text.contains("channel 16")&&text.contains("drums")&&text.contains("3/4")&&text.contains("natural minor")&&text.contains("Финал"));
 check(vp->getViewedComponent()->getDescription().contains("4294967295")&&vp->getViewedComponent()->getDescription().contains("7.125"));
 check(button(*editor,"OPEN PROJECT")&&button(*editor,"OPEN PROJECT")->isEnabled());check(button(*editor,"SAVE PROJECT")&&button(*editor,"SAVE PROJECT")->isEnabled());check(!button(*editor,"EXPORT MIDI")->isEnabled());check(!button(*editor,"GENERATE 8 IDEAS")->isVisible()&&vp->getHeight()>editor->getHeight()/2);
 if(argc>1){auto image=editor->createComponentSnapshot(editor->getLocalBounds());juce::FileOutputStream out{juce::File(argv[1])};check(out.openedOk());juce::PNGImageFormat png;check(png.writeImageToStream(image,out));}
 editor->setSize(1600,1100);check(vp->getViewedComponent()->getWidth()==vp->getMaximumVisibleWidth());
 if(argc>2){auto image=editor->createComponentSnapshot(editor->getLocalBounds());juce::FileOutputStream out{juce::File(argv[2])};juce::PNGImageFormat png;check(out.openedOk()&&png.writeImageToStream(image,out));}
 if(argc>3){check(vp->getViewedComponent()->keyPressed(juce::KeyPress(juce::KeyPress::endKey)));auto image=editor->createComponentSnapshot(editor->getLocalBounds());juce::FileOutputStream out{juce::File(argv[3])};juce::PNGImageFormat png;check(out.openedOk()&&png.writeImageToStream(image,out));}
 const std::string legacy=R"({"version":1,"idea":{"metadata":{"bars":4,"bpm":120},"layers":{"bass":[{"pitch":38,"start":0.123,"duration":1,"velocity":90}],"chords":[],"melody":[]}}})";
 check(p.restoreStateInformation(legacy.data(),int(legacy.size())));check(projectFiles::write(file,saved(p)).isEmpty()&&projectFiles::read(file).text==legacy);
 juce::Timer::callAfterDelay(260,[&]{check(viewport(*editor)->getViewedComponent()->getName()=="Piano roll");check(button(*editor,"EXPORT MIDI")->isEnabled());juce::MessageManager::getInstance()->stopDispatchLoop();});juce::MessageManager::getInstance()->runDispatchLoop();
 // A note covering the whole short document reaches the final visible pixel column.
 {auto shortSong=fixture();shortSong.endTick={480};shortSong.tempos.resize(1);shortSong.meters.resize(1);shortSong.keys.resize(1);shortSong.sections.clear();shortSong.tracks.resize(1);shortSong.tracks[0].notes={{100,60,90,{0},{480}}};
  ComposerOverview view;check(view.setDocument(shortSong,0));view.setSize(1200,view.requiredHeight());auto image=view.createComponentSnapshot(view.getLocalBounds());check(image.getPixelAt(400,170)==image.getPixelAt(1185,170));
  shortSong.tracks[0].notes[0].duration={240};check(view.setDocument(shortSong,0));image=view.createComponentSnapshot(view.getLocalBounds());check(image.getPixelAt(400,170)!=image.getPixelAt(1000,170));check(image.getPixelAt(689,170)==image.getPixelAt(1000,170));
 }
 // Maximum fully-overlapping source must aggregate without scanning every note on paint.
 c=fixture();c.tracks.resize(1);c.tracks[0].notes.clear();c.tracks[0].controls.clear();
 for(size_t i=0;i<131072;++i)c.tracks[0].notes.push_back({Id(i+1000),60,90,{0},{3840}});
 for(size_t i=0;i<65536;++i)c.tracks[0].controls.push_back({Id(i+200000),{0},1,int(i%128)});
 check(validate(c));ComposerOverview dense;check(dense.setDocument(c,0));dense.setSize(1200,dense.requiredHeight());auto image=dense.createComponentSnapshot(dense.getLocalBounds());check(image.isValid());
 // Dense real host block overflows and remains paused; status must explain how to resume.
 c=fixture();c.tracks.resize(2);c.tracks[0].notes.clear();c.tracks[1].notes.clear();c.tracks[0].controls.clear();c.tracks[1].controls.clear();Id id=1000;
 for(auto& t:c.tracks)for(int tick=0;tick<12;tick+=2)for(int pitch=0;pitch<128;++pitch)t.notes.push_back({id++,pitch,90,{tick},{1}});
 auto overflow=project(c);check(p.restoreStateInformation(overflow.data(),int(overflow.size())));
 struct Host:juce::AudioPlayHead{double beat=7.125;bool playing=true;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setPpqPosition(beat);p.setBpm(1000);p.setIsPlaying(playing);return p;}}host;
 p.setPlayHead(&host);p.prepareToPlay(48000,64);juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.ensureSize(32768);p.processBlock(audio,midi);p.processBlock(audio,midi);
 check(p.getStatus().contains("stop/start")&&p.getStatus().contains("seek"));host.beat+=64*1000/(60*48000.0);p.processBlock(audio,midi);check(midi.isEmpty());host.playing=false;p.processBlock(audio,midi);check(!p.getStatus().contains("limit reached"));p.setPlayHead(nullptr);
 editor.reset();root.deleteRecursively();std::cout<<"Composer overview, bounded files, mode refresh and overflow guidance passed\n";
}
