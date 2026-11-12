#pragma once
#include "Wire.h"
namespace editing {
struct Snapshot {juce::var idea;uint64_t revision=0,replacement=0;};
inline bool layer(const juce::String& name){return name=="bass"||name=="chords"||name=="melody";}
inline double snap(double beat){return std::floor(beat*4+.5)/4;}
inline bool change(juce::var& idea,const juce::String& name,int index,int pitch,double start,double duration,bool remove){
 if(!layer(name)||!std::isfinite(start)||!std::isfinite(duration)||!idea.isObject())return false;
 auto* notes=idea["layers"][juce::Identifier(name)].getArray();double end=double(idea["metadata"]["bars"])*4;
 if(!notes||end<.25||index< -1||index>=notes->size()||(remove&&index<0))return false;
 if(remove){notes->remove(index);return true;}
 if(index>=0){auto old=(*notes)[index];if(int(old["pitch"])==pitch&&double(old["start"])==start&&double(old["duration"])==duration)return false;}
 bool moving=index>=0&&double((*notes)[index]["duration"])==duration;
 duration=moving?duration:juce::jlimit(.25,end,snap(duration));start=juce::jlimit(0.0,end-duration,snap(start));pitch=juce::jlimit(0,127,pitch);
 auto note=index<0?juce::var(new juce::DynamicObject):(*notes)[index];auto* o=note.getDynamicObject();if(!o)return false;
 o->setProperty("layer",name);o->setProperty("pitch",pitch);o->setProperty("start",start);o->setProperty("duration",duration);if(index<0){o->setProperty("velocity",90);notes->add(note);}return true;
}
inline bool quantize(juce::var& idea,const juce::String& name){
 if(!layer(name))return false;auto* notes=idea["layers"][juce::Identifier(name)].getArray();if(!notes)return false;double end=double(idea["metadata"]["bars"])*4;
 for(auto& n:*notes){double start=juce::jlimit(0.0,end-.25,snap(double(n["start"])));double finish=juce::jlimit(start+.25,end,snap(double(n["start"])+double(n["duration"])));n.getDynamicObject()->setProperty("start",start);n.getDynamicObject()->setProperty("duration",finish-start);}return true;
}
}
