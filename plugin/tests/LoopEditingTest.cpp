#include "../Source/engine/LoopEditing.h"
#include "../Source/engine/CompositionJson.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>

namespace faults {inline int64_t remaining=-1,count=0;}
void* operator new(size_t n) {
    ++faults::count;
    if(faults::remaining==0){faults::remaining=-1;throw std::bad_alloc();}
    if(faults::remaining>0)--faults::remaining;
    if(auto* p=std::malloc(n?n:1))return p;
    throw std::bad_alloc();
}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,size_t)noexcept{std::free(p);}
void operator delete[](void* p,size_t)noexcept{std::free(p);}

using namespace composer;
#define check(x) do{if(!(x))throw std::runtime_error("line "+std::to_string(__LINE__)+": " #x);}while(false)

Composition fixture() {
    Composition c;c.id="loop-edit-fixture";c.engineVersion=5;c.seed=421;
    c.style="ambient";c.source="offline seed 421";c.legacyJson="{\"retained\":true}";c.timingNotice="Original timing";
    c.endTick={3840};c.tempos={{{0},500000},{{1920},600000}};
    c.meters={{{0},4,4},{{1920},3,4}};c.keys={{{0},0,Scale::naturalMinor},{{1920},7,Scale::dorian}};
    c.sections={{50,"A",{0},{1920},SectionRole::verse},{51,"B",{1920},{3840},SectionRole::chorus}};
    c.chords={{60,{0},{1920},0,{0,3,7},{48,51,55},0,30,"First harmony"},
              {61,{1920},{1920},7,{2,7,10},{50,55,58},1,60,"Second harmony"}};
    Track bass;bass.id=10;bass.name="Bass";bass.channel=1;bass.lowPitch=36;bass.highPitch=84;
    bass.program=40;bass.bank=Bank{2,3};bass.ownedPitches.set(60);
    bass.notes={{23,67,93,{1440},{240}},{20,60,90,{0},{720}},{22,64,92,{960},{240}},
                {21,62,91,{480},{240}},{24,66,94,{1440},{120}}};
    bass.controls={{30,{0},7,100},{31,{1920},64,0}};
    Track lead;lead.id=40;lead.name="Lead";lead.role=Role::lead;lead.channel=2;lead.program=80;
    lead.notes={{41,72,100,{480},{480}}};lead.controls={{42,{120},1,33}};
    c.tracks={bass,lead};return c;
}

std::string snapshot(const Composition& c) {
    jsonDetail::SchemaWriter writer;writer.value(c);return std::move(writer.output);
}
void expectSuccess(const Composition& original,const LoopEditResult& result,const Composition& expected) {
    check(result.composition);check(result.error.empty());check(validate(*result.composition));
    check(snapshot(*result.composition)==snapshot(expected));
    check(snapshot(original)==snapshot(fixture()));
}

void editsPreserveIdentityAndMetadata() {
    const auto c=fixture();check(validate(c));const auto original=snapshot(c);
    auto expected=c;expected.tracks[0].notes={{20,60,90,{0},{720}},{22,64,92,{960},{240}},
        {21,65,101,{1200},{300}},{24,66,94,{1440},{120}},{23,67,93,{1440},{240}}};
    expectSuccess(c,editLoopNote(c,10,21,Note{40,65,101,{1200},{300}}),expected);
    expected=c;expected.tracks[0].notes={{20,60,90,{0},{720}},{21,62,91,{480},{240}},
        {62,62,77,{480},{360}},{22,64,92,{960},{240}},{24,66,94,{1440},{120}},{23,67,93,{1440},{240}}};
    expectSuccess(c,editLoopNote(c,10,0,Note{61,62,77,{480},{360}}),expected);
    expected=c;expected.tracks[0].notes={{20,60,90,{0},{720}},{21,62,91,{480},{240}},
        {24,66,94,{1440},{120}},{23,67,93,{1440},{240}}};
    expectSuccess(c,editLoopNote(c,10,22,std::nullopt),expected);
    check(snapshot(c)==original);
    auto one=c;one.tracks[0].notes={{20,60,90,{0},{720}}};
    auto empty=editLoopNote(one,10,20,std::nullopt);check(empty.composition&&empty.composition->tracks[0].notes.empty());
}

void rejectedEditsPreserveInput() {
    const auto c=fixture();const auto original=snapshot(c);
    auto rejected=[&](Id track,Id note,const std::optional<Note>& replacement) {
        auto result=editLoopNote(c,track,note,replacement);check(!result.composition&&!result.error.empty());check(snapshot(c)==original);
    };
    const Note note{0,60,90,{0},{480}};
    rejected(0,0,note);rejected(9999,0,note);rejected(10,41,note);rejected(10,9999,note);
    rejected(10,0,std::nullopt);rejected(10,9999,std::nullopt);
    for(const auto invalid:{Note{0,-1,90,{0},{480}},Note{0,128,90,{0},{480}},Note{0,35,90,{0},{480}},
        Note{0,85,90,{0},{480}},Note{0,60,0,{0},{480}},Note{0,60,128,{0},{480}},
        Note{0,60,90,{-1},{480}},Note{0,60,90,{3840},{1}},Note{0,60,90,{0},{0}},
        Note{0,60,90,{0},{-1}},Note{0,60,90,{3839},{2}},
        Note{0,60,90,{1},{std::numeric_limits<int64_t>::max()}}}) {
        rejected(10,0,invalid);rejected(10,21,invalid);
    }
    auto duplicate=c;duplicate.tracks[0].notes[0].id=40;const auto duplicateBefore=snapshot(duplicate);
    auto bad=editLoopNote(duplicate,10,20,note);check(!bad.composition&&!bad.error.empty());check(snapshot(duplicate)==duplicateBefore);
    auto invalid=c;invalid.tracks[1].notes[0].velocity=0;const auto invalidBefore=snapshot(invalid);
    bad=editLoopNote(invalid,10,20,note);check(!bad.composition&&!bad.error.empty());check(snapshot(invalid)==invalidBefore);
    check(editLoopNote(c,10,21,Note{0,63,90,{480},{240}}).composition);
}

void rangesUseHalfOpenStartsAndFreshIds() {
    const auto c=fixture();auto expected=c;
    expected.tracks[0].notes={{20,60,90,{0},{720}},{63,68,101,{480},{240}},
        {62,70,102,{1200},{240}},{24,66,94,{1440},{120}},{23,67,93,{1440},{240}}};
    expectSuccess(c,replaceLoopRange(c,10,{480},{1440},{{20,70,102,{1200},{240}},
        {std::numeric_limits<Id>::max(),68,101,{480},{240}}}),expected);
    expected=c;expected.tracks[0].notes={{20,60,90,{0},{720}},{24,66,94,{1440},{120}},{23,67,93,{1440},{240}}};
    expectSuccess(c,replaceLoopRange(c,10,{480},{1440},{}),expected);
    expected=c;expected.tracks[0].notes.clear();expectSuccess(c,replaceLoopRange(c,10,{0},c.endTick,{}),expected);
    auto endpoint=replaceLoopRange(c,10,{3360},{3840},{{0,60,90,{3360},{480}}});
    check(endpoint.composition&&endpoint.composition->tracks[0].notes.back().start.value==3360);
}

void rejectedRangesPreserveInput() {
    const auto c=fixture();const auto original=snapshot(c);
    auto rejected=[&](Id track,Tick start,Tick end,const std::vector<Note>& notes) {
        auto result=replaceLoopRange(c,track,start,end,notes);check(!result.composition&&!result.error.empty());check(snapshot(c)==original);
    };
    rejected(9999,{480},{1440},{});rejected(0,{480},{1440},{});
    rejected(10,{-1},{1440},{});rejected(10,{480},{480},{});rejected(10,{1440},{480},{});
    rejected(10,{480},{3841},{});rejected(10,{3840},{3841},{});
    for(const auto note:{Note{0,60,90,{479},{1}},Note{0,60,90,{1440},{1}},
        Note{0,60,90,{1200},{241}},Note{0,60,90,{480},{0}},Note{0,60,90,{480},{-1}},
        Note{0,35,90,{480},{120}},Note{0,85,90,{480},{120}},Note{0,60,0,{480},{120}},
        Note{0,60,128,{480},{120}},Note{0,60,90,{480},{std::numeric_limits<int64_t>::max()}}})
        rejected(10,{480},{1440},{note});
    auto duplicate=c;duplicate.sections[0].id=20;const auto before=snapshot(duplicate);
    auto bad=replaceLoopRange(duplicate,10,{480},{1440},{});check(!bad.composition&&!bad.error.empty());check(snapshot(duplicate)==before);
    check(replaceLoopRange(c,10,{480},{1440},{{0,60,90,{480},{960}}}).composition);
}

void freshIdsHandleAllKindsAndUint64Boundary() {
    auto c=fixture();c.tracks[0].id=1;
    c.tracks[0].notes={{6,67,93,{1440},{240}},{2,60,90,{0},{720}},{4,64,92,{960},{240}},
        {3,62,91,{480},{240}},{7,66,94,{1440},{120}}};
    c.tracks[0].controls={{8,{0},7,100}};c.tracks[1].id=9;c.tracks[1].notes[0].id=10;c.tracks[1].controls[0].id=11;
    c.sections[0].id=12;c.sections[1].id=13;c.chords[0].id=14;c.chords[1].id=std::numeric_limits<Id>::max();
    check(validate(c));const auto before=snapshot(c);
    auto added=editLoopNote(c,1,0,Note{0,65,100,{1800},{120}});
    check(added.composition&&added.composition->tracks[0].notes.back().id==5);
    auto range=replaceLoopRange(c,1,{480},{1440},{{0,60,90,{480},{120}},{0,61,90,{600},{120}}});
    check(range.composition);check(range.composition->tracks[0].notes[1].id==5);check(range.composition->tracks[0].notes[2].id==15);
    check(snapshot(c)==before);
    c.chords[1].id=std::numeric_limits<Id>::max()-1;
    range=replaceLoopRange(c,1,{480},{1440},{{0,60,90,{480},{120}},{0,61,90,{600},{120}}});
    check(range.composition);check(range.composition->tracks[0].notes[1].id==std::numeric_limits<Id>::max());
    check(range.composition->tracks[0].notes[2].id==5);
}

void countLimitsAndLongImportedModels() {
    auto c=fixture();c.tracks[0].notes.clear();c.tracks[0].notes.reserve(limits::notes-1);
    for(size_t i=0;i<limits::notes-1;++i)c.tracks[0].notes.push_back({Id(1000+i),60,90,{0},{120}});
    check(validate(c));const auto before=snapshot(c);
    auto rejected=editLoopNote(c,10,0,Note{0,60,90,{480},{120}});check(!rejected.composition&&!rejected.error.empty());
    rejected=replaceLoopRange(c,10,{480},{1440},{{0,60,90,{480},{120}}});check(!rejected.composition&&!rejected.error.empty());
    check(snapshot(c)==before);
    auto replaced=replaceLoopRange(c,10,{0},{120},{{0,60,90,{0},{120}}});
    check(replaced.composition&&replaced.composition->tracks[0].notes.size()==1&&validate(*replaced.composition));
    auto tooMany=std::vector<Note>(limits::notes+1,Note{0,60,90,{0},{120}});
    rejected=replaceLoopRange(c,10,{0},{120},tooMany);check(!rejected.composition&&!rejected.error.empty());
    c=fixture();c.endTick={61440};const auto old=snapshot(c);
    auto edited=editLoopNote(c,10,0,Note{0,65,90,{60000},{1440}});
    check(edited.composition&&edited.composition->endTick.value==61440&&edited.composition->seed==421);
    auto range=replaceLoopRange(c,10,{60000},{61440},{{0,65,90,{60000},{1440}}});
    check(range.composition&&range.composition->endTick.value==61440&&snapshot(c)==old);
}

void sharedDrumOwnershipAndDensityAreValidated() {
    Composition c=fixture();c.tracks.clear();
    Track drums;drums.id=1;drums.name="Drums";drums.role=Role::drums;drums.channel=10;drums.ownedPitches.set(36);
    drums.notes={{2,36,100,{0},{120}}};Track percussion=drums;percussion.id=3;percussion.name="Percussion";percussion.role=Role::percussion;
    percussion.ownedPitches.reset();percussion.ownedPitches.set(60);percussion.notes={{4,60,80,{0},{120}}};c.tracks={drums,percussion};
    check(validate(c));const auto before=snapshot(c);
    auto bad=editLoopNote(c,1,0,Note{0,60,90,{480},{120}});check(!bad.composition&&!bad.error.empty());
    bad=replaceLoopRange(c,1,{480},{960},{{0,60,90,{480},{120}}});check(!bad.composition&&!bad.error.empty());
    check(snapshot(c)==before);check(editLoopNote(c,1,0,Note{0,36,90,{480},{120}}).composition);
    auto sole=c;sole.tracks.resize(1);sole.tracks[0].ownedPitches.reset();check(validate(sole));
    check(editLoopNote(sole,1,2,Note{0,38,90,{0},{120}}).composition);
    check(editLoopNote(sole,1,0,Note{0,42,90,{480},{120}}).composition);
    check(replaceLoopRange(sole,1,{0},{480},{{0,46,90,{0},{120}}}).composition);
    c=fixture();c.tracks.clear();Id id=100;
    for(int channel=1;channel<=3;++channel) {
        Track t;t.id=id++;t.name="Dense";t.channel=channel;
        for(int pitch=0;pitch<(channel==3?0:128);++pitch)t.notes.push_back({id++,pitch,90,{0},{480}});
        c.tracks.push_back(t);
    }
    check(validate(c));const auto dense=snapshot(c);
    bad=editLoopNote(c,c.tracks[2].id,0,Note{0,60,90,{0},{480}});check(!bad.composition&&!bad.error.empty());
    bad=replaceLoopRange(c,c.tracks[2].id,{0},{480},{{0,60,90,{0},{480}}});check(!bad.composition&&!bad.error.empty());
    check(snapshot(c)==dense);check(editLoopNote(c,c.tracks[2].id,0,Note{0,60,90,{480},{480}}).composition);
}

void allocationFailuresPreserveInputAndRecover() {
    const auto c=fixture();const auto before=snapshot(c);const std::vector<Note> replacements={{0,65,100,{480},{240}}};
    auto sweep=[&](auto action) {
        faults::count=0;auto good=action();const auto allocations=faults::count;check(good.composition);const auto expected=snapshot(*good.composition);
        for(int64_t position=0;position<allocations;++position) {
            faults::remaining=position;LoopEditResult result;bool escaped=false;
            try{result=action();}catch(const std::bad_alloc&){escaped=true;}
            faults::remaining=-1;check(!escaped);check(!result.composition&&!result.error.empty());check(snapshot(c)==before);
        }
        auto retry=action();check(retry.composition&&snapshot(*retry.composition)==expected);
    };
    sweep([&]{return editLoopNote(c,10,0,Note{0,65,100,{480},{240}});});
    sweep([&]{return editLoopNote(c,10,21,Note{0,65,100,{480},{240}});});
    sweep([&]{return editLoopNote(c,10,21,std::nullopt);});
    sweep([&]{return replaceLoopRange(c,10,{480},{1440},replacements);});
}

int main() {
    int failures=0;
    auto run=[&](const char* name,auto test) {try{test();std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& error){++failures;std::cerr<<"FAIL "<<name<<": "<<error.what()<<'\n';}};
    run("add/update/delete preserve IDs and metadata",editsPreserveIdentityAndMetadata);
    run("invalid edits are atomic and recover",rejectedEditsPreserveInput);
    run("half-open replacement preserves crossing/outside notes",rangesUseHalfOpenStartsAndFreshIds);
    run("invalid selections and replacements are atomic",rejectedRangesPreserveInput);
    run("global IDs use free holes at uint64 boundary",freshIdsHandleAllKindsAndUint64Boundary);
    run("global count bounds and longer imported documents",countLimitsAndLongImportedModels);
    run("shared drums and result density remain valid",sharedDrumOwnershipAndDensityAreValidated);
    run("allocation failure sweep and recovery",allocationFailuresPreserveInputAndRecover);
    if(failures)return 1;
    std::cout<<"Pure loop note/range editing tests passed\n";
}
