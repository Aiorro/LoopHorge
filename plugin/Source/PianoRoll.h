#pragma once
#include "Processor.h"
// View-only previews are local; the processor accepts one revision-guarded edit on release.
class PianoRoll final:public juce::Component {
public:
 explicit PianoRoll(Processor& p):processor(p){setName("Piano roll");refresh();}
 void selectLayer(const juce::String& name){if(!editing::layer(name))return;activeLayer=name;selected=-1;dragging=false;refresh();}
 bool refresh(){auto next=processor.snapshot();bool replaced=next.replacement!=state.replacement;if(next.revision!=state.revision){dragging=false;selected=-1;state=std::move(next);repaint();}return replaced;}
 int preferredScrollY()const{int pitch=60;auto notes=state.idea["layers"][juce::Identifier(activeLayer)];if(notes.size())pitch=int(notes[0]["pitch"]);return juce::jlimit(0,128*row-300,(127-pitch)*row-130);}
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff242936));double end=beats();if(end<=0)return;
  for(int pitch=0;pitch<128;++pitch){int y=(127-pitch)*row;g.setColour(juce::Colour(pitch%12==0?0xff4c566a:0xff303646));g.drawHorizontalLine(y,0,float(getWidth()));if(pitch%12==0){g.setColour(juce::Colours::lightgrey);g.drawText("C"+juce::String(pitch/12-1),0,y,46,row,juce::Justification::centredRight);}}
  for(double beat=0;beat<=end;beat+=.25){g.setColour(juce::Colour(std::fmod(beat,4)==0?0xff667286:std::fmod(beat,1)==0?0xff424b5c:0xff303646));g.drawVerticalLine(int(xAt(beat)),0,float(getHeight()));}
  const char* names[]={"bass","chords","melody"};const juce::Colour colours[]={juce::Colour(0xffb494ff),juce::Colour(0xff53d9ba),juce::Colour(0xffffbc70)};
  for(int ch=0;ch<3;++ch){auto list=state.idea["layers"][names[ch]];for(int i=0;i<list.size();++i){auto note=dragging&&activeLayer==names[ch]&&i==selected?preview:list[i];auto rect=bounds(note);g.setColour(colours[ch].withAlpha(activeLayer==names[ch]?1.0f:.3f));g.fillRoundedRectangle(rect,2);if(i==selected&&activeLayer==names[ch]){g.setColour(juce::Colours::white);g.drawRect(rect,1.5f);}}}
 }
 void mouseDoubleClick(const juce::MouseEvent& e)override{refresh();if(e.x<48||hit(e.position)>=0||beats()<=0)return;processor.edit(state.revision,activeLayer,-1,pitchAt(e.y),beatAt(e.x),1);refresh();}
 void mouseDown(const juce::MouseEvent& e)override{
  refresh();selected=hit(e.position);if(selected<0)return;
  if(e.mods.isRightButtonDown()||e.mods.isAltDown()){processor.edit(state.revision,activeLayer,selected,0,0,1,true);refresh();return;}
  if(!e.mods.isLeftButtonDown())return;original=state.idea["layers"][juce::Identifier(activeLayer)][selected];preview=original.clone();anchor=e.position;resizing=e.position.x>=bounds(original).getRight()-juce::jmin(5.0f,bounds(original).getWidth()/3);dragging=true;repaint();
 }
 void mouseDrag(const juce::MouseEvent& e)override{
  if(!dragging)return;if(processor.snapshot().revision!=state.revision){refresh();return;}
  double start=double(original["start"]),duration=double(original["duration"]);int pitch=int(original["pitch"]);
  if(resizing){start=juce::jmin(start,beats()-.25);duration=juce::jlimit(.25,beats()-start,editing::snap(duration+(e.position.x-anchor.x)*beats()/usableWidth()));}
  else {start=juce::jlimit(0.0,beats()-duration,editing::snap(start+(e.position.x-anchor.x)*beats()/usableWidth()));pitch=juce::jlimit(0,127,pitch+int(std::round((anchor.y-e.position.y)/row)));}
  preview=original.clone();preview.getDynamicObject()->setProperty("start",start);preview.getDynamicObject()->setProperty("duration",duration);preview.getDynamicObject()->setProperty("pitch",pitch);repaint();
 }
 void mouseUp(const juce::MouseEvent&)override{if(!dragging)return;dragging=false;processor.edit(state.revision,activeLayer,selected,int(preview["pitch"]),double(preview["start"]),double(preview["duration"]));refresh();repaint();}
private:
 static constexpr int row=12;Processor& processor;editing::Snapshot state;juce::String activeLayer="bass";int selected=-1;bool dragging=false,resizing=false;juce::Point<float> anchor;juce::var original,preview;
 double beats()const{return double(state.idea["metadata"]["bars"])*4;}
 double usableWidth()const{return juce::jmax(1,getWidth()-48);}
 float xAt(double beat)const{return 48+float(beat/juce::jmax(1.0,beats())*usableWidth());}
 double beatAt(float x)const{return (x-48)*beats()/usableWidth();}
 int pitchAt(float y)const{return juce::jlimit(0,127,127-int(y)/row);}
 juce::Rectangle<float> bounds(const juce::var& n)const{float right=juce::jmax(51.0f,float(getWidth())),left=juce::jlimit(48.0f,right-3,xAt(double(n["start"])));float width=juce::jlimit(3.0f,right-left,xAt(double(n["start"])+double(n["duration"]))-left);return {left,float((127-int(n["pitch"]))*row+1),width,float(row-2)};}
 int hit(juce::Point<float> point)const{auto list=state.idea["layers"][juce::Identifier(activeLayer)];for(int i=list.size();--i>=0;)if(bounds(list[i]).contains(point))return i;return -1;}
};
