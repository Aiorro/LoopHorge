#include "../Source/engine/Mood.h"
#include "../Source/engine/Generation.h"
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace composer;
#define check(x) do {if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
uint64_t notes(const Composition& c){
    uint64_t hash=14695981039346656037ULL;
    for(const auto& t:c.tracks)for(const auto& n:t.notes)for(auto value:{int64_t(n.pitch),int64_t(n.velocity),n.start.value,n.duration.value})
        for(int i=0;i<8;++i){hash^=uint64_t(value)>>(i*8)&255;hash*=1099511628211ULL;}
    return hash;
}
double averagePitch(const Track& t){double sum=0;for(const auto& n:t.notes)sum+=n.pitch;return sum/t.notes.size();}
double averageVelocity(const Track& t){double sum=0;for(const auto& n:t.notes)sum+=n.velocity;return sum/t.notes.size();}
int64_t gates(const Track& t){int64_t sum=0;for(const auto& n:t.notes)sum+=n.duration.value;return sum;}
int main(){
    Config base;base.seed=42;base.humanization=0;
    Mood neutral;
    auto ordinary=generateLoop(base),same=generateMoodLoop(base,neutral);
    check(same.composition&&notes(*same.composition)==notes(*ordinary.composition));check(same.composition->engineVersion==4);
    auto pair=[&](int Mood::*axis){Mood low,high;low.*axis=0;high.*axis=100;
        auto a=generateMoodLoop(base,low),b=generateMoodLoop(base,high);check(a.composition&&b.composition);
        check(validate(*a.composition)&&validate(*b.composition));check(notes(*a.composition)!=notes(*b.composition));
        check(a.composition->id!=b.composition->id);return std::pair{*a.composition,*b.composition};};
    auto dark=pair(&Mood::darkness);check(averagePitch(dark.first.tracks[2])>averagePitch(dark.second.tracks[2]));
    auto energy=pair(&Mood::energy);check(energy.first.tracks[0].notes.size()<energy.second.tracks[0].notes.size());
    check(averageVelocity(energy.first.tracks[2])<averageVelocity(energy.second.tracks[2]));
    pair(&Mood::melancholy);
    auto tension=pair(&Mood::tension);check(tension.first.tracks[1].notes.size()<tension.second.tracks[1].notes.size());
    auto aggression=pair(&Mood::aggression);check(averageVelocity(aggression.first.tracks[0])<averageVelocity(aggression.second.tracks[0]));
    check(gates(aggression.first.tracks[0])>gates(aggression.second.tracks[0]));
    auto atmosphere=pair(&Mood::atmosphere);check(gates(atmosphere.first.tracks[1])<gates(atmosphere.second.tracks[1]));
    check(atmosphere.first.tracks[2].notes.size()>atmosphere.second.tracks[2].notes.size());
    auto complexity=pair(&Mood::complexity);check(complexity.first.tracks[2].notes.size()<complexity.second.tracks[2].notes.size());
    check(base.performance.octaveOffset==0&&base.complexity==50);
    for(auto axis:{&Mood::darkness,&Mood::energy,&Mood::melancholy,&Mood::tension,&Mood::aggression,&Mood::atmosphere,&Mood::complexity}){
        Mood invalid;invalid.*axis=-1;check(!deriveMood(invalid)&&!generateMoodLoop(base,invalid).composition);
        invalid.*axis=101;check(!deriveMood(invalid)&&!generateMoodLoop(base,invalid).composition);
        invalid.*axis=std::numeric_limits<int>::min();check(!deriveMood(invalid));
    }
    {auto bad=base;bad.complexity=101;check(!generateMoodLoop(bad,neutral).composition);bad=base;bad.performance.gatePercent=0;check(!generateMoodLoop(bad,neutral).composition);}
    auto reject=[&](auto mutation){auto bad=base;mutation(bad.performance);check(!generateLoop(bad).composition);};
    reject([](auto& p){p.octaveOffset=2;});reject([](auto& p){p.velocityOffset=std::numeric_limits<int>::max();});
    reject([](auto& p){p.gatePercent=0;});reject([](auto& p){p.chordGatePercent=151;});
    reject([](auto& p){p.bassStride=0;});reject([](auto& p){p.leadStride=3;});
    reject([](auto& p){p.extraChordVoices=3;});reject([](auto& p){p.tensionOffset=-51;});reject([](auto& p){p.contour=Contour(99);});
    for(int corner=0;corner<128;++corner)for(auto feel:{Feel::half,Feel::normal,Feel::doubleTime})for(int style=0;style<5;++style){
        Mood mood;int i=0;for(auto axis:{&Mood::darkness,&Mood::energy,&Mood::melancholy,&Mood::tension,&Mood::aggression,&Mood::atmosphere,&Mood::complexity})mood.*axis=corner&(1<<i++)?100:0;
        auto config=base;config.drums=true;config.feel=feel;config.style=Style(style);config.root=11;config.scale=Scale(corner%11);
        config.numerator=1;config.denominator=16;config.swing=100;config.humanization=100;
        auto r=generateMoodLoop(config,mood);if(!r.composition)std::cerr<<"corner="<<corner<<" "<<r.error<<"\n";
        check(r.composition&&validate(*r.composition));check(r.composition->id.size()<=128);
        for(const auto& t:r.composition->tracks)for(const auto& n:t.notes)check(n.pitch>=t.lowPitch&&n.pitch<=t.highPitch&&n.velocity>0&&n.duration.value>0&&n.start.value+n.duration.value<=r.composition->endTick.value);
    }
    std::cout<<"seven mood axes: real musical effects, neutral equivalence and1920 joint extremes passed\n";
}
