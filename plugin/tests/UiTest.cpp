#include "../Source/Processor.h"
#include "../Source/PianoRoll.h"
#include <iostream>
#define check(value) do{if(!(value)){std::cerr<<"UI check at "<<__LINE__<<"\n";std::abort();}}while(false)
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI init;Processor p;std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
 juce::TextButton *generate=nullptr,*probe=nullptr,*cancel=nullptr,*undoButton=nullptr,*redoButton=nullptr,*quantizeButton=nullptr,*dragMidi=nullptr;juce::ComboBox *provider=nullptr,*variation=nullptr;juce::TextEditor* model=nullptr;
 for(int i=0;i<editor->getNumChildComponents();++i){auto* c=editor->getChildComponent(i);check(editor->getLocalBounds().contains(c->getBounds()));
  if(auto* b=dynamic_cast<juce::TextButton*>(c)){if(b->getButtonText()=="GENERATE 8 IDEAS")generate=b;if(b->getButtonText()=="TEST (1 IDEA)")probe=b;if(b->getButtonText()=="CANCEL REQUEST")cancel=b;if(b->getButtonText()=="DRAG MIDI")dragMidi=b;if(b->getButtonText()=="UNDO")undoButton=b;if(b->getButtonText()=="REDO")redoButton=b;if(b->getButtonText()=="QUANTIZE 1/4 BEAT")quantizeButton=b;}
  if(auto* box=dynamic_cast<juce::ComboBox*>(c)){if(box->getText()=="offline")provider=box;if(box->getNumItems()==0)variation=box;}
  if(auto* text=dynamic_cast<juce::TextEditor*>(c))if(!text->isMultiLine())model=text;
 }
 check(generate&&probe&&cancel&&provider&&model&&variation&&undoButton&&redoButton&&quantizeButton&&dragMidi);check(!dragMidi->isEnabled());check(!model->isEnabled());check(probe->getTooltip().contains("charges"));
 provider->setSelectedId(2,juce::sendNotificationSync);check(model->isEnabled());model->setText("test-model");provider->setSelectedId(1,juce::sendNotificationSync);check(!model->isEnabled()&&model->getText().isEmpty());
 bool boundaryOkay=true,selectionOkay=true;
 juce::Timer::callAfterDelay(100,[&]{
 generate->onClick();for(int i=0;i<100&&p.busy;++i)juce::Thread::sleep(10);check(!p.busy&&p.ideaCount()==8);auto before=juce::JSON::toString(p.getIdea());
 probe->onClick();for(int i=0;i<100&&p.busy;++i)juce::Thread::sleep(10);check(!p.busy&&p.getStatus().contains("ready"));check(before==juce::JSON::toString(p.getIdea()));cancel->onClick();

 juce::Timer::callAfterDelay(250,[&]{
 check(dragMidi->isEnabled());
 bool initialScrollOkay=true;for(int c=0;c<editor->getNumChildComponents();++c)if(auto* v=dynamic_cast<juce::Viewport*>(editor->getChildComponent(c))){int y=(127-int(p.getIdea()["layers"]["bass"][0]["pitch"]))*12;if(y<v->getViewPositionY()||y>=v->getViewPositionY()+v->getViewHeight()){std::cerr<<"first generated layer outside viewport\n";initialScrollOkay=false;}}
 check(provider->getTitle()=="Provider");check(editor->isResizable());editor->setSize(1200,900);for(int c=0;c<editor->getNumChildComponents();++c){auto* component=editor->getChildComponent(c);check(editor->getLocalBounds().contains(component->getBounds()));if(auto* v=dynamic_cast<juce::Viewport*>(component))check(v->getHeight()==468);}editor->setSize(1000,740);
 PianoRoll roll(p);roll.setSize(960,1536);roll.selectLayer("bass");
 int keyboardCount=p.getIdea()["layers"]["bass"].size();check(roll.addNote());check(p.getIdea()["layers"]["bass"].size()==keyboardCount+1);int keyboardPitch=int(p.getIdea()["layers"]["bass"][keyboardCount]["pitch"]);check(!roll.keyPressed(juce::KeyPress(juce::KeyPress::upKey,juce::ModifierKeys::ctrlModifier,0)));check(int(p.getIdea()["layers"]["bass"][keyboardCount]["pitch"])==keyboardPitch);check(roll.keyPressed(juce::KeyPress(juce::KeyPress::upKey)));check(int(p.getIdea()["layers"]["bass"][keyboardCount]["pitch"])==keyboardPitch+1);check(roll.keyPressed(juce::KeyPress(juce::KeyPress::rightKey,juce::ModifierKeys::shiftModifier,0)));check(double(p.getIdea()["layers"]["bass"][keyboardCount]["duration"])==1.25);check(roll.keyPressed(juce::KeyPress(juce::KeyPress::deleteKey)));check(p.getIdea()["layers"]["bass"].size()==keyboardCount);

 juce::MemoryBlock keyboardState;p.getStateInformation(keyboardState);p.setStateInformation(keyboardState.getData(),int(keyboardState.getSize()));auto keyboardReplacement=juce::JSON::toString(p.getIdea());check(roll.keyPressed(juce::KeyPress(juce::KeyPress::deleteKey)));check(juce::JSON::toString(p.getIdea())==keyboardReplacement);
 auto event=[&](float x,float y,juce::ModifierKeys mods=juce::ModifierKeys::leftButtonModifier){return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),{x,y},mods,1,0,0,0,0,&roll,&roll,juce::Time::getCurrentTime(),{x,y},juce::Time::getCurrentTime(),1,false);};
 auto point=[&](double beat,int pitch){return juce::Point<float>(48+float(beat/16*912),float((127-pitch)*12+6));};
 auto a=point(1,120);int notes=p.getIdea()["layers"]["bass"].size();roll.mouseDoubleClick(event(a.x,a.y));check(p.getIdea()["layers"]["bass"].size()==notes+1);check(roll.keyPressed(juce::KeyPress(juce::KeyPress::upKey)));check(int(p.getIdea()["layers"]["bass"][notes]["pitch"])==121);check(roll.keyPressed(juce::KeyPress(juce::KeyPress::downKey)));
 auto b=point(2,121);roll.mouseDown(event(a.x,a.y));roll.mouseDrag(event(b.x,b.y));check(double(p.getIdea()["layers"]["bass"][notes]["start"])==1);roll.mouseUp(event(b.x,b.y));check(double(p.getIdea()["layers"]["bass"][notes]["start"])==2);check(int(p.getIdea()["layers"]["bass"][notes]["pitch"])==121);
 auto edge=point(3,121);auto finish=point(4,121);roll.mouseDown(event(edge.x-1,edge.y));roll.mouseDrag(event(finish.x,finish.y));roll.mouseUp(event(finish.x,finish.y));check(double(p.getIdea()["layers"]["bass"][notes]["duration"])==2);
 roll.mouseDown(event(b.x,b.y,juce::ModifierKeys::rightButtonModifier));check(p.getIdea()["layers"]["bass"].size()==notes);undoButton->onClick();check(p.getIdea()["layers"]["bass"].size()==notes+1);redoButton->onClick();check(p.getIdea()["layers"]["bass"].size()==notes);undoButton->onClick();
 roll.refresh();roll.mouseDown(event(b.x,b.y));p.choose(0);auto changed=juce::JSON::toString(p.getIdea());roll.mouseDrag(event(a.x,a.y));roll.mouseUp(event(a.x,a.y));check(juce::JSON::toString(p.getIdea())==changed);
 boundaryOkay=initialScrollOkay;
 for(int c=0;c<editor->getNumChildComponents();++c)if(auto* v=dynamic_cast<juce::Viewport*>(editor->getChildComponent(c)))if(v->getViewedComponent()->getWidth()!=v->getMaximumVisibleWidth()){std::cerr<<"roll endpoint clipped by scrollbar\n";boundaryOkay=false;}
 auto restoreNote=[&](double start,double duration){auto value=juce::JSON::parse("{\"version\":1,\"idea\":{\"metadata\":{\"bars\":4,\"bpm\":120},\"layers\":{\"bass\":[{\"pitch\":120,\"start\":0,\"duration\":1,\"velocity\":90}],\"chords\":[],\"melody\":[]}}}");value["idea"]["layers"]["bass"][0].getDynamicObject()->setProperty("start",start);value["idea"]["layers"]["bass"][0].getDynamicObject()->setProperty("duration",duration);auto raw=juce::JSON::toString(value);p.setStateInformation(raw.toRawUTF8(),int(raw.getNumBytesAsUTF8()));roll.refresh();};
 restoreNote(1,.001);quantizeButton->onClick();check(double(p.getIdea()["layers"]["bass"][0]["duration"])==.25);undoButton->onClick();check(double(p.getIdea()["layers"]["bass"][0]["duration"])==.001);a=point(1,120);b=point(2,120);roll.mouseDown(event(a.x+.5f,a.y));roll.mouseDrag(event(b.x+.5f,b.y));roll.mouseUp(event(b.x+.5f,b.y));if(double(p.getIdea()["layers"]["bass"][0]["start"])!=2||double(p.getIdea()["layers"]["bass"][0]["duration"])!=.001){std::cerr<<"short note cannot move with original duration\n";boundaryOkay=false;}
 restoreNote(15.99,.01);a=point(15.99,120);roll.mouseDown(event(959.5f,a.y));roll.mouseDrag(event(958,a.y));roll.mouseUp(event(958,a.y));auto last=p.getIdea()["layers"]["bass"][0];if(double(last["duration"])<.25||double(last["start"])+double(last["duration"])>16){std::cerr<<"tail resize violates grid/bounds\n";boundaryOkay=false;}
 generate->onClick();for(int i=0;i<100&&p.busy;++i)juce::Thread::sleep(10);

 juce::Timer::callAfterDelay(250,[&]{
  bool scrolled=false;for(int c=0;c<editor->getNumChildComponents();++c)if(auto* v=dynamic_cast<juce::Viewport*>(editor->getChildComponent(c)))scrolled=v->getViewPositionY()>0;check(scrolled);
  check(variation->getNumItems()==8);variation->setSelectedId(4,juce::sendNotificationSync);generate->onClick();
  juce::Timer::callAfterDelay(400,[&]{
   check(!p.busy);if(variation->getSelectedItemIndex()!=0){std::cerr<<"selector disagrees with regenerated idea\n";selectionOkay=false;}
   juce::MemoryBlock state;p.getStateInformation(state);p.setStateInformation(state.getData(),int(state.getSize()));
   juce::Timer::callAfterDelay(300,[&]{if(variation->getNumItems()!=1||variation->getSelectedItemIndex()!=0){std::cerr<<"selector disagrees with restored state\n";selectionOkay=false;}juce::MessageManager::getInstance()->stopDispatchLoop();});
  });
 });
 });
 });
 juce::MessageManager::getInstance()->runDispatchLoop();if(!selectionOkay||!boundaryOkay)return 1;

 if(argc>1){auto image=editor->createComponentSnapshot(editor->getLocalBounds());check(image.isValid());juce::FileOutputStream out{juce::File(argv[1])};check(out.openedOk());juce::PNGImageFormat format;check(format.writeImageToStream(image,out));}
 std::cout<<"native model/probe/cancel controls and offline generate handlers passed\n";
}
