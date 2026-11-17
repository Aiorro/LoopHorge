#include "../Source/Processor.h"
#include "../Source/ComposerOverview.h"
#include "../Source/ComposerMoodControls.h"
#include "../Source/ProjectFiles.h"
#include <iostream>
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
struct HostListener:juce::AudioProcessorListener{
 int changes=0;void audioProcessorParameterChanged(juce::AudioProcessor*,int,float)override{}
 void audioProcessorChanged(juce::AudioProcessor*,const ChangeDetails& details)override{check(juce::MessageManager::getInstance()->isThisTheMessageThread());if(details.nonParameterStateChanged)++changes;}
};
struct Flow:juce::Timer{
 std::unique_ptr<Processor> p=std::make_unique<Processor>();std::unique_ptr<juce::AudioProcessorEditor> editor{p->createEditor()};HostListener listener;
 int phase=0,ticks=0,dirtyBeforeClose=0;juce::TextButton *mode=nullptr,*compose=nullptr;juce::TextEditor *seed=nullptr,*direction=nullptr;std::string first;juce::Viewport* vp=nullptr;
 Flow(){
  {auto file=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("recipe-limit",".json");check(file.replaceWithText(juce::String::repeatedString("a",32769)));check(projectFiles::read(file,32768).text.empty()&&projectFiles::read(file,32768).error.isNotEmpty());check(file.deleteFile());}
  {ComposerMoodControls restored;composer::ManualOverrides manual;manual.values.mood.energy=17;manual.pinned.set(size_t(composer::Field::energy));restored.apply(manual);check(restored.overrides().pinned==manual.pinned&&restored.overrides().values.mood.energy==17);}
  {ComposerMoodControls controls;controls.setSize(952,400);check(controls.overrides().pinned.none());
   const composer::Field fields[]{composer::Field::darkness,composer::Field::energy,composer::Field::melancholy,composer::Field::tension,composer::Field::aggression,composer::Field::atmosphere,composer::Field::complexity};
   for(size_t i=0;i<7;++i){auto* automatic=dynamic_cast<juce::ToggleButton*>(controls.getChildComponent(int(i*3+1)));auto* value=dynamic_cast<juce::Slider*>(controls.getChildComponent(int(i*3+2)));check(automatic&&value&&automatic->getToggleState()&&!value->isEnabled());
    automatic->setToggleState(false,juce::sendNotificationSync);value->setValue(int(i*10),juce::sendNotificationSync);auto captured=controls.overrides();check(captured.pinned.test(size_t(fields[i]))&&composer::fieldValue(captured.values,fields[i])==std::to_string(i*10));check(value->isEnabled());
    automatic->setToggleState(true,juce::sendNotificationSync);check(!controls.overrides().pinned.test(size_t(fields[i]))&&!value->isEnabled());
   }
  }
static_cast<juce::AudioProcessor&>(*p).addListener(&listener);for(int i=0;i<editor->getNumChildComponents();++i){auto* c=editor->getChildComponent(i);if(c->getName()=="Composer mode")mode=dynamic_cast<juce::TextButton*>(c);if(c->getName()=="Compose offline loop")compose=dynamic_cast<juce::TextButton*>(c);if(c->getName()=="Composer fallback seed")seed=dynamic_cast<juce::TextEditor*>(c);if(c->getName()=="Musical direction")direction=dynamic_cast<juce::TextEditor*>(c);if(auto* v=dynamic_cast<juce::Viewport*>(c))vp=v;}
  check(mode&&compose&&seed&&direction&&vp);check(mode->getButtonText()=="COMPOSE LOOP"&&mode->isEnabled()&&!compose->isVisible());
  direction->setText("4 bars ambient bass chords lead drums");
  // Real insertion exercises JUCE filters; setText alone bypasses them.
  for(const auto& invalid:juce::StringArray{"-1","1.5","10000000000","12x","4294967296"}){seed->selectAll();seed->insertTextAtCaret(invalid);mode->onClick();check(!p->busy&&!p->isComposerMode());check(seed->getText()==invalid);}
  seed->setText("42");
  auto prefix=direction->getText();const juce::String oversized=prefix+juce::String::repeatedString(" ",4096-prefix.length())+"3 minutes song";
  for(const auto& invalid:juce::StringArray{oversized,juce::String::repeatedString(juce::String::fromUTF8("я"),2050)}){direction->selectAll();direction->insertTextAtCaret(invalid);mode->onClick();check(!p->busy&&!p->isComposerMode());check(direction->getText()==invalid);check(p->getStatus().contains("4096"));}
  direction->setText(prefix);seed->setText("4294967296");mode->onClick();check(!p->busy&&!p->isComposerMode());seed->clear();mode->onClick();check(!p->busy);seed->setText("42");mode->onClick();startTimer(50);
 }
 ~Flow(){static_cast<juce::AudioProcessor&>(*p).removeListener(&listener);}
 void snapshot(const char* path){auto image=editor->createComponentSnapshot(editor->getLocalBounds());juce::MemoryOutputStream out;juce::PNGImageFormat png;check(png.writeImageToStream(image,out)&&juce::File(path).replaceWithData(out.getData(),out.getDataSize()));}
 void timerCallback()override{
  check(++ticks<400);if(p->busy||ticks%8)return;
  if(phase==0){check(p->isComposerMode()&&dynamic_cast<ComposerOverview*>(vp->getViewedComponent()));check(compose->isVisible()&&compose->isEnabled()&&direction->isVisible()&&direction->isEnabled());check(mode->getButtonText()=="BACK TO LEGACY");check(listener.changes>0);
   juce::TextButton* settings=nullptr;for(int i=0;i<editor->getNumChildComponents();++i)if(editor->getChildComponent(i)->getName()=="Composer mood settings")settings=dynamic_cast<juce::TextButton*>(editor->getChildComponent(i));check(settings&&settings->isVisible());settings->onClick();check(!vp->isVisible());int recipeButtons=0;for(int i=0;i<editor->getNumChildComponents();++i)if(auto* b=dynamic_cast<juce::TextButton*>(editor->getChildComponent(i)))if(b->getButtonText()=="OPEN RECIPE"||b->getButtonText()=="SAVE RECIPE"){check(b->isVisible()&&b->getBottom()<editor->getHeight()-60);++recipeButtons;}check(recipeButtons==2);snapshot("/tmp/loopforge-mood-panel-min.png");settings->onClick();check(vp->isVisible());
   bool readable=false;for(int i=0;i<editor->getNumChildComponents();++i)if(auto* text=dynamic_cast<juce::TextEditor*>(editor->getChildComponent(i)))if(text->getName()=="Resolved Composer settings")readable=text->isReadOnly()&&text->getText().contains("seed=42 [default]");check(readable);
   check(vp->getY()>=compose->getBottom()&&seed->getRight()<=mode->getX());snapshot("/tmp/loopforge-compose-ui-min.png");editor->setSize(1600,1100);snapshot("/tmp/loopforge-compose-ui-large.png");editor->setSize(1000,740);mode->onClick();phase=1;
  }else if(phase==1){check(!p->isComposerMode()&&mode->getButtonText()=="VIEW COMPOSER");check(!compose->isVisible());mode->onClick();phase=2;
  }else if(phase==2){check(p->isComposerMode()&&compose->isVisible());seed->setText("43");ComposerMoodControls* panel=nullptr;for(int i=0;i<editor->getNumChildComponents();++i)if(auto* c=dynamic_cast<ComposerMoodControls*>(editor->getChildComponent(i)))panel=c;check(panel);
   auto* automatic=dynamic_cast<juce::ToggleButton*>(panel->getChildComponent(4));auto* energy=dynamic_cast<juce::Slider*>(panel->getChildComponent(5));check(automatic&&energy);automatic->setToggleState(false,juce::sendNotificationSync);energy->setValue(0);direction->setText("4 bars energy100");compose->onClick();energy->setValue(100);direction->setText("3 minutes song");phase=3;
  }else if(phase==3){check(p->isComposerMode());auto presentation=p->presentation(UINT64_MAX);check(presentation);first=presentation->json;check(composer::decodeProjectJson(first).project->composition->seed==43);check(p->composerSummary().contains("energy=0 [manual]"));compose->onClick();phase=4;
  }else if(phase==4){check(p->presentation(UINT64_MAX)->json==first&&p->getStatus().contains("rejected"));direction->setText("32 bars ambient bass chords lead drums");dirtyBeforeClose=listener.changes;compose->onClick();editor.reset();phase=5;
  }else{check(p->isComposerMode()&&p->presentation(UINT64_MAX)->json!=first&&listener.changes>dirtyBeforeClose);stopTimer();juce::MessageManager::getInstance()->stopDispatchLoop();}
 }
};
int main(){juce::ScopedJuceInitialiser_GUI init;Flow flow;juce::MessageManager::getInstance()->runDispatchLoop();std::cout<<"Native Composer real-editor controls, seed validation, mode cycle, snapshots and close-during-work passed\n";}
