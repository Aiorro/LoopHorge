#pragma once
#include "Validation.h"
#include <atomic>
#include <limits>
#include <new>
#include <utility>

namespace composer {
struct ActiveNote {uint16_t key=0;uint32_t count=0;uint8_t velocity=0;};
struct ControlValue {uint8_t channel=1,controller=0,value=0;};
struct ChannelPreset {int program=-1,bankMsb=-1,bankLsb=-1;};
struct ChaseState {
    std::array<ActiveNote,256> notes{};size_t noteCount=0;
    std::array<ControlValue,256> controls{};size_t controlCount=0;
    std::array<ChannelPreset,16> presets{};
};
struct PlaybackPoint {Tick tick;uint16_t key=0;uint32_t count=0;uint8_t velocity=0,value=0;bool onset=false,control=false;};
struct PrepareResult;
class PreparedSong {
public:
    PreparedSong(PreparedSong&&)noexcept=default;PreparedSong& operator=(PreparedSong&&)noexcept=default;
    PreparedSong(const PreparedSong&)=delete;PreparedSong& operator=(const PreparedSong&)=delete;
    Tick endTick()const noexcept{return end;}
    double durationSeconds()const noexcept{return seconds;}
    size_t storageBytes()const noexcept{return storage;}
    size_t eventCount()const noexcept{return events.size();}
    std::optional<PlaybackPoint> event(size_t index)const noexcept{
        if(index>=events.size())return {};
        const auto& e=events[index];PlaybackPoint p;
        if(e.control){const auto& c=controls[e.index];p.tick=c.tick;p.key=c.key;p.value=c.value;p.control=true;}
        else{const auto& n=notes[e.index];p.tick=n.tick;p.key=n.key;p.count=n.count;p.velocity=n.velocity;p.onset=n.onset;}
        return p;
    }
    size_t firstAfter(Tick tick)const noexcept{
        auto it=std::upper_bound(events.begin(),events.end(),tick,[&](Tick t,const Ref& e){return t<eventTick(e);});return size_t(it-events.begin());
    }
    ChaseState chase(Tick tick)const noexcept{
        ChaseState state;if(tick.value<0||tick>=end)return state;state.presets=presets;
        for(size_t key=0;key<noteLanes.size();++key){
            const auto lane=noteLanes[key];if(!lane.count)continue;
            auto begin=notes.begin()+lane.begin,finish=begin+lane.count;
            auto next=std::upper_bound(begin,finish,tick,[](Tick t,const NotePoint& p){return t<p.tick;});
            if(next!=begin){const auto& n=*std::prev(next);if(n.count)state.notes[state.noteCount++]={uint16_t(key),n.count,n.velocity};}
        }
        std::array<ControlPoint,256> chosen{};size_t count=0;
        for(size_t key=0;key<controlLanes.size();++key){
            const auto lane=controlLanes[key];if(!lane.count)continue;
            auto begin=controls.begin()+lane.begin,finish=begin+lane.count;
            auto next=std::upper_bound(begin,finish,tick,[](Tick t,const ControlPoint& p){return t<p.tick;});
            if(next!=begin)chosen[count++]=*std::prev(next);
        }
        std::sort(chosen.begin(),chosen.begin()+count,[](const auto& a,const auto& b){return a.tick<b.tick||(a.tick==b.tick&&a.order<b.order);});
        for(size_t i=0;i<count;++i){const auto& c=chosen[i];state.controls[state.controlCount++]={uint8_t(c.key/128+1),uint8_t(c.key%128),c.value};}
        return state;
    }
    std::optional<double> secondsAtTick(double tick)const noexcept{
        if(!std::isfinite(tick)||tick<0||tick>double(end.value))return {};
        auto next=std::upper_bound(tempos.begin(),tempos.end(),tick,[](double t,const TempoPoint& p){return t<double(p.tick.value);});
        const auto& p=*std::prev(next);return p.seconds+(tick-double(p.tick.value))/p.tickRate;
    }
    std::optional<double> tickAtSeconds(double value)const noexcept{
        if(!std::isfinite(value)||value<0||value>seconds)return {};
        auto next=std::upper_bound(tempos.begin(),tempos.end(),value,[](double s,const TempoPoint& p){return s<p.seconds;});
        const auto& p=*std::prev(next);return std::min(double(end.value),double(p.tick.value)+(value-p.seconds)*p.tickRate);
    }
private:
    PreparedSong()=default;
    struct Lane {uint32_t begin=0,count=0;};
    struct NotePoint {Tick tick;uint32_t count=0;uint16_t key=0;uint8_t velocity=0;bool onset=false;};
    struct ControlPoint {Tick tick;uint32_t order=0;uint16_t key=0;uint8_t value=0;};
    struct Ref {uint32_t index=0;bool control=false;};
    struct TempoPoint {Tick tick;double seconds=0,tickRate=0;};
    Tick end;double seconds=0;size_t storage=0;
    std::vector<NotePoint> notes;std::vector<ControlPoint> controls;std::vector<Ref> events;std::vector<TempoPoint> tempos;
    std::array<Lane,2048> noteLanes{},controlLanes{};std::array<ChannelPreset,16> presets{};
    Tick eventTick(const Ref& e)const noexcept{return e.control?controls[e.index].tick:notes[e.index].tick;}
    int priority(const Ref& e)const noexcept{return e.control?2:notes[e.index].onset?3:1;}
    friend PrepareResult prepareComposition(const Composition&,const std::atomic<bool>*);
};
struct PrepareResult {std::optional<PreparedSong> song;std::string error;};
inline PrepareResult prepareComposition(const Composition& c,const std::atomic<bool>* cancelled=nullptr){
    auto stop=[&]{return cancelled&&cancelled->load(std::memory_order_relaxed);};
    try{
        if(stop())return {{},"Playback preparation cancelled"};
        auto valid=validate(c);if(!valid)return {{},valid.message};
        if(stop())return {{},"Playback preparation cancelled"};
        size_t count=0,ccCount=0;for(const auto& t:c.tracks){count+=t.notes.size();ccCount+=t.controls.size();}
        PreparedSong song;song.end=c.endTick;song.notes.reserve(2*count);song.controls.reserve(ccCount);song.tempos.reserve(c.tempos.size());
        struct Edge {Tick tick;uint16_t key;uint8_t velocity;bool on;};std::vector<Edge> edges;edges.reserve(2*count);
        uint32_t order=0;
        for(const auto& t:c.tracks){
            if(stop())return {{},"Playback preparation cancelled"};
            auto& preset=song.presets[size_t(t.channel-1)];if(t.program)preset.program=*t.program;if(t.bank){preset.bankMsb=t.bank->msb;preset.bankLsb=t.bank->lsb;}
            for(const auto& n:t.notes){auto key=uint16_t((t.channel-1)*128+n.pitch);edges.push_back({n.start,key,uint8_t(n.velocity),true});edges.push_back({{n.start.value+n.duration.value},key,0,false});}
            for(const auto& cc:t.controls)song.controls.push_back({cc.tick,order++,uint16_t((t.channel-1)*128+cc.controller),uint8_t(cc.value)});
        }
        std::stable_sort(edges.begin(),edges.end(),[](const auto& a,const auto& b){return a.key<b.key||(a.key==b.key&&(a.tick<b.tick||(a.tick==b.tick&&a.on<b.on)));});
        size_t i=0;
        while(i<edges.size()){
            if(stop())return {{},"Playback preparation cancelled"};
            const auto key=edges[i].key;auto& lane=song.noteLanes[key];lane.begin=uint32_t(song.notes.size());uint32_t held=0;uint8_t velocity=0;
            while(i<edges.size()&&edges[i].key==key){
                if((i&4095)==0&&stop())return {{},"Playback preparation cancelled"};
                const auto tick=edges[i].tick;bool onset=false,released=false;
                do{const auto& e=edges[i++];if(e.on){++held;velocity=e.velocity;onset=true;}else if(--held==0)released=true;}while(i<edges.size()&&edges[i].key==key&&edges[i].tick==tick);
                // A final old release must remain before same-tick controllers/new onsets.
                if(released&&onset){song.notes.push_back({tick,0,key,velocity,false});++lane.count;}
                song.notes.push_back({tick,held,key,velocity,onset});++lane.count;
            }
        }
        std::stable_sort(song.controls.begin(),song.controls.end(),[](const auto& a,const auto& b){return a.key<b.key||(a.key==b.key&&a.tick<b.tick);});
        for(size_t n=0;n<song.controls.size();++n){auto& lane=song.controlLanes[song.controls[n].key];if(!lane.count)lane.begin=uint32_t(n);++lane.count;}
        song.events.reserve(song.notes.size()+song.controls.size());
        for(size_t n=0;n<song.notes.size();++n)song.events.push_back({uint32_t(n),false});
        for(size_t n=0;n<song.controls.size();++n)song.events.push_back({uint32_t(n),true});
        std::stable_sort(song.events.begin(),song.events.end(),[&](const auto& a,const auto& b){auto ta=song.eventTick(a),tb=song.eventTick(b);if(ta!=tb)return ta<tb;auto pa=song.priority(a),pb=song.priority(b);if(pa!=pb)return pa<pb;if(a.control&&b.control)return song.controls[a.index].order<song.controls[b.index].order;return false;});
        double seconds=0;
        for(size_t n=0;n<c.tempos.size();++n){const auto& t=c.tempos[n];double rate=480.0*1000000/t.microsPerQuarter;song.tempos.push_back({t.tick,seconds,rate});auto end=n+1<c.tempos.size()?c.tempos[n+1].tick:c.endTick;seconds+=(end.value-t.tick.value)/rate;}
        song.seconds=seconds;
        size_t bytes=sizeof(song);bool fits=detail::vectorBytes(bytes,song.notes)&&detail::vectorBytes(bytes,song.controls)&&detail::vectorBytes(bytes,song.events)&&detail::vectorBytes(bytes,song.tempos);
        const auto document=valid.accountedBytes-count*64-ccCount*32;
        if(!fits||document>limits::documentBytes-bytes)return {{},"Document and prepared playback exceed memory budget"};
        song.storage=bytes;if(stop())return {{},"Playback preparation cancelled"};
        return {std::move(song),{}};
    }catch(const std::bad_alloc&){return {{},"Insufficient memory to prepare playback"};}
}
}
