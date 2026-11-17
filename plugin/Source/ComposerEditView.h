#pragma once
#include <juce_core/juce_core.h>
#include "engine/LoopEditing.h"

// Small, immutable control-thread projection. IDs are strings, preserving all64bits.
namespace composerView {
inline juce::String key(composer::Id id){return "track-"+juce::String(id);}
inline std::vector<int64_t> barStarts(const composer::Composition& c){
 std::vector<int64_t> starts;size_t meter=0;
 for(int64_t tick=0;tick<c.endTick.value;){if(starts.size()==16)return {};starts.push_back(tick);while(meter+1<c.meters.size()&&c.meters[meter+1].tick.value<=tick)++meter;const auto& m=c.meters[meter];tick+=480LL*4*m.numerator/m.denominator;}
 starts.push_back(c.endTick.value);return starts;
}
inline bool editable(const composer::Composition& c,size_t bytes){return bytes<=8*1024*1024&&composer::loopEditDetail::count(c)<=8192&&c.endTick.value>=120&&!barStarts(c).empty();}
inline juce::var make(const composer::Composition& c,size_t bytes,composer::Id selected){
 juce::var result(new juce::DynamicObject),metadata(new juce::DynamicObject);auto* root=result.getDynamicObject();root->setProperty("composer",true);root->setProperty("metadata",metadata);auto* meta=metadata.getDynamicObject();const bool allowed=editable(c,bytes);meta->setProperty("editable",allowed);meta->setProperty("quarterBeats",c.endTick.value/480.0);root->setProperty("editedNoteId",juce::String(selected));if(!allowed)return result;
 juce::Array<juce::var> bars,tracks;for(auto tick:barStarts(c))bars.add(tick/480.0);meta->setProperty("barStarts",bars);juce::var layers(new juce::DynamicObject),defaults(new juce::DynamicObject);
 for(const auto& t:c.tracks){const auto k=key(t.id);juce::var descriptor(new juce::DynamicObject);descriptor.getDynamicObject()->setProperty("key",k);descriptor.getDynamicObject()->setProperty("name",juce::String::fromUTF8(t.name.c_str())+" [ch "+juce::String(t.channel)+"]");tracks.add(descriptor);int pitch=juce::jlimit(t.lowPitch,t.highPitch,60);if(t.channel==10){for(int p=t.lowPitch;p<=t.highPitch;++p)if(t.ownedPitches[size_t(p)]){pitch=p;break;}}defaults.getDynamicObject()->setProperty(juce::Identifier(k),pitch);
  juce::Array<juce::var> notes;for(const auto& n:t.notes){juce::var v(new juce::DynamicObject);auto* o=v.getDynamicObject();o->setProperty("id",juce::String(n.id));o->setProperty("pitch",n.pitch);o->setProperty("velocity",n.velocity);o->setProperty("start",n.start.value/480.0);o->setProperty("duration",n.duration.value/480.0);notes.add(v);}layers.getDynamicObject()->setProperty(juce::Identifier(k),notes);
 }
 root->setProperty("tracks",tracks);root->setProperty("layers",layers);root->setProperty("defaults",defaults);return result;
}
}
