#pragma once
#include "Processor.h"
// View-only previews are local; the processor accepts one revision-guarded edit on release.
class PianoRoll final:public juce::Component {
public:
 explicit PianoRoll(Processor& p):processor(p){setName("Piano roll");setTitle("Piano roll");setWantsKeyboardFocus(true);refresh();}
 void selectLayer(const juce::String& name){refresh();if(!editing::layer(name)&&!state.idea["layers"][juce::Identifier(name)].isArray())return;activeLayer=name;selected=-1;dragging=false;refresh();}
 bool refresh(){if(state.idea.isObject()&&processor.compositionRevision()==state.revision)return false;auto next=processor.snapshot();bool replaced=next.replacement!=state.replacement;if(next.revision!=state.revision){dragging=false;selected=-1;state=std::move(next);if(bool(state.idea["composer"])&&!state.idea["layers"][juce::Identifier(activeLayer)].isArray()){auto tracks=state.idea["tracks"];activeLayer=tracks.size()?tracks[0]["key"].toString():"bass";}setDescription(notes().size()?"No note selected. Brackets select notes.":"Empty layer. Use Add note to begin.");repaint();}return replaced;}
 int preferredScrollY()const{int pitch=60;auto notes=state.idea["layers"][juce::Identifier(activeLayer)];if(notes.size())pitch=int(notes[0]["pitch"]);return juce::jlimit(0,128*row-300,(127-pitch)*row-130);}
 bool hasSelection()const{return selected>=0&&selected<notes().size();}
 bool addNote(){refresh();if(beats()<=0)return false;auto list=notes();double start=0;int pitch=bool(state.idea["composer"])?int(state.idea["defaults"][juce::Identifier(activeLayer)]):activeLayer=="bass"?36:activeLayer=="melody"?72:60;if(selected>=0&&selected<list.size()){auto n=list[selected];start=juce::jlimit(0.0,beats()-.25,editing::snap(double(n["start"])+double(n["duration"])));pitch=int(n["pitch"]);}auto replacement=state.replacement;bool changed=processor.edit(state.revision,activeLayer,-1,pitch,start,juce::jmin(1.0,beats()-start));afterEdit(changed,list.size(),replacement);return changed;}
 bool deleteSelected(){if(refresh())return false;auto list=notes();if(selected<0||selected>=list.size())return false;int index=selected;auto replacement=state.replacement;bool changed=processor.edit(state.revision,activeLayer,index,0,0,1,true);afterEdit(changed,index,replacement);return changed;}
 bool keyPressed(const juce::KeyPress& key)override{
  if(key.getModifiers().isCtrlDown()||key.getModifiers().isCommandDown()||key.getModifiers().isAltDown())return false;
  const int code=key.getKeyCode();if(code!='['&&code!=']'&&code!=juce::KeyPress::homeKey&&code!=juce::KeyPress::endKey&&code!=juce::KeyPress::upKey&&code!=juce::KeyPress::downKey&&code!=juce::KeyPress::leftKey&&code!=juce::KeyPress::rightKey&&code!=juce::KeyPress::deleteKey&&code!=juce::KeyPress::backspaceKey)return false;
  if(refresh()){reveal();return true;}dragging=false;auto list=notes();if(list.size()==0)return true;if(selected<0){selected=0;reveal();return true;}
  if(code=='['||code==']'||code==juce::KeyPress::homeKey||code==juce::KeyPress::endKey){selected=code==juce::KeyPress::homeKey?0:code==juce::KeyPress::endKey?list.size()-1:juce::jlimit(0,list.size()-1,selected+(code==']'?1:-1));reveal();return true;}
  if(code==juce::KeyPress::deleteKey||code==juce::KeyPress::backspaceKey){deleteSelected();return true;}
  auto n=list[selected];int pitch=n["pitch"];double start=n["start"],duration=n["duration"];
  if(code==juce::KeyPress::upKey||code==juce::KeyPress::downKey)pitch=juce::jlimit(0,127,pitch+(code==juce::KeyPress::upKey?1:-1));
  else if(key.getModifiers().isShiftDown()){start=juce::jmin(start,beats()-.25);duration=juce::jlimit(.25,beats()-start,duration+(code==juce::KeyPress::rightKey?.25:-.25));}
  else start=juce::jlimit(0.0,beats()-duration,start+(code==juce::KeyPress::rightKey?.25:-.25));
  auto replacement=state.replacement;bool changed=processor.edit(state.revision,activeLayer,selected,pitch,start,duration);afterEdit(changed,selected,replacement);return true;
 }
 void focusGained(FocusChangeType)override{refresh();if(selected<0&&notes().size())selected=0;reveal();}
 std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler()override{return std::make_unique<juce::AccessibilityHandler>(*this,juce::AccessibilityRole::group);}
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff242936));double end=beats();if(end<=0)return;
  for(int pitch=0;pitch<128;++pitch){int y=(127-pitch)*row;g.setColour(juce::Colour(pitch%12==0?0xff4c566a:0xff303646));g.drawHorizontalLine(y,0,float(getWidth()));if(pitch%12==0){g.setColour(juce::Colours::lightgrey);g.drawText("C"+juce::String(pitch/12-1),0,y,46,row,juce::Justification::centredRight);}}
  for(double beat=0;beat<=end;beat+=.25){g.setColour(juce::Colour(isBar(beat)?0xff667286:std::fmod(beat,1)==0?0xff424b5c:0xff303646));g.drawVerticalLine(int(xAt(beat)),0,float(getHeight()));}
  juce::StringArray names;if(bool(state.idea["composer"])){for(auto& t:*state.idea["tracks"].getArray())names.add(t["key"].toString());}else names={"bass","chords","melody"};const juce::Colour colours[]={juce::Colour(0xffb494ff),juce::Colour(0xff53d9ba),juce::Colour(0xffffbc70),juce::Colour(0xffef8b9e)};
  for(int ch=0;ch<names.size();++ch){auto list=state.idea["layers"][juce::Identifier(names[ch])];for(int i=0;i<list.size();++i){auto note=dragging&&activeLayer==names[ch]&&i==selected?preview:list[i];auto rect=bounds(note);g.setColour(colours[ch%4].withAlpha(activeLayer==names[ch]?1.0f:.3f));g.fillRoundedRectangle(rect,2);if(i==selected&&activeLayer==names[ch]){g.setColour(juce::Colours::white);g.drawRect(rect.expanded(hasKeyboardFocus(true)?2.0f:0.0f),hasKeyboardFocus(true)?2.0f:1.5f);}}}
 }
 void mouseDoubleClick(const juce::MouseEvent& e)override{refresh();if(e.x<48||hit(e.position)>=0||beats()<=0)return;int index=notes().size();auto replacement=state.replacement;bool changed=processor.edit(state.revision,activeLayer,-1,pitchAt(e.y),beatAt(e.x),1);afterEdit(changed,index,replacement);}
 void mouseDown(const juce::MouseEvent& e)override{
  refresh();if(isShowing())grabKeyboardFocus();selected=hit(e.position);if(selected<0)return;
  if(e.mods.isRightButtonDown()||e.mods.isAltDown()){deleteSelected();return;}
  if(!e.mods.isLeftButtonDown())return;original=state.idea["layers"][juce::Identifier(activeLayer)][selected];preview=original.clone();anchor=e.position;resizing=e.position.x>=bounds(original).getRight()-juce::jmin(5.0f,bounds(original).getWidth()/3);dragging=true;repaint();
 }
 void mouseDrag(const juce::MouseEvent& e)override{
  if(!dragging)return;if(processor.compositionRevision()!=state.revision){refresh();return;}
  double start=double(original["start"]),duration=double(original["duration"]);int pitch=int(original["pitch"]);
  if(resizing){start=juce::jmin(start,beats()-.25);duration=juce::jlimit(.25,beats()-start,editing::snap(duration+(e.position.x-anchor.x)*beats()/usableWidth()));}
  else {start=juce::jlimit(0.0,beats()-duration,editing::snap(start+(e.position.x-anchor.x)*beats()/usableWidth()));pitch=juce::jlimit(0,127,pitch+int(std::round((anchor.y-e.position.y)/row)));}
  preview=original.clone();preview.getDynamicObject()->setProperty("start",start);preview.getDynamicObject()->setProperty("duration",duration);preview.getDynamicObject()->setProperty("pitch",pitch);repaint();
 }
 void mouseUp(const juce::MouseEvent&)override{if(!dragging)return;dragging=false;auto replacement=state.replacement;int index=selected;bool changed=processor.edit(state.revision,activeLayer,index,int(preview["pitch"]),double(preview["start"]),double(preview["duration"]));afterEdit(changed,index,replacement);}
private:
 static constexpr int row=12;Processor& processor;editing::Snapshot state;juce::String activeLayer="bass";int selected=-1;bool dragging=false,resizing=false;juce::Point<float> anchor;juce::var original,preview;
 juce::var notes()const{return state.idea["layers"][juce::Identifier(activeLayer)];}
 void afterEdit(bool changed,int index,uint64_t replacement){auto previous=notes();auto oldId=selected>=0&&selected<previous.size()?previous[selected]["id"].toString():juce::String{};refresh();if(changed&&state.replacement==replacement){selected=notes().size()?juce::jlimit(0,notes().size()-1,index):-1;if(bool(state.idea["composer"])){auto id=state.idea["editedNoteId"].toString();if(id=="0")id=oldId;auto list=notes();for(int i=0;i<list.size();++i)if(list[i]["id"].toString()==id){selected=i;break;}}}reveal();}
 void reveal(){auto list=notes();if(selected>=0&&selected<list.size()){auto n=list[selected];setDescription(activeLayer+" note "+juce::String(selected+1)+" of "+juce::String(list.size())+", MIDI "+n["pitch"].toString()+", beat "+n["start"].toString()+". Brackets select; arrows move; Shift and arrows resize; Delete removes.");if(auto* viewport=findParentComponentOfClass<juce::Viewport>()){int y=(127-int(n["pitch"]))*row;if(y<viewport->getViewPositionY()||y+row>viewport->getViewPositionY()+viewport->getViewHeight())viewport->setViewPosition(0,juce::jmax(0,y-viewport->getViewHeight()/2));}}else setDescription(list.size()?"No note selected. Brackets select notes.":"Empty layer. Use Add note to begin.");repaint();}
 double beats()const{return bool(state.idea["composer"])?(bool(state.idea["metadata"]["editable"])?double(state.idea["metadata"]["quarterBeats"]):0):double(state.idea["metadata"]["bars"])*4;}
 bool isBar(double beat)const{if(!bool(state.idea["composer"]))return std::fmod(beat,4)==0;auto bars=state.idea["metadata"]["barStarts"];for(int i=0;i<bars.size();++i)if(std::abs(double(bars[i])-beat)<1e-8)return true;return false;}
 double usableWidth()const{return juce::jmax(1,getWidth()-48);}
 float xAt(double beat)const{return 48+float(beat/juce::jmax(1.0,beats())*usableWidth());}
 double beatAt(float x)const{return (x-48)*beats()/usableWidth();}
 int pitchAt(float y)const{return juce::jlimit(0,127,127-int(y)/row);}
 juce::Rectangle<float> bounds(const juce::var& n)const{float right=juce::jmax(51.0f,float(getWidth())),left=juce::jlimit(48.0f,right-3,xAt(double(n["start"])));float width=juce::jlimit(3.0f,right-left,xAt(double(n["start"])+double(n["duration"]))-left);return {left,float((127-int(n["pitch"]))*row+1),width,float(row-2)};}
 int hit(juce::Point<float> point)const{auto list=state.idea["layers"][juce::Identifier(activeLayer)];for(int i=list.size();--i>=0;)if(bounds(list[i]).contains(point))return i;return -1;}
};
