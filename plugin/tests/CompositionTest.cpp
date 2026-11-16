#include "../Source/engine/Time.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <source_location>

using namespace composer;
#define check(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " #x "\n"; std::abort(); } } while (false)

Composition fixture() {
    Composition c;
    c.id = "fixture"; c.endTick = {1920};
    c.tempos = {{{0}, 500000}}; c.meters = {{{0}, 4, 4}};
    c.keys = {{{0}, 0, Scale::naturalMinor}};
    Track t; t.id = 1; t.name = "Bass"; t.role = Role::bass; t.channel = 1;
    t.notes = {{2, 60, 90, {0}, {480}}}; c.tracks = {t};
    return c;
}

template<class Mutation> void rejects(Mutation mutate, std::source_location where=std::source_location::current()) {
    auto c = fixture(); mutate(c); if(validate(c)){std::cerr<<"accepted invalid mutation at "<<where.line()<<"\n";std::abort();}
}

int main() {
    check(validate(fixture()));
    rejects([](auto& c){c.schemaVersion=1;});
    rejects([](auto& c){c.ppq=960;});
    rejects([](auto& c){c.engineVersion=0;});
    rejects([](auto& c){c.id.clear();});
    rejects([](auto& c){c.endTick={0};});
    rejects([](auto& c){c.endTick={std::numeric_limits<int64_t>::max()};});
    rejects([](auto& c){c.tempos.clear();});
    rejects([](auto& c){c.tempos[0].tick={1};});
    rejects([](auto& c){c.tempos[0].microsPerQuarter=0;});
    rejects([](auto& c){c.tempos[0].microsPerQuarter=16777216;});
    rejects([](auto& c){c.tempos.push_back(c.tempos[0]);});
    rejects([](auto& c){c.tempos.push_back({{1921},500000});});
    rejects([](auto& c){c.meters[0].denominator=3;});
    rejects([](auto& c){c.meters[0].numerator=13;});
    rejects([](auto& c){c.meters.push_back({{480},3,4});});
    rejects([](auto& c){c.keys[0].root=12;});
    rejects([](auto& c){c.keys[0].scale=static_cast<Scale>(999);});
    rejects([](auto& c){c.keys[0].tick={-1};});
    rejects([](auto& c){c.tracks.clear();});
    rejects([](auto& c){c.tracks.resize(17,c.tracks[0]);});
    rejects([](auto& c){c.tracks[0].id=0;});
    rejects([](auto& c){c.tracks[0].notes[0].id=1;});
    rejects([](auto& c){c.tracks[0].channel=0;});
    rejects([](auto& c){c.tracks[0].channel=17;});
    rejects([](auto& c){c.tracks[0].channel=10;});
    rejects([](auto& c){c.tracks[0].role=static_cast<Role>(999);});
    rejects([](auto& c){auto t=c.tracks[0];t.id=3;t.notes[0].id=4;c.tracks.push_back(t);});
    rejects([](auto& c){c.tracks[0].program=128;});
    rejects([](auto& c){c.tracks[0].bank=Bank{0,128};});
    rejects([](auto& c){c.tracks[0].lowPitch=61;});
    rejects([](auto& c){c.tracks[0].notes[0].pitch=128;});
    rejects([](auto& c){c.tracks[0].notes[0].velocity=0;});
    rejects([](auto& c){c.tracks[0].notes[0].start={-1};});
    rejects([](auto& c){c.tracks[0].notes[0].duration={0};});
    rejects([](auto& c){c.tracks[0].notes[0].duration={std::numeric_limits<int64_t>::max()};});
    rejects([](auto& c){c.tracks[0].notes[0].start={1900};});
    rejects([](auto& c){c.tracks[0].controls={{3,{0},64,128}};});
    rejects([](auto& c){c.tracks[0].controls={{3,{0},120,0}};});
    rejects([](auto& c){c.tracks[0].controls={{3,{0},32,0}};});
    rejects([](auto& c){c.tracks[0].controls={{3,{1921},64,0}};});
    rejects([](auto& c){for(int i=1;i<=17;++i)c.tracks[0].controls.push_back({uint64_t(i+2),{0},i,0});});
    rejects([](auto& c){c.legacyJson.resize(4*1024*1024+1);});
    rejects([](auto& c){c.tracks[0].notes.resize(131073);});
    rejects([](auto& c){c.tracks[0].notes.reserve(limits::documentBytes/sizeof(Note)+1);});
    rejects([](auto& c){c.tempos.resize(4097);});

    auto c=fixture(); c.endTick={960}; c.tempos.push_back({{480},1000000});
    check(validate(c)); check(std::abs(secondsAt(c,{960})->value-1.5)<1e-12);
    check(tickAt(c,{1.5})->value==960); check(tickAt(c,{.75})->value==600);
    check(!secondsAt(c,{-1})); check(!secondsAt(c,{961}));
    check(!tickAt(c,{-1})); check(!tickAt(c,{2}));
    check(!tickAt(c,{std::numeric_limits<double>::quiet_NaN()}));
    check(!tickAt(c,{std::numeric_limits<double>::infinity()}));
    for(int64_t i=0;i<=960;++i)check(tickAt(c,*secondsAt(c,{i}))->value==i);
    { auto many=fixture();many.endTick={8160};many.tempos.clear();
      for(int64_t i=0;i<4080;++i)many.tempos.push_back({{i*2},500000+int(i%997)});
      check(validate(many));
      for(int64_t i=0;i<=8160;++i){auto inverse=tickAt(many,*secondsAt(many,{i}));check(inverse&&inverse->value==i);}
    }
    c.tempos[0].microsPerQuarter=0;check(!secondsAt(c,{1}));check(!tickAt(c,{.1}));
    c=fixture();c.meters={{{0},6,8},{{1440},3,4}};check(validate(c));
    c.meters[1].tick={1439};check(!validate(c));
    // Third change is aligned relative to the preceding meter origin, not absolute tick0.
    c=fixture();c.endTick={6240};c.meters={{{0},4,4},{{1920},3,4},{{3360},5,8}};check(validate(c));
    c.meters[2].tick={4320};check(validate(c).code==ValidationCode::maps);
    c.meters[2].tick={3361};check(validate(c).code==ValidationCode::maps);
    c=fixture();c.endTick={480*3601LL};check(!validate(c));

    // Lifetime count, not NoteOn count, controls the distinct active-key limit.
    c=fixture();c.tracks[0].notes.clear();
    uint64_t id=20;
    for(int channel=1;channel<=3;++channel){
        Track t;t.id=id++;t.name="dense";t.channel=channel;
        for(int pitch=0;pitch<128;++pitch)t.notes.push_back({id++,pitch,90,{0},{480}});
        c.tracks.push_back(t);
    }
    c.tracks.erase(c.tracks.begin()); c.tracks.pop_back(); check(validate(c));
    auto t=c.tracks.back();t.id=id++;t.channel=3;t.notes={Note{id++,60,90,{0},{480}}};
    c.tracks.push_back(t);check(validate(c).code==ValidationCode::density);
    c.tracks.back().notes[0].start={480}; check(validate(c));
    c=fixture();for(size_t i=1;i<limits::notes;++i)c.tracks[0].notes.push_back({uint64_t(i+2),60,90,{0},{480}});
    check(validate(c));

    c=fixture();c.endTick={172800};c.tracks[0].notes.clear();
    for(int i=0;i<24000;++i)c.tracks[0].notes.push_back({uint64_t(i+2),60+i%12,90,{i*7},{5}});
    check(validate(c));check(std::abs(secondsAt(c,c.endTick)->value-180.0)<1e-9);

    c=fixture();c.tracks.clear();
    Track drums;drums.id=1;drums.channel=10;drums.name="Drums";drums.role=Role::drums;
    drums.ownedPitches.set(36);drums.notes={{2,36,100,{0},{120}}};
    auto percussion=drums;percussion.id=3;percussion.name="Percussion";percussion.role=Role::percussion;
    percussion.ownedPitches.reset();percussion.ownedPitches.set(60);percussion.notes={{4,60,80,{0},{120}}};
    c.tracks={drums,percussion};check(validate(c));
    c.tracks[1].ownedPitches.set(36);check(!validate(c));
    c.tracks[1]=percussion;c.tracks[0].controls={{5,{0},7,100}};check(!validate(c));
    c.tracks[0].controls.clear();c.tracks[0].program=0;check(!validate(c));
    c.tracks[0].program.reset();c.tracks[1].notes[0].pitch=61;check(!validate(c));

    c=fixture();c.sections={{3,"intro",{0},{1920},SectionRole::intro}};
    c.chords={{4,{0},{1920},0,{0,3,7},{48,51,55},0,30,""}};check(validate(c));
    c.chords[0].pitchClasses={0,12};check(!validate(c));
    c.chords.clear();c.sections[0].end={1921};check(!validate(c));
    std::cout<<"composition-v2 validation, resource, channel, density and time tests passed\n";
}
