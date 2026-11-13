#pragma once
#include "Config.h"
#include "Theory.h"

namespace composer {
struct HarmonyPlan {std::vector<Chord> chords;std::vector<Key> keys,contexts;std::vector<int> degrees;std::string error;};
inline HarmonyPlan planHarmony(const Config& config){
    auto checked=validateConfig(config);if(!checked)return {{},{},{},{},checked.message};
    const int64_t barTicks=480LL*4*config.numerator/config.denominator;
    HarmonyPlan plan;plan.keys={{{0},config.root,config.scale}};
    if(config.modulation)plan.keys.push_back({{config.modulation->bar*barTicks},config.modulation->root,config.modulation->scale});
    plan.contexts.resize(size_t(config.bars));plan.degrees.resize(size_t(config.bars));
    const int progressions[5][4]={{0,5,2,6},{0,3,4,0},{0,5,3,4},{0,3,5,4},{0,6,5,4}};
    std::vector<int> previous;
    const int harmonicBars=config.harmonicBars?config.harmonicBars:(config.style==Style::ambient?2:1);
    for(int bar=0;bar<config.bars;){
        Key context=plan.keys[0];if(config.modulation&&bar>=config.modulation->bar)context=plan.keys[1];
        int length=std::min(harmonicBars,config.bars-bar);
        if(config.modulation&&bar<config.modulation->bar)length=std::min(length,config.modulation->bar-bar);
        int degree=progressions[int(config.style)][(bar/harmonicBars)%4];
        if(bar+length==config.bars)degree=0;
        bool borrowed=config.modalInterchange&&bar%4==2&&bar+length<config.bars;
        if(borrowed){context.scale=context.scale==Scale::major?Scale::naturalMinor:Scale::major;degree=3;}
        degree%=int(scaleIntervals(context.scale).size());
        int voices=(config.style==Style::rnb||config.style==Style::ambient)?(config.complexity>=80?5:4):3;
        auto pitches=chordPitches(context,degree,voices,3);
        if(!pitches){plan.error="Requested harmony exceeds pitch range";return plan;}
        Chord chord;chord.start={bar*barTicks};chord.duration={length*barTicks};chord.root=(*pitches)[0]%12;
        for(int pitch:*pitches)chord.pitchClasses.push_back(pitch%12);
        auto voicing=voiceLead(chord.pitchClasses,previous,48,84);
        if(!voicing){plan.error="No valid bounded chord voicing";return plan;}
        chord.voicing=*voicing;previous=*voicing;
        chord.inversion=int(std::find(chord.pitchClasses.begin(),chord.pitchClasses.end(),chord.voicing[0]%12)-chord.pitchClasses.begin());
        chord.tension=bar+length==config.bars?15:std::min(100,25+degree*6+config.complexity/5+(borrowed?15:0));
        if(borrowed)chord.explanation=context.scale==Scale::major?"Modal interchange: IV from parallel major":"Modal interchange: iv from parallel natural minor";
        for(int i=bar;i<bar+length;++i){plan.contexts[size_t(i)]=context;plan.degrees[size_t(i)]=degree;}
        plan.chords.push_back(std::move(chord));bar+=length;
    }
    return plan;
}
}
