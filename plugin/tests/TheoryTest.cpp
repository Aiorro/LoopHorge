#include "../Source/engine/Theory.h"
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace composer;
#define check(x) do {if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
int main(){
    const std::vector<int> major{0,2,4,5,7,9,11},minor{0,2,3,5,7,8,10};
    check(scaleIntervals(Scale::major)==major);check(scaleIntervals(Scale::naturalMinor)==minor);
    check(scaleIntervals(Scale::harmonicMinor)==std::vector<int>({0,2,3,5,7,8,11}));
    check(scaleIntervals(Scale::melodicMinor)==std::vector<int>({0,2,3,5,7,9,11}));
    check(scaleIntervals(Scale::melodicMinor,true)==minor);
    check(scaleIntervals(Scale::dorian)==std::vector<int>({0,2,3,5,7,9,10}));
    check(scaleIntervals(Scale::phrygian)==std::vector<int>({0,1,3,5,7,8,10}));
    check(scaleIntervals(Scale::lydian)==std::vector<int>({0,2,4,6,7,9,11}));
    check(scaleIntervals(Scale::mixolydian)==std::vector<int>({0,2,4,5,7,9,10}));
    check(scaleIntervals(Scale::majorPentatonic)==std::vector<int>({0,2,4,7,9}));
    check(scaleIntervals(Scale::minorPentatonic)==std::vector<int>({0,3,5,7,10}));
    check(scaleIntervals(Scale::blues)==std::vector<int>({0,3,5,6,7,10}));
    check(scaleIntervals(static_cast<Scale>(99)).empty());
    for(int root=0;root<12;++root)for(int scale=0;scale<=int(Scale::blues);++scale){
        Key key{{0},root,Scale(scale)};auto intervals=scaleIntervals(key.scale);
        for(int degree=-10;degree<15;++degree){auto pitch=degreePitch(key,degree,4);check(pitch&&*pitch>=0&&*pitch<=127);
            int relative=(*pitch-root+120)%12;check(std::find(intervals.begin(),intervals.end(),relative)!=intervals.end());}
        auto notes=chordPitches(key,0,7,3);check(notes&&notes->size()>=3);
        std::bitset<12> seen;for(int p:*notes){check(!seen.test(size_t(p%12)));seen.set(size_t(p%12));}
    }
    Key c{{0},0,Scale::major};check(*degreePitch(c,-1,4)==59);check(*degreePitch(c,7,4)==72);
    check(chordPitches(c,0,3,4)==std::vector<int>({60,64,67}));
    check(chordPitches(c,0,4,4)==std::vector<int>({60,64,67,71}));
    check(chordPitches(c,0,5,4)==std::vector<int>({60,64,67,71,74}));
    check(chordPitches(Key{{0},11,Scale::major},0,3,3)==std::vector<int>({59,63,66}));
    check(chordPitches(Key{{0},0,Scale::majorPentatonic},0,7,3)->size()==5);
    check(!degreePitch(c,std::numeric_limits<int>::max(),4));check(!degreePitch(c,-1025,4));
    check(!degreePitch(c,0,10));check(!degreePitch(c,-1,-1));
    check(!degreePitch(Key{{0},12,Scale::major},0,4));check(!chordPitches(c,0,2,4));
    check(!chordPitches(c,0,7,9));
    auto led=voiceLead({5,9,0},{60,64,67},48,84);check(led==std::vector<int>({60,65,69}));
    check(voiceLead({0,4,7},{60,64,67},48,84)==std::vector<int>({60,64,67}));
    check(!voiceLead({0,0,7},{},48,84));check(!voiceLead({12},{},0,127));
    check(!voiceLead({0,4,7},{},60,64));check(!voiceLead({0,4,7},{},84,48));
    check(!voiceLead({0,4,7},{128},48,84));
    std::cout<<"native theory:12roots,11scales, extensions, bounds and voice leading passed\n";
}
