#pragma once
#include "Validation.h"
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <utility>

namespace composer {
struct MidiResult {
    std::vector<uint8_t> bytes;std::string error;
    explicit operator bool()const{return error.empty()&&!bytes.empty();}
};
namespace midiDetail {
inline constexpr size_t maximumBytes=32*1024*1024;
inline constexpr uint32_t maximumDelta=0x0fffffff;
struct Writer {
    std::vector<uint8_t> bytes;
    void reserve(size_t count)const{if(count>maximumBytes-bytes.size())throw std::length_error("MIDI exceeds 32 MiB");}
    void byte(int value){reserve(1);bytes.push_back(uint8_t(value));}
    void raw(const uint8_t* data,size_t count){reserve(count);if(count)bytes.insert(bytes.end(),data,data+count);}
    void be(uint32_t value,int count){for(int shift=(count-1)*8;shift>=0;shift-=8)byte(int((value>>shift)&255));}
    void vlq(uint32_t value){uint8_t data[4];int count=0;data[count++]=uint8_t(value&127);while((value>>=7)!=0)data[count++]=uint8_t((value&127)|128);while(count)byte(data[--count]);}
    void delta(int64_t value){
        // SMF deltas are at most four bytes, even when the timeline is longer.
        while(value>maximumDelta){vlq(maximumDelta);byte(0xff);byte(0x7f);byte(0);value-=maximumDelta;}
        vlq(uint32_t(value));
    }
};
struct Event {int64_t tick=0;int priority=0;std::vector<uint8_t> data;};
inline Event meta(int64_t tick,int type,const uint8_t* data,size_t count){
    Writer w;w.byte(0xff);w.byte(type);w.vlq(uint32_t(count));w.raw(data,count);return {tick,0,std::move(w.bytes)};
}
inline Event text(int64_t tick,int type,const std::string& value){return meta(tick,type,reinterpret_cast<const uint8_t*>(value.data()),value.size());}
inline Event midi(int64_t tick,int priority,std::initializer_list<uint8_t> bytes){return {tick,priority,bytes};}
inline void chunk(Writer& output,std::vector<Event>& events,int64_t end){
    std::stable_sort(events.begin(),events.end(),[](const auto& a,const auto& b){return a.tick<b.tick||(a.tick==b.tick&&a.priority<b.priority);});
    Writer track;int64_t previous=0;
    for(const auto& event:events){track.delta(event.tick-previous);track.raw(event.data.data(),event.data.size());previous=event.tick;}
    track.delta(end-previous);track.byte(0xff);track.byte(0x2f);track.byte(0);
    const uint8_t tag[]={'M','T','r','k'};output.raw(tag,4);output.be(uint32_t(track.bytes.size()),4);output.raw(track.bytes.data(),track.bytes.size());
}
inline std::pair<int,bool> signature(const Key& key){
    const bool minor=key.scale==Scale::naturalMinor||key.scale==Scale::harmonicMinor||key.scale==Scale::melodicMinor||key.scale==Scale::minorPentatonic||key.scale==Scale::blues;
    int majorRoot=key.root+(minor?3:0);
    switch(key.scale){case Scale::dorian:majorRoot-=2;break;case Scale::phrygian:majorRoot-=4;break;case Scale::lydian:majorRoot-=5;break;case Scale::mixolydian:majorRoot-=7;break;default:break;}
    // Prefer the conventional spelling with the fewest accidentals; F# wins the tie.
    constexpr int sharps[]={0,-5,2,-3,4,-1,6,1,-4,3,-2,5};
    return {sharps[(majorRoot%12+12)%12],minor};
}
inline std::string keyLabel(const Key& key){
    constexpr const char* roots[]={"C","Db","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
    constexpr const char* modes[]={"major","natural minor","harmonic minor","melodic minor","dorian","phrygian","lydian","mixolydian","major pentatonic","minor pentatonic","blues"};
    return std::string("LoopForge key: ")+roots[key.root]+" "+modes[int(key.scale)];
}
inline std::vector<Event> conductor(const Composition& c){
    std::vector<Event> events;events.push_back(text(0,3,"LoopForge conductor"));
    for(const auto& t:c.tempos){uint8_t data[]={uint8_t(t.microsPerQuarter>>16),uint8_t(t.microsPerQuarter>>8),uint8_t(t.microsPerQuarter)};events.push_back(meta(t.tick.value,0x51,data,3));}
    for(const auto& m:c.meters){int power=0;for(int d=m.denominator;d>1;d/=2)++power;uint8_t data[]={uint8_t(m.numerator),uint8_t(power),24,8};events.push_back(meta(m.tick.value,0x58,data,4));}
    for(const auto& k:c.keys){auto [sf,minor]=signature(k);uint8_t data[]={uint8_t(sf),uint8_t(minor)};events.push_back(meta(k.tick.value,0x59,data,2));events.push_back(text(k.tick.value,1,keyLabel(k)));}
    for(const auto& s:c.sections)events.push_back(text(s.start.value,6,s.name));
    return events;
}
inline std::vector<Event> trackEvents(const Track& t,int64_t end){
    std::vector<Event> events;events.reserve(t.notes.size()*3+t.controls.size()+8);
    events.push_back(text(0,3,t.name));
    const auto channel=uint8_t(t.channel-1);
    auto control=[&](int64_t tick,int priority,int cc,int value){events.push_back(midi(tick,priority,{uint8_t(0xb0|channel),uint8_t(cc),uint8_t(value)}));};
    if(t.bank){control(0,0,0,t.bank->msb);control(0,0,32,t.bank->lsb);}
    if(t.program)events.push_back(midi(0,0,{uint8_t(0xc0|channel),uint8_t(*t.program)}));
    std::bitset<128> pedals;
    for(const auto& cc:t.controls){control(cc.tick.value,2,cc.controller,cc.value);if(cc.controller==64||cc.controller==66||cc.controller==69)pedals.set(size_t(cc.controller));}
    struct Edge{int64_t tick;int pitch,velocity;bool on;};
    std::vector<Edge> edges;edges.reserve(t.notes.size()*2);
    for(const auto& note:t.notes){edges.push_back({note.start.value,note.pitch,note.velocity,true});edges.push_back({note.start.value+note.duration.value,note.pitch,0,false});}
    std::stable_sort(edges.begin(),edges.end(),[](const auto& a,const auto& b){return a.tick<b.tick||(a.tick==b.tick&&a.on<b.on);});
    std::array<uint32_t,128> held{};std::array<int64_t,128> onsetTick;onsetTick.fill(-1);std::array<size_t,128> onsetIndex{};
    for(const auto& edge:edges){
        auto& count=held[size_t(edge.pitch)];
        auto off=[&](int priority){events.push_back(midi(edge.tick,priority,{uint8_t(0x80|channel),uint8_t(edge.pitch),0}));};
        if(edge.on){
            // Simultaneous unisons share one audible onset; retain every lifetime.
            const auto pitch=size_t(edge.pitch);
            if(onsetTick[pitch]==edge.tick)events[onsetIndex[pitch]].data[2]=uint8_t(edge.velocity);
            else{
                if(count)off(3);
                onsetTick[pitch]=edge.tick;onsetIndex[pitch]=events.size();
                events.push_back(midi(edge.tick,3,{uint8_t(0x90|channel),uint8_t(edge.pitch),uint8_t(edge.velocity)}));
            }
            ++count;
        }
        else if(--count==0)off(1);
    }
    for(int cc:{64,66,69})if(pedals.test(size_t(cc)))control(end,4,cc,0);
    return events;
}
}

// Worker-only: allocates and validates the entire immutable document before encoding.
inline MidiResult encodeCompositionMidi(const Composition& composition){
    try{
        auto valid=validate(composition);if(!valid)return {{},valid.message};
        midiDetail::Writer output;const uint8_t header[]={'M','T','h','d'};output.raw(header,4);output.be(6,4);output.be(1,2);output.be(uint32_t(composition.tracks.size()+1),2);output.be(480,2);
        auto conductor=midiDetail::conductor(composition);midiDetail::chunk(output,conductor,composition.endTick.value);
        for(const auto& track:composition.tracks){auto events=midiDetail::trackEvents(track,composition.endTick.value);midiDetail::chunk(output,events,composition.endTick.value);}
        return {std::move(output.bytes),{}};
    }catch(const std::length_error&){return {{},"MIDI exceeds encoding budget"};}
    catch(const std::bad_alloc&){return {{},"Insufficient memory for MIDI export"};}
}
}
