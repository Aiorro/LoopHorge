#include "../Source/Processor.h"
#include "../Source/ComposerOverview.h"
#include <iostream>
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
struct HostListener:juce::AudioProcessorListener{
 int changes=0;void audioProcessorParameterChanged(juce::AudioProcessor*,int,float)override{}
 void audioProcessorChanged(juce::AudioProcessor*,const ChangeDetails& details)override{check(juce::MessageManager::getInstance()->isThisTheMessageThread());if(details.nonParameterStateChanged)++changes;}
};
struct Flow:juce::Timer{
 std::unique_ptr<Processor> p=std::make_unique<Processor>();std::unique_ptr<juce::AudioProcessorEditor> editor{p->createEditor()};HostListener listener;
 int phase=0,ticks=0,dirtyBeforeClose=0;juce::TextButton *mode=nullptr,*compose=nullptr;juce::TextEditor *seed=nullptr,*direction=nullptr;std::string first;juce::Viewport* vp=nullptr;
 Flow(){static_cast<juce::AudioProcessor&>(*p).addListener(&listener);for(int i=0;i<editor->getNumChildComponents();++i){auto* c=editor->getChildComponent(i);if(c->getName()=="Composer mode")mode=dynamic_cast<juce::TextButton*>(c);if(c->getName()=="Compose offline loop")compose=dynamic_cast<juce::TextButton*>(c);if(c->getName()=="Composer fallback seed")seed=dynamic_cast<juce::TextEditor*>(c);if(c->getName()=="Musical direction")direction=dynamic_cast<juce::TextEditor*>(c);if(auto* v=dynamic_cast<juce::Viewport*>(c))vp=v;}
  check(mode&&compose&&seed&&direction&&vp);check(mode->getButtonText()=="COMPOSE LOOP"&&mode->isEnabled()&&!compose->isVisible());
  direction->setText("4 bars ambient bass chords lead drums");seed->setText("4294967296");mode->onClick();check(!p->busy&&!p->isComposerMode());seed->clear();mode->onClick();check(!p->busy);seed->setText("42");mode->onClick();startTimer(50);
 }
 ~Flow(){static_cast<juce::AudioProcessor&>(*p).removeListener(&listener);}
 void snapshot(const char* path){auto image=editor->createComponentSnapshot(editor->getLocalBounds());juce::MemoryOutputStream out;juce::PNGImageFormat png;check(png.writeImageToStream(image,out)&&juce::File(path).replaceWithData(out.getData(),out.getDataSize()));}
 void timerCallback()override{
  check(++ticks<400);if(p->busy||ticks%8)return;
  if(phase==0){check(p->isComposerMode()&&dynamic_cast<ComposerOverview*>(vp->getViewedComponent()));check(compose->isVisible()&&compose->isEnabled()&&direction->isVisible()&&direction->isEnabled());check(mode->getButtonText()=="BACK TO LEGACY");check(listener.changes>0);
   bool readable=false;for(int i=0;i<editor->getNumChildComponents();++i)if(auto* text=dynamic_cast<juce::TextEditor*>(editor->getChildComponent(i)))if(text->getName()=="Resolved Composer settings")readable=text->isReadOnly()&&text->getText().contains("seed=42 [default]");check(readable);
   check(vp->getY()>=compose->getBottom()&&seed->getRight()<=mode->getX());snapshot("/tmp/loopforge-compose-ui-min.png");editor->setSize(1600,1100);snapshot("/tmp/loopforge-compose-ui-large.png");editor->setSize(1000,740);mode->onClick();phase=1;
  }else if(phase==1){check(!p->isComposerMode()&&mode->getButtonText()=="VIEW COMPOSER");check(!compose->isVisible());mode->onClick();phase=2;
  }else if(phase==2){check(p->isComposerMode()&&compose->isVisible());seed->setText("43");compose->onClick();direction->setText("3 minutes song");phase=3;
  }else if(phase==3){check(p->isComposerMode());auto presentation=p->presentation(UINT64_MAX);check(presentation);first=presentation->json;check(composer::decodeProjectJson(first).project->composition->seed==43);compose->onClick();phase=4;
  }else if(phase==4){check(p->presentation(UINT64_MAX)->json==first&&p->getStatus().contains("rejected"));direction->setText("32 bars ambient bass chords lead drums");dirtyBeforeClose=listener.changes;compose->onClick();editor.reset();phase=5;
  }else{check(p->isComposerMode()&&p->presentation(UINT64_MAX)->json!=first&&listener.changes>dirtyBeforeClose);stopTimer();juce::MessageManager::getInstance()->stopDispatchLoop();}
 }
};
int main(){juce::ScopedJuceInitialiser_GUI init;Flow flow;juce::MessageManager::getInstance()->runDispatchLoop();std::cout<<"Native Composer real-editor controls, seed validation, mode cycle, snapshots and close-during-work passed\n";}
