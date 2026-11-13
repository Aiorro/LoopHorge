#pragma once
#include "Composition.h"
#include <algorithm>
#include <cstdlib>
#include <limits>

namespace composer {
inline std::vector<int> scaleIntervals(Scale scale,bool descending=false){
    switch(scale){
        case Scale::major:return {0,2,4,5,7,9,11};
        case Scale::naturalMinor:return {0,2,3,5,7,8,10};
        case Scale::harmonicMinor:return {0,2,3,5,7,8,11};
        case Scale::melodicMinor:return descending?std::vector<int>{0,2,3,5,7,8,10}:std::vector<int>{0,2,3,5,7,9,11};
        case Scale::dorian:return {0,2,3,5,7,9,10};
        case Scale::phrygian:return {0,1,3,5,7,8,10};
        case Scale::lydian:return {0,2,4,6,7,9,11};
        case Scale::mixolydian:return {0,2,4,5,7,9,10};
        case Scale::majorPentatonic:return {0,2,4,7,9};
        case Scale::minorPentatonic:return {0,3,5,7,10};
        case Scale::blues:return {0,3,5,6,7,10};
    }
    return {};
}

inline std::optional<int> degreePitch(const Key& key,int degree,int octave,bool descending=false){
    if(key.root<0||key.root>11||degree< -1024||degree>1024||octave< -1||octave>9)return {};
    auto intervals=scaleIntervals(key.scale,descending);if(intervals.empty())return {};
    const int count=int(intervals.size());int octaves=degree/count,index=degree%count;
    if(index<0){index+=count;--octaves;}
    int pitch=(octave+1)*12+key.root+octaves*12+intervals[size_t(index)];
    if(pitch<0||pitch>127)return {};
    return pitch;
}

inline std::optional<std::vector<int>> chordPitches(const Key& key,int degree,int voices,int octave){
    if(voices<3||voices>7||degree< -1024||degree>1012)return {};
    std::vector<int> result;std::bitset<12> classes;
    for(int i=0;i<voices;++i){
        auto pitch=degreePitch(key,degree+i*2,octave);if(!pitch)return {};
        if(!classes.test(size_t(*pitch%12))){result.push_back(*pitch);classes.set(size_t(*pitch%12));}
    }
    return result;
}

// Enumerate closed-position inversions over MIDI octaves; no platform-dependent random tie-break.
inline std::optional<std::vector<int>> voiceLead(const std::vector<int>& pitchClasses,const std::vector<int>& previous,int low,int high){
    if(pitchClasses.empty()||pitchClasses.size()>12||previous.size()>12||low<0||high>127||low>high)return {};
    std::bitset<12> seen;
    for(int pc:pitchClasses){if(pc<0||pc>11||seen.test(size_t(pc)))return {};seen.set(size_t(pc));}
    for(int pitch:previous)if(pitch<0||pitch>127)return {};
    auto pcs=pitchClasses,prior=previous;std::sort(pcs.begin(),pcs.end());std::sort(prior.begin(),prior.end());
    std::optional<std::vector<int>> best;int bestScore=std::numeric_limits<int>::max();
    for(size_t inversion=0;inversion<pcs.size();++inversion)for(int base=-12;base<=120;base+=12){
        std::vector<int> notes;int score=0;
        for(size_t i=0;i<pcs.size();++i){
            size_t index=(inversion+i)%pcs.size();int pitch=base+pcs[index]+(inversion+i>=pcs.size()?12:0);
            if(pitch<low||pitch>high){notes.clear();break;}
            int reference=prior.empty()?(low+high)/2:prior[std::min(i,prior.size()-1)];
            score+=std::abs(pitch-reference);notes.push_back(pitch);
        }
        if(!notes.empty()&&(score<bestScore||(score==bestScore&&(!best||notes<*best)))){bestScore=score;best=std::move(notes);}
    }
    return best;
}
}
