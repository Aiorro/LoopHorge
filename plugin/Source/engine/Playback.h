#pragma once
#include "Prepared.h"
#include <type_traits>

namespace composer {
enum class Clock {hostQuarterBeats,compositionSeconds};
struct PlaybackBlock {
    Clock clock=Clock::hostQuarterBeats;double position=0,sampleRate=48000,bpm=120,hostStartQuarterBeat=0;
    int samples=64;bool playing=true;
};
struct MidiPacket {uint8_t status=0,data1=0,data2=0,size=3;};
struct PlaybackResult {double nextPosition=0;int messages=0,events=0;bool overflow=false,invalidClock=false,ended=false;};
class Playback {
public:
    template<class Sink>PlaybackResult render(const PreparedSong& song,const PlaybackBlock& b,Sink&& sink)noexcept{
        static_assert(std::is_nothrow_invocable_v<Sink&,MidiPacket,int>,"Playback sink must be nonthrowing and preallocated");
        PlaybackResult r;r.nextPosition=std::isfinite(b.position)?b.position:0;
        if(b.samples==0)return r;
        const bool host=b.clock==Clock::hostQuarterBeats;
        const double origin=host?b.hostStartQuarterBeat:0;
        const double start=b.position-origin;
        const double step=host?b.bpm/(60*b.sampleRate):1/b.sampleRate;
        const double next=b.position+b.samples*step;
        if(b.samples<0||(b.clock!=Clock::hostQuarterBeats&&b.clock!=Clock::compositionSeconds)
           ||!std::isfinite(b.position)||!std::isfinite(origin)||!std::isfinite(start)||!std::isfinite(next)
           ||!std::isfinite(b.sampleRate)||b.sampleRate<1||b.sampleRate>768000
           ||(host&&(!std::isfinite(b.bpm)||b.bpm<1||b.bpm>1000))){
            flush(sink,r,0);have=false;inSong=false;paused=false;r.invalidClock=true;return r;
        }
        const double tolerance=std::max({1e-12,lastStep*1e-6,step*1e-6,std::abs(b.position)*std::numeric_limits<double>::epsilon()*8});
        const bool reset=!have||lastSong!=&song||lastClock!=b.clock||lastOrigin!=origin||!wasPlaying||std::abs(b.position-lastNext)>tolerance;
        lastSong=&song;lastClock=b.clock;lastOrigin=origin;lastNext=next;lastStep=step;have=true;wasPlaying=b.playing;
        r.nextPosition=b.playing?next:b.position;
        if(!b.playing){flush(sink,r,0);inSong=false;paused=false;return r;}
        if(reset){flush(sink,r,0);inSong=false;paused=false;}
        if(paused){r.overflow=true;return r;}
        const double end=host?song.endTick().value/480.0:song.durationSeconds();
        auto offset=[&](double at){double sample=std::ceil((at-start)/step-1e-7);if(sample<0)return 0;if(sample>=b.samples)return b.samples;return int(sample);};
        if(start>=end){flush(sink,r,0);inSong=false;r.ended=true;if(!host){r.nextPosition=end;lastNext=end;}return r;}
        if(!inSong){
            const double target=std::max(0.0,start);const int sample=offset(target);if(sample>=b.samples)return r;
            const double tick=host?target*480:*song.tickAtSeconds(target);
            const Tick at{int64_t(std::floor(tick))};const auto state=song.chase(at);
            for(int ch=0;ch<16;++ch){const auto& p=state.presets[size_t(ch)];
                if(p.bankMsb>=0){emit(sink,r,{uint8_t(0xb0|ch),0,uint8_t(p.bankMsb),3},sample);emit(sink,r,{uint8_t(0xb0|ch),32,uint8_t(p.bankLsb),3},sample);}
                if(p.program>=0)emit(sink,r,{uint8_t(0xc0|ch),uint8_t(p.program),0,2},sample);
            }
            for(size_t i=0;i<state.controlCount;++i){const auto& c=state.controls[i];emit(sink,r,{uint8_t(0xb0|(c.channel-1)),c.controller,c.value,3},sample);}
            for(size_t i=0;i<state.noteCount;++i){const auto& n=state.notes[i];held[n.key]=n.count;emit(sink,r,{uint8_t(0x90|(n.key/128)),uint8_t(n.key%128),n.velocity,3},sample);}
            cursor=song.firstAfter(at);inSong=true;
        }
        while(auto point=song.event(cursor)){
            if(point->tick>=song.endTick())break;
            const double at=host?point->tick.value/480.0:*song.secondsAtTick(double(point->tick.value));const int sample=offset(at);
            if(sample>=b.samples)break;
            const auto key=point->key;int needed=point->control?1:point->onset?(held[key]?2:1):(!point->count&&held[key]?1:0);
            if(r.events>=ordinaryLimit||r.messages+needed>ordinaryLimit){flush(sink,r,sample);paused=true;inSong=false;r.overflow=true;return r;}
            ++r.events;++cursor;const auto channel=uint8_t(key/128);
            if(point->control)emit(sink,r,{uint8_t(0xb0|channel),uint8_t(key%128),point->value,3},sample);
            else{
                if(held[key]&&(point->onset||!point->count))emit(sink,r,{uint8_t(0x80|channel),uint8_t(key%128),0,3},sample);
                if(point->onset)emit(sink,r,{uint8_t(0x90|channel),uint8_t(key%128),point->velocity,3},sample);
                held[key]=point->count;
            }
        }
        const int finish=offset(end);
        if(finish<b.samples){flush(sink,r,finish);inSong=false;cursor=song.eventCount();r.ended=true;if(!host){r.nextPosition=end;lastNext=end;}}
        return r;
    }
private:
    static constexpr int ordinaryLimit=2048-384;
    std::array<uint32_t,2048> held{};uint16_t touched=0;size_t cursor=0;
    const PreparedSong* lastSong=nullptr;Clock lastClock=Clock::hostQuarterBeats;
    double lastOrigin=0,lastNext=0,lastStep=0;bool have=false,wasPlaying=false,inSong=false,paused=false;
    template<class Sink>void emit(Sink& sink,PlaybackResult& r,MidiPacket packet,int sample)noexcept{sink(packet,sample);++r.messages;touched|=uint16_t(1u<<(packet.status&15));}
    template<class Sink>void flush(Sink& sink,PlaybackResult& r,int sample)noexcept{
        for(size_t key=0;key<held.size();++key)if(held[key]){emit(sink,r,{uint8_t(0x80|(key/128)),uint8_t(key%128),0,3},sample);held[key]=0;}
        for(int ch=0;ch<16;++ch)if(touched&(1u<<ch))for(int cc:{64,66,69})emit(sink,r,{uint8_t(0xb0|ch),uint8_t(cc),0,3},sample);
        touched=0;
    }
};
}
