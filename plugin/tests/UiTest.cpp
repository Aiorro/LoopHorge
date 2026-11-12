#include "../Source/Processor.h"
#include <iostream>
#define check(value) do{if(!(value)){std::cerr<<"UI check at "<<__LINE__<<"\n";std::abort();}}while(false)
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI init;Processor p;std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
 juce::TextButton *generate=nullptr,*probe=nullptr,*cancel=nullptr;juce::ComboBox *provider=nullptr,*variation=nullptr;juce::TextEditor* model=nullptr;
 for(int i=0;i<editor->getNumChildComponents();++i){auto* c=editor->getChildComponent(i);check(editor->getLocalBounds().contains(c->getBounds()));
  if(auto* b=dynamic_cast<juce::TextButton*>(c)){if(b->getButtonText()=="GENERATE 8 IDEAS")generate=b;if(b->getButtonText()=="TEST (1 IDEA)")probe=b;if(b->getButtonText()=="CANCEL REQUEST")cancel=b;}
  if(auto* box=dynamic_cast<juce::ComboBox*>(c)){if(box->getText()=="offline")provider=box;if(box->getNumItems()==0)variation=box;}
  if(auto* text=dynamic_cast<juce::TextEditor*>(c))if(!text->isMultiLine())model=text;
 }
 check(generate&&probe&&cancel&&provider&&model&&variation);check(!model->isEnabled());check(probe->getTooltip().contains("charges"));
 provider->setSelectedId(2,juce::sendNotificationSync);check(model->isEnabled());model->setText("test-model");provider->setSelectedId(1,juce::sendNotificationSync);check(!model->isEnabled()&&model->getText().isEmpty());
 generate->onClick();for(int i=0;i<100&&p.busy;++i)juce::Thread::sleep(10);check(!p.busy&&p.ideaCount()==8);auto before=juce::JSON::toString(p.getIdea());
 probe->onClick();for(int i=0;i<100&&p.busy;++i)juce::Thread::sleep(10);check(!p.busy&&p.getStatus().contains("ready"));check(before==juce::JSON::toString(p.getIdea()));cancel->onClick();

 bool selectionOkay=true;
 juce::Timer::callAfterDelay(250,[&]{
  check(variation->getNumItems()==8);variation->setSelectedId(4,juce::sendNotificationSync);generate->onClick();
  juce::Timer::callAfterDelay(400,[&]{
   check(!p.busy);if(variation->getSelectedItemIndex()!=0){std::cerr<<"selector disagrees with regenerated idea\n";selectionOkay=false;}
   juce::MemoryBlock state;p.getStateInformation(state);p.setStateInformation(state.getData(),int(state.getSize()));
   juce::Timer::callAfterDelay(300,[&]{if(variation->getNumItems()!=1||variation->getSelectedItemIndex()!=0){std::cerr<<"selector disagrees with restored state\n";selectionOkay=false;}juce::MessageManager::getInstance()->stopDispatchLoop();});
  });
 });
 juce::MessageManager::getInstance()->runDispatchLoop();if(!selectionOkay)return 1;

 if(argc>1){auto image=editor->createComponentSnapshot(editor->getLocalBounds());check(image.isValid());juce::FileOutputStream out{juce::File(argv[1])};check(out.openedOk());juce::PNGImageFormat format;check(format.writeImageToStream(image,out));}
 std::cout<<"native model/probe/cancel controls and offline generate handlers passed\n";
}
