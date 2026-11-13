#pragma once
#include "Composition.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_set>

namespace composer {
enum class ValidationCode { ok,version,identity,time,maps,track,note,control,harmony,sections,resource,density };
struct ValidationResult {
    ValidationCode code=ValidationCode::ok; std::string message;
    explicit operator bool()const{return code==ValidationCode::ok;}
};
namespace detail {
inline bool text(const std::string& s,size_t maximum=1024,bool required=false) {
    return (!required||!s.empty())&&s.size()<=maximum&&s.find('\0')==std::string::npos;
}
inline bool span(Tick start,Tick duration,Tick end) {
    return start.value>=0&&start<end&&duration.value>0&&duration.value<=end.value-start.value;
}
template<class Point> bool mapOrder(const std::vector<Point>& points,Tick end) {
    if(points.empty()||points.size()>limits::mapPoints||points.front().tick.value!=0)return false;
    for(size_t i=0;i<points.size();++i)
        if(points[i].tick.value<0||points[i].tick>=end||(i&&points[i].tick<=points[i-1].tick))return false;
    return true;
}
inline bool validTempos(const Composition& c) {
    if(c.ppq!=480||c.endTick.value<=0||c.endTick.value>limits::maxTick||!mapOrder(c.tempos,c.endTick))return false;
    for(const auto& p:c.tempos)if(p.microsPerQuarter<1||p.microsPerQuarter>16777215)return false;
    return true;
}
inline double secondsUnchecked(const Composition& c,Tick tick) {
    double seconds=0;
    for(size_t i=0;i<c.tempos.size()&&c.tempos[i].tick<tick;++i) {
        const auto end=i+1<c.tempos.size()?std::min(tick,c.tempos[i+1].tick):tick;
        seconds+=(end.value-c.tempos[i].tick.value)*(c.tempos[i].microsPerQuarter/(480.0*1000000.0));
    }
    return seconds;
}
inline bool addBytes(size_t& total,size_t count,size_t width=1) {
    if(count>(limits::documentBytes-total)/width)return false;
    total+=count*width;return true;
}
template<class T> bool vectorBytes(size_t& total,const std::vector<T>& v) {return addBytes(total,v.capacity(),sizeof(T));}
}

inline ValidationResult validate(const Composition& c) {
    auto error=[](ValidationCode code,const char* message){return ValidationResult{code,message};};
    if(c.schemaVersion!=2||c.ppq!=480||c.engineVersion==0)return error(ValidationCode::version,"Unsupported composition version or resolution");
    if(!detail::text(c.id,128,true)||!detail::text(c.style)||!detail::text(c.source)||!detail::text(c.timingNotice))return error(ValidationCode::identity,"Invalid composition identity or metadata");
    if(c.legacyJson.size()>limits::legacyBytes)return error(ValidationCode::resource,"Legacy provenance exceeds limit");
    if(c.endTick.value<=0||c.endTick.value>limits::maxTick)return error(ValidationCode::time,"Composition endpoint exceeds limit");
    if(c.tracks.empty()||c.tracks.size()>limits::tracks||c.sections.size()>limits::sections||c.chords.size()>limits::chords
       ||c.tempos.size()>limits::mapPoints||c.meters.size()>limits::mapPoints||c.keys.size()>limits::mapPoints)
        return error(ValidationCode::resource,"Composition collection limit exceeded");
    size_t noteCount=0,controlCount=0,bytes=sizeof(c);
    bool memory=detail::vectorBytes(bytes,c.tracks)&&detail::vectorBytes(bytes,c.tempos)&&detail::vectorBytes(bytes,c.meters)
        &&detail::vectorBytes(bytes,c.keys)&&detail::vectorBytes(bytes,c.sections)&&detail::vectorBytes(bytes,c.chords);
    for(const auto* s:{&c.id,&c.style,&c.source,&c.legacyJson,&c.timingNotice})memory=memory&&detail::addBytes(bytes,s->capacity()+1);
    for(const auto& t:c.tracks) {
        if(t.notes.size()>limits::notes-noteCount||t.controls.size()>limits::controls-controlCount)
            return error(ValidationCode::resource,"Composition event limit exceeded");
        noteCount+=t.notes.size();controlCount+=t.controls.size();
        memory=memory&&detail::vectorBytes(bytes,t.notes)&&detail::vectorBytes(bytes,t.controls)&&detail::addBytes(bytes,t.name.capacity()+1);
    }
    // Include conservative prepared note/CC events and timeline indexes in the same budget.
    memory=memory&&detail::addBytes(bytes,noteCount,2*32)&&detail::addBytes(bytes,controlCount,32);
    for(const auto& section:c.sections)memory=memory&&detail::addBytes(bytes,section.name.capacity()+1);
    for(const auto& chord:c.chords)memory=memory&&detail::vectorBytes(bytes,chord.pitchClasses)&&detail::vectorBytes(bytes,chord.voicing)&&detail::addBytes(bytes,chord.explanation.capacity()+1);
    if(!memory)return error(ValidationCode::resource,"Document and prepared data exceed memory budget");
    if(!detail::validTempos(c)||!detail::mapOrder(c.meters,c.endTick)||!detail::mapOrder(c.keys,c.endTick))return error(ValidationCode::maps,"Invalid timeline origin or ordering");
    if(detail::secondsUnchecked(c,c.endTick)>limits::seconds)return error(ValidationCode::time,"Composition exceeds thirty minutes");
    for(size_t i=0;i<c.meters.size();++i) {
        const auto& m=c.meters[i];
        if(m.numerator<1||m.numerator>12||(m.denominator!=2&&m.denominator!=4&&m.denominator!=8&&m.denominator!=16))return error(ValidationCode::maps,"Unsupported meter");
        if(i) {
            const auto& previous=c.meters[i-1];
            const auto bar=480LL*4*previous.numerator/previous.denominator;
            if((m.tick.value-previous.tick.value)%bar!=0)return error(ValidationCode::maps,"Meter change must occur at a preceding-meter bar boundary");
        }
    }
    for(const auto& k:c.keys)if(k.root<0||k.root>11||k.scale<Scale::major||k.scale>Scale::blues)return error(ValidationCode::maps,"Invalid key or scale");
    std::unordered_set<Id> ids;
    auto identity=[&](Id id){return id!=0&&ids.insert(id).second;};
    std::array<int,16> channels{};
    for(const auto& t:c.tracks)if(t.channel>=1&&t.channel<=16)++channels[size_t(t.channel-1)];
    std::bitset<128> ownedDrums;
    for(const auto& t:c.tracks) {
        if(!identity(t.id)||!detail::text(t.name,128,true)||t.role<Role::bass||t.role>Role::harmony||t.channel<1||t.channel>16||t.lowPitch<0||t.highPitch>127||t.lowPitch>t.highPitch)
            return error(ValidationCode::track,"Invalid track identity, role, range or channel");
        bool drum=t.role==Role::drums||t.role==Role::percussion;
        if(drum!=(t.channel==10)||(t.channel!=10&&channels[size_t(t.channel-1)]>1))return error(ValidationCode::track,"Channel assignment conflicts with track role");
        if((t.program&&(*t.program<0||*t.program>127))||(t.bank&&(t.bank->msb<0||t.bank->msb>127||t.bank->lsb<0||t.bank->lsb>127)))return error(ValidationCode::track,"Invalid GM preset");
        const bool shared=t.channel==10&&channels[9]>1;
        if(shared) {
            if(t.program||t.bank||!t.controls.empty()||t.ownedPitches.none()||(ownedDrums&t.ownedPitches).any())return error(ValidationCode::track,"Shared percussion requires disjoint pitches and fixed GM configuration");
            ownedDrums|=t.ownedPitches;
        }
        std::bitset<128> controllers;
        for(const auto& n:t.notes) {
            if(!identity(n.id)||n.pitch<t.lowPitch||n.pitch>t.highPitch||n.velocity<1||n.velocity>127||!detail::span(n.start,n.duration,c.endTick)
               ||(shared&&!t.ownedPitches.test(size_t(n.pitch))))return error(ValidationCode::note,"Invalid note identity, pitch, velocity or interval");
        }
        for(const auto& control:t.controls) {
            if(!identity(control.id)||control.tick.value<0||control.tick>c.endTick||control.controller<1||control.controller>=120||control.controller==32||control.value<0||control.value>127)
                return error(ValidationCode::control,"Invalid MIDI controller event");
            controllers.set(size_t(control.controller));
        }
        if(controllers.count()>16)return error(ValidationCode::control,"Too many distinct controllers on a channel");
    }
    for(size_t i=0;i<c.sections.size();++i) {
        const auto& s=c.sections[i];
        if(!identity(s.id)||!detail::text(s.name,128,true)||s.start.value<0||s.start>=s.end||s.end>c.endTick||s.role<SectionRole::intro||s.role>SectionRole::outro
           ||(i&&s.start<c.sections[i-1].end))return error(ValidationCode::sections,"Invalid or overlapping sections");
    }
    for(size_t i=0;i<c.chords.size();++i) {
        const auto& h=c.chords[i];
        if(!identity(h.id)||!detail::span(h.start,h.duration,c.endTick)||h.root<0||h.root>11||h.pitchClasses.empty()||h.pitchClasses.size()>12||h.voicing.size()>16
           ||h.inversion<0||size_t(h.inversion)>=h.pitchClasses.size()||h.tension<0||h.tension>100||!detail::text(h.explanation)
           ||(i&&h.start.value<c.chords[i-1].start.value+c.chords[i-1].duration.value))return error(ValidationCode::harmony,"Invalid chord timeline");
        std::bitset<12> pitches;
        for(int pitch:h.pitchClasses){if(pitch<0||pitch>11||pitches.test(size_t(pitch)))return error(ValidationCode::harmony,"Invalid chord pitch classes");pitches.set(size_t(pitch));}
        for(int pitch:h.voicing)if(pitch<0||pitch>127||!pitches.test(size_t(pitch%12)))return error(ValidationCode::harmony,"Voicing is outside chord pitch classes");
    }
    struct Edge {int64_t tick;int key,delta;};
    std::vector<Edge> edges;edges.reserve(noteCount*2);
    for(const auto& t:c.tracks)for(const auto& n:t.notes) {
        int key=(t.channel-1)*128+n.pitch;
        edges.push_back({n.start.value,key,1});edges.push_back({n.start.value+n.duration.value,key,-1});
    }
    std::sort(edges.begin(),edges.end(),[](const auto& a,const auto& b){return a.tick<b.tick||(a.tick==b.tick&&a.delta<b.delta);});
    std::array<int,2048> held{};int active=0;
    for(const auto& edge:edges) {
        auto& count=held[size_t(edge.key)];
        if(edge.delta>0){if(count++==0&&++active>256)return error(ValidationCode::density,"Composition exceeds 256 simultaneous channel/pitch keys");}
        else if(--count==0)--active;
    }
    return {};
}
}
