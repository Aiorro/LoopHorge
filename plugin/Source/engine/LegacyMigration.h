#pragma once
#include "Validation.h"
#include "../Wire.h"
#include <cmath>

namespace composer {
struct ImportResult {
    std::optional<Composition> composition;
    std::string error,timingNotice;
};
namespace detail {
// Bound traversal before JUCE serialization, including unknown legacy extension fields.
inline bool legacySize(const juce::var& value,size_t& remaining,int depth=0) {
    if(depth>=32||remaining<2)return false;
    remaining-=2;
    auto string=[&](const juce::String& s) {
        auto bytes=s.getNumBytesAsUTF8();
        if(bytes>remaining/6)return false;
        remaining-=bytes*6;return true;
    };
    if(value.isString())return string(value.toString());
    if(value.isDouble())return std::isfinite(double(value))&&remaining>=32?(remaining-=32,true):false;
    if(value.isInt()||value.isInt64()){if(remaining<24)return false;remaining-=24;return true;}
    if(value.isBool()||value.isVoid())return true;
    if(auto* array=value.getArray()) {
        if(size_t(array->size())>remaining/2)return false;
        for(const auto& child:*array)if(!legacySize(child,remaining,depth+1))return false;
        return true;
    }
    if(auto* object=value.getDynamicObject()) {
        const auto& properties=object->getProperties();
        if(size_t(properties.size())>remaining/2)return false;
        for(int i=0;i<properties.size();++i)
            if(!string(properties.getName(i).toString())||!legacySize(properties.getValueAt(i),remaining,depth+1))return false;
        return true;
    }
    return false;
}
}

inline ImportResult importLegacyIdea(const juce::var& idea) {
    auto fail=[](const char* message){return ImportResult{{},message,{}};};
    try {
        size_t remaining=limits::legacyBytes;
        if(!detail::legacySize(idea,remaining))return fail("Legacy data exceeds serialization or nesting limits");
        Schedule checked;
        if(!wire::schedule(idea,checked))return fail("Invalid legacy composition");
        const auto metadata=idea["metadata"];
        if(!wire::integer(metadata["bars"],4,8)||(int(metadata["bars"])!=4&&int(metadata["bars"])!=8)
           ||(!metadata["bpm"].isInt()&&!metadata["bpm"].isInt64()&&!metadata["bpm"].isDouble()))return fail("Invalid legacy meter or tempo");
        if(metadata.hasProperty("seed")&&!wire::integer(metadata["seed"],0,4294967295.0))return fail("Invalid legacy seed");
        const juce::StringArray roots{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        const juce::StringArray modes{"minor","major","dorian","phrygian"};
        const Scale scales[]{Scale::naturalMinor,Scale::major,Scale::dorian,Scale::phrygian};
        const juce::StringArray styles{"trap","boom-bap","r&b","ambient","nu-metal"};
        int root=roots.indexOf(metadata["key"].toString()),mode=modes.indexOf(metadata["mode"].toString());
        if(!metadata["key"].isString()||!metadata["mode"].isString()||!metadata["style"].isString()
           ||root<0||mode<0||!styles.contains(metadata["style"].toString()))return fail("Unsupported legacy key, mode or style");
        Composition c;
        c.legacyJson=juce::JSON::toString(idea,true).toStdString();
        if(c.legacyJson.size()>limits::legacyBytes)return fail("Legacy data exceeds byte limit");
        uint64_t hash=14695981039346656037ULL;
        for(unsigned char byte:c.legacyJson){hash^=byte;hash*=1099511628211ULL;}
        c.id="legacy-"+std::to_string(hash);c.source="legacy-v1-import";
        c.seed=metadata.hasProperty("seed")?uint32_t(double(metadata["seed"])):0;
        c.style=metadata["style"].toString().toStdString();
        c.endTick={int64_t(checked.beats)*480};
        auto micros=int(std::llround(60000000.0/checked.bpm));
        c.tempos={{{0},micros}};c.meters={{{0},4,4}};c.keys={{{0},root,scales[mode]}};
        Id id=1;bool quantized=false;
        const char* names[]{"bass","chords","melody"};
        const Role roles[]{Role::bass,Role::chords,Role::lead};
        for(int index=0;index<3;++index) {
            Track track;track.id=id++;track.name=names[index];track.role=roles[index];track.channel=index+1;
            const auto& notes=*idea["layers"][names[index]].getArray();track.notes.reserve(size_t(notes.size()));
            for(const auto& note:notes) {
                double start=note["start"],finish=start+double(note["duration"]);
                auto onset=std::clamp(int64_t(std::llround(start*480)),int64_t(0),c.endTick.value-1);
                auto end=std::clamp(int64_t(std::llround(finish*480)),onset+1,c.endTick.value);
                quantized=quantized||std::abs(start-onset/480.0)>1e-12||std::abs(finish-end/480.0)>1e-12;
                track.notes.push_back({id++,int(note["pitch"]),int(note["velocity"]),{onset},{end-onset}});
            }
            c.tracks.push_back(std::move(track));
        }
        std::string notice=quantized?"Legacy timing quantized to PPQ480 (minimum one tick); original data retained.":"Legacy timing represented exactly at PPQ480; original data retained.";
        if(std::abs(60000000.0/checked.bpm-micros)>1e-9)notice+=" MIDI tempo rounded to the nearest microsecond per quarter.";
        if(!metadata.hasProperty("seed"))notice+=" Missing legacy seed defaults to0; no randomness added.";
        c.timingNotice=notice;
        auto validation=validate(c);
        if(!validation)return {{},validation.message,{}};
        return {std::move(c),{},std::move(notice)};
    }catch(const std::bad_alloc&) {return fail("Not enough memory to import legacy composition");}
}

inline ImportResult importLegacyState(const juce::var& state) {
    if(!state.isObject()||!wire::integer(state["version"],1,1))return {{},"Unsupported legacy saved-state version",{}};
    return importLegacyIdea(state["idea"]);
}
}
