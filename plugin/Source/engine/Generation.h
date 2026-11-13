#pragma once
#include "Harmony.h"
#include "Rhythm.h"

namespace composer {
struct GenerationResult {std::optional<Composition> composition;std::string error;};
namespace musicDetail {
inline std::string configId(const Config& c){
    std::string id="loop-v3-";
    // Injective byte encoding of validated bounded fields; no hash or host-endian dependency.
    auto byte=[&](unsigned value){constexpr char hex[]="0123456789abcdef";id+=hex[(value>>4)&15];id+=hex[value&15];};
    for(int value:{c.root,int(c.scale),int(c.style),c.bpm,c.bars,c.numerator,c.denominator,c.complexity,c.humanization,c.swing,c.harmonicBars,int(c.modalInterchange),int(c.drums),int(c.ghostNotes),int(c.drumFills),int(c.feel)})byte(unsigned(value));
    for(int shift=0;shift<32;shift+=8)byte((c.seed>>shift)&255);
    const auto& p=c.performance;
    for(int value:{p.octaveOffset+1,p.velocityOffset+32,p.gatePercent,p.chordGatePercent,p.bassStride,p.leadStride,p.extraChordVoices+1,p.tensionOffset+50,int(p.contour)})byte(unsigned(value));
    byte(c.modulation.has_value());
    if(c.modulation)for(int value:{c.modulation->bar,c.modulation->root,int(c.modulation->scale)})byte(unsigned(value));
    return id;
}
inline const Chord& at(const std::vector<Chord>& chords,int64_t tick){
    auto it=std::upper_bound(chords.begin(),chords.end(),tick,[](int64_t t,const Chord& h){return t<h.start.value;});
    return *std::prev(it);
}
inline int registerPitch(int pitch,int previous,int low,int high){
    int best=pitch,bestDistance=1000;
    for(int candidate=pitch%12;candidate<=127;candidate+=12)if(candidate>=low&&candidate<=high){
        int distance=std::abs(candidate-previous);
        if(distance<bestDistance){bestDistance=distance;best=candidate;}
    }
    return best;
}
}

inline GenerationResult generateLoop(const Config& config){
    auto checked=validateConfig(config);if(!checked)return {{},checked.message};
    try {
        auto harmony=planHarmony(config);if(!harmony.error.empty())return {{},harmony.error};
        const int64_t barTicks=480LL*4*config.numerator/config.denominator;
        Composition c;c.id=musicDetail::configId(config);c.engineVersion=3;c.seed=config.seed;c.style=styleName(config.style);c.source="native-motif-rules-v3";
        c.endTick={config.bars*barTicks};c.tempos={{{0},int(std::llround(60000000.0/config.bpm))}};
        c.meters={{{0},config.numerator,config.denominator}};c.keys=harmony.keys;c.chords=harmony.chords;
        Id id=1;
        for(auto& h:c.chords)h.id=id++;
        c.sections={{id++,"Loop phrase",{0},c.endTick,SectionRole::verse}};
        Track bass,chords,lead;
        const int shift=config.performance.octaveOffset*12;
        bass.id=id++;bass.name="Bass";bass.role=Role::bass;bass.channel=1;bass.lowPitch=28+shift;bass.highPitch=52+shift;
        chords.id=id++;chords.name="Chords";chords.role=Role::chords;chords.channel=2;chords.lowPitch=48+shift;chords.highPitch=84+shift;
        lead.id=id++;lead.name="Lead";lead.role=Role::lead;lead.channel=3;lead.lowPitch=60+shift;lead.highPitch=96+shift;
        for(const auto& h:c.chords){
            const int sustain=config.style==Style::ambient?98:config.style==Style::nuMetal?55:88;
            const int64_t duration=std::clamp(h.duration.value*sustain*config.performance.gatePercent*config.performance.chordGatePercent/1000000,int64_t(1),h.duration.value);
            for(int pitch:h.voicing)chords.notes.push_back({id++,pitch,std::clamp(72+config.performance.velocityOffset,1,127),{h.start},{duration}});
        }
        const std::vector<int> bassUnits[]={{0,6,10,14},{0,7,12},{0,5,9,14},{0,8},{0,2,4,7,8,10,14}};
        const std::vector<int> melodyUnits[]={{0,1,3,4,6,7},{0,3,4,7},{0,2,3,4,6,7},{0,3,6},{0,2,3,4,7}};
        // One motif family per phrase; odd bars answer, later phrases preserve its contour.
        const int motif[][2][6]={{{0,2,4,2,1,0},{0,1,2,4,2,0}},{{0,2,1,4,2,0},{0,4,2,1,2,0}}};
        const auto family=musicDetail::seedFor(config.seed,3,0)%2;
        int previous=72+shift;
        for(int bar=0;bar<config.bars;++bar){
            const int64_t start=bar*barTicks;
            const auto& h=musicDetail::at(c.chords,start);
            musicDetail::Random bassRandom(musicDetail::seedFor(config.seed,1,uint32_t(bar)));
            const auto& units=bassUnits[int(config.style)];
            auto bassGrid=musicDetail::rhythmGrid(units,16,config.feel,bar);
            for(size_t i=0;i<bassGrid.pulses.size();++i){
                const auto& pulse=bassGrid.pulses[i];int original=units[pulse.sourceIndex];
                if(pulse.sourceIndex%size_t(config.performance.bassStride)!=0&&pulse.sourceIndex+1!=units.size())continue;
                int64_t tick=musicDetail::onset(pulse.unit,bassGrid.divisions,barTicks,config,bassRandom,original,bassGrid.stepMultiplier);
                int64_t next=i+1<bassGrid.pulses.size()?int64_t(bassGrid.pulses[i+1].unit)*barTicks/bassGrid.divisions:barTicks;
                int64_t duration=std::max<int64_t>(1,(next-tick)*(config.style==Style::ambient?95:78)/100);
                duration=std::clamp(duration*config.performance.gatePercent/100,int64_t(1),barTicks-tick);
                int velocity=std::clamp((pulse.sourceIndex==0?108:original%4?76:94)+bassRandom.offset(config.humanization/10)+config.performance.velocityOffset,1,127);
                bass.notes.push_back({id++,36+h.root+shift,velocity,{start+tick},{duration}});
            }
            musicDetail::Random leadRandom(musicDetail::seedFor(config.seed,3,uint32_t(bar)));
            auto onsets=melodyUnits[int(config.style)];int divisions=8;
            if(config.complexity<25)onsets={0,4,7};
            if(config.complexity>=90){onsets={0,1,2,3,4};divisions=5;}
            auto leadGrid=musicDetail::rhythmGrid(onsets,divisions,config.feel,bar);
            for(size_t i=0;i<leadGrid.pulses.size();++i){
                const auto& pulse=leadGrid.pulses[i];
                if(pulse.sourceIndex%size_t(config.performance.leadStride)!=0&&pulse.sourceIndex+1!=onsets.size())continue;
                const int phraseBar=config.feel==Feel::half?bar/2:config.feel==Feel::doubleTime?bar*2+pulse.cycle:bar;
                int offset=motif[family][phraseBar%2][pulse.sourceIndex%6];
                if(config.performance.contour==Contour::rising)offset=int(pulse.sourceIndex);
                if(config.performance.contour==Contour::falling)offset=int(onsets.size()-pulse.sourceIndex-1);
                if(pulse.sourceIndex==0||pulse.sourceIndex+1==onsets.size())offset=0;
                auto pitch=degreePitch(harmony.contexts[size_t(bar)],harmony.degrees[size_t(bar)]+offset,4+config.performance.octaveOffset);
                if(!pitch)return {{},"Motif exceeds supported pitch range"};
                int target=*pitch;
                if(std::abs(target-previous)>12)target=musicDetail::registerPitch(target,previous,lead.lowPitch,lead.highPitch);
                previous=target;
                int64_t tick=musicDetail::onset(pulse.unit,leadGrid.divisions,barTicks,config,leadRandom,onsets[pulse.sourceIndex],leadGrid.stepMultiplier);
                int64_t next=i+1<leadGrid.pulses.size()?int64_t(leadGrid.pulses[i+1].unit)*barTicks/leadGrid.divisions:barTicks;
                int64_t duration=std::clamp((next-tick)*80*config.performance.gatePercent/10000,int64_t(1),barTicks-tick);
                int velocity=std::clamp((pulse.sourceIndex==0?94:78)+leadRandom.offset(config.humanization/8)+config.performance.velocityOffset,1,127);
                lead.notes.push_back({id++,target,velocity,{start+tick},{duration}});
            }
        }
        c.tracks={std::move(bass),std::move(chords),std::move(lead)};
        if(config.drums){
            auto drums=generateDrums(config);if(!drums.track)return {{},drums.error};
            drums.track->id=id++;
            for(auto& note:drums.track->notes)note.id=id++;
            c.tracks.push_back(std::move(*drums.track));
        }
        auto validation=validate(c);if(!validation)return {{},validation.message};
        return {std::move(c),{}};
    }catch(const std::bad_alloc&){return {{},"Not enough memory to generate composition"};}
}
inline GenerationResult generateMoodLoop(Config config,const Mood& mood){
    auto checked=validateConfig(config);if(!checked)return {{},checked.message};
    auto performance=deriveMood(mood);if(!performance)return {{},"Mood axes must be between0 and100"};
    config.performance=*performance;config.complexity=mood.complexity;
    return generateLoop(config);
}
}
