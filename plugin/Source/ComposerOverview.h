#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "engine/Composition.h"
#include "engine/Direction.h"
#include "StudioLook.h"
#include <array>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>

// Read-only full-song overview. Model construction is non-RT and paint never scans source notes.
class ComposerOverview final:public juce::Component {
public:
 ComposerOverview(){setWantsKeyboardFocus(true);setName("Composer overview");setTitle("Composer overview");details.setName("Composition maps and tracks");details.setTitle("Composition maps and tracks");details.setMultiLine(true);details.setReadOnly(true);details.setScrollbarsShown(true);details.setCaretVisible(false);details.setFont(juce::FontOptions(12));details.setColour(juce::TextEditor::backgroundColourId,studio::field);addAndMakeVisible(details);}
 // Input has passed the composition/project validator. No document is retained.
 bool setDocument(const composer::Composition& c,double origin){
  if(c.endTick.value<=0||c.tracks.size()>rows.size()||!std::isfinite(origin))return false;
  rowCount=c.tracks.size();end=c.endTick.value;sections.clear();size_t totalNotes=0,totalControls=0;
  std::string text="PROJECT "+c.id+"\nSeed "+std::to_string(c.seed)+" | Engine "+std::to_string(c.engineVersion)+" | "+c.style+"\n";
  char number[64];auto last=std::to_chars(number,number+sizeof(number),origin,std::chars_format::general,std::numeric_limits<double>::max_digits10);
  text+="Host origin PPQ "+std::string(number,last.ptr)+" | End tick "+std::to_string(end)+"\nComposition tempo map is used standalone; host BPM controls plugin playback.\n\n";
  std::vector<std::array<int32_t,bins+1>> changes(128);
  for(size_t i=0;i<rowCount;++i){auto& row=rows[i];const auto& t=c.tracks[i];row=Row{};row.colour=palette[size_t(t.role)%palette.size()];row.label=juce::String::fromUTF8(t.name.c_str());row.sub=juce::String(composer::roleName(t.role))+" | ch "+juce::String(t.channel)+" | "+juce::String(t.notes.size())+" notes";
   text+="Track "+std::to_string(t.id)+": "+t.name+" | "+composer::roleName(t.role)+" | channel "+std::to_string(t.channel)+" | "+std::to_string(t.notes.size())+" notes | "+std::to_string(t.controls.size())+" CC";
   if(t.program){text+=" | GM program "+std::to_string(*t.program);}
   if(t.bank){text+=" | bank "+std::to_string(t.bank->msb)+":"+std::to_string(t.bank->lsb);}
   text+='\n';
   totalNotes+=t.notes.size();totalControls+=t.controls.size();std::fill(changes.begin(),changes.end(),std::array<int32_t,bins+1>{});
   for(const auto& n:t.notes){if(n.pitch<0||n.pitch>127||n.start.value<0||n.duration.value<=0||n.start.value>=end||n.duration.value>end-n.start.value)return false;
    const auto first=size_t(n.start.value*bins/end),after=size_t(((n.start.value+n.duration.value)*bins+end-1)/end);
    ++changes[size_t(n.pitch)][first];--changes[size_t(n.pitch)][after];
   }
   for(int pitch=0;pitch<128;++pitch){int32_t held=0;for(size_t b=0;b<bins;++b){held+=changes[size_t(pitch)][b];if(held>0){auto& bin=row.cells[b];bin.low=std::min(bin.low,pitch);bin.high=std::max(bin.high,pitch);bin.count+=uint32_t(held);}}}
  }
  text+="\nTEMPO MAP\n";for(const auto& t:c.tempos)text+="tick "+std::to_string(t.tick.value)+": "+std::to_string(60000000.0/t.microsPerQuarter)+" BPM\n";
  text+="\nMETER MAP\n";for(const auto& m:c.meters)text+="tick "+std::to_string(m.tick.value)+": "+std::to_string(m.numerator)+"/"+std::to_string(m.denominator)+"\n";
  constexpr const char* roots[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
  text+="\nKEY MAP\n";for(const auto& k:c.keys)text+="tick "+std::to_string(k.tick.value)+": "+roots[k.root]+" "+composer::scaleName(k.scale)+"\n";
  text+="\nSECTIONS\n";for(const auto& s:c.sections){text+=s.name+" ["+std::to_string(s.start.value)+", "+std::to_string(s.end.value)+") id "+std::to_string(s.id)+"\n";sections.push_back({s.start.value,s.end.value,juce::String::fromUTF8(s.name.c_str())});}
  summary=juce::String(rowCount)+" tracks | "+juce::String(totalNotes)+" notes | "+juce::String(totalControls)+" CC | seed "+juce::String(c.seed)+" | origin PPQ "+juce::String::fromUTF8(number,int(last.ptr-number));
  setDescription(summary+". Read-only pitch range and density overview; exact maps and tracks below.");details.setText(juce::String::fromUTF8(text.data(),int(text.size())),false);details.setCaretPosition(0);resized();repaint();return true;
 }
 bool keyPressed(const juce::KeyPress& key)override{
  if(key.getModifiers().isAnyModifierKeyDown())return false;
  auto* viewport=findParentComponentOfClass<juce::Viewport>();if(!viewport)return false;int y=viewport->getViewPositionY();const int page=std::max(1,viewport->getViewHeight()-32);
  const int code=key.getKeyCode();if(code==juce::KeyPress::homeKey)y=0;else if(code==juce::KeyPress::endKey)y=getHeight();else if(code==juce::KeyPress::pageUpKey)y-=page;else if(code==juce::KeyPress::pageDownKey)y+=page;else if(code==juce::KeyPress::upKey)y-=32;else if(code==juce::KeyPress::downKey)y+=32;else return false;
  viewport->setViewPosition(viewport->getViewPositionX(),std::max(0,y));return true;
 }
 int requiredHeight()const{return header+int(rowCount)*rowHeight+230;}
 void resized()override{details.setBounds(12,header+int(rowCount)*rowHeight+10,std::max(1,getWidth()-24),210);}
 void paint(juce::Graphics& g)override{
  g.fillAll(studio::background);g.setColour(studio::text);g.setFont(studio::font(14,juce::Font::bold));g.drawFittedText(summary,12,10,getWidth()-24,24,juce::Justification::centredLeft,1);g.setFont(studio::font(12));g.setColour(studio::muted);
  g.drawText("Track activity   /   Quarter beats",12,39,getWidth()-24,20,juce::Justification::centredLeft);
  const int x=190,w=std::max(1,getWidth()-x-14);if(end<=0)return;
  for(int i=0;i<=8;++i){const int at=x+w*i/8;g.setColour(studio::border);g.drawVerticalLine(at,83.0f,float(header+int(rowCount)*rowHeight));g.setColour(studio::muted);g.drawText(juce::String(double(end)*i/(8*480.0),1),std::clamp(at-20,x,x+std::max(0,w-40)),61,40,20,i==0?juce::Justification::centredLeft:(i==8?juce::Justification::centredRight:juce::Justification::centred));}
  for(const auto& s:sections){const int a=x+int(double(s.start)/end*w),b=x+int(double(s.end)/end*w);g.setColour(studio::active);g.fillRect(a,84,std::max(1,b-a-1),20);if(b-a>52){g.setColour(studio::text);g.drawText(s.name,a+3,84,b-a-6,20,juce::Justification::centredLeft);}}
  for(size_t i=0;i<rowCount;++i){const auto& r=rows[i];const int top=header+int(i)*rowHeight;g.setColour(studio::border);g.drawHorizontalLine(top+rowHeight-1,0.0f,float(getWidth()));g.setColour(studio::text);g.drawFittedText(r.label,12,top+8,170,22,juce::Justification::centredLeft,1);g.setColour(studio::muted);g.drawFittedText(r.sub,12,top+31,170,28,juce::Justification::topLeft,2);
   const int columns=std::min(w,int(bins));for(int col=0;col<columns;++col){int low=128,high=-1;uint32_t density=0;const size_t first=size_t(col)*bins/size_t(columns),after=size_t(col+1)*bins/size_t(columns);for(auto b=first;b<after;++b){low=std::min(low,r.cells[b].low);high=std::max(high,r.cells[b].high);density=std::max(density,r.cells[b].count);}if(high<0)continue;
    const float a=float(x)+float(col)*w/columns,b=float(x)+float(col+1)*w/columns;g.setColour(juce::Colour(r.colour).withAlpha(std::min(1.0f,0.5f+float(density)*0.05f)));g.fillRect(a,float(top+8)+(127-high)*0.38f,std::max(1.0f,b-a),std::max(2.0f,(high-low+1)*0.38f));
   }
  }
 }
private:
 static constexpr size_t bins=1024;static constexpr int header=112,rowHeight=72;
 struct Bin{int low=128,high=-1;uint32_t count=0;};struct Row{juce::String label,sub;uint32_t colour=0;std::array<Bin,bins> cells;};
 struct Section{int64_t start,end;juce::String name;};
 inline static constexpr std::array<uint32_t,9> palette={0xffd7fb66,0xff67d8bb,0xffe4b877,0xffb7a4ef,0xffeaa5b5,0xff88bdd5,0xffa9e6ab,0xffc5ade7,0xffc6d47e};
 std::array<Row,16> rows;size_t rowCount=0;int64_t end=0;std::vector<Section> sections;juce::String summary;juce::TextEditor details;
};
