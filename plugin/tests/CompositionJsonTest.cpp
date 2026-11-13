#include "../Source/engine/CompositionJson.h"
#include "../Source/engine/Generation.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
using namespace composer;
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
std::string fixture(){return R"({"schemaVersion":2,"ppq":480,"engineVersion":4294967295,"seed":4294967295,"id":"fixture","style":"trap","source":"native","legacyJson":"{\"value\":1}","timingNotice":"exact","endTick":6240,"tempos":[{"tick":0,"microsPerQuarter":500000},{"tick":2880,"microsPerQuarter":400000}],"meters":[{"tick":0,"numerator":3,"denominator":4},{"tick":2880,"numerator":5,"denominator":8}],"keys":[{"tick":0,"root":0,"scale":"natural-minor"},{"tick":2880,"root":2,"scale":"dorian"}],"sections":[{"id":"18446744073709551615","name":"Verse","startTick":0,"endTick":6240,"role":"verse"}],"chords":[{"id":"9007199254740993","startTick":0,"durationTicks":1440,"root":0,"pitchClasses":[0,3,7],"voicing":[48,51,55],"inversion":0,"tension":75,"explanation":"borrowed"}],"tracks":[{"id":"1","name":"Bass \u0411\u0430\u0441 \ud83c\udfb5","role":"bass","channel":16,"lowPitch":24,"highPitch":90,"program":33,"bank":{"msb":2,"lsb":7},"ownedPitches":"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000","notes":[{"id":"18446744073709551614","pitch":60,"velocity":99,"startTick":240,"durationTicks":720}],"controls":[{"id":"9007199254740995","tick":120,"controller":64,"value":127}]}]})";}
std::string replace(std::string s,std::string_view from,std::string_view to){auto p=s.find(from);check(p!=std::string::npos);s.replace(p,from.size(),to);return s;}
void rejects(std::string_view json){auto r=decodeCompositionJson(json);check(!r.composition&&!r.error.empty());}
int main(){
    auto read=decodeCompositionJson(fixture());if(!read.composition)std::cerr<<read.error<<'\n';check(read.composition);const auto& c=*read.composition;
    check(c.schemaVersion==2&&c.ppq==480&&c.engineVersion==UINT32_MAX&&c.seed==UINT32_MAX&&c.id=="fixture"&&c.style=="trap"&&c.source=="native");
    check(c.legacyJson=="{\"value\":1}"&&c.timingNotice=="exact"&&c.endTick.value==6240);
    check(c.tempos.size()==2&&c.tempos[1].tick.value==2880&&c.tempos[1].microsPerQuarter==400000);
    check(c.meters.size()==2&&c.meters[1].numerator==5&&c.meters[1].denominator==8&&c.meters[1].tick.value==2880);
    check(c.keys.size()==2&&c.keys[0].scale==Scale::naturalMinor&&c.keys[1].root==2&&c.keys[1].scale==Scale::dorian);
    check(c.sections.size()==1&&c.sections[0].id==UINT64_MAX&&c.sections[0].name=="Verse"&&c.sections[0].end.value==6240&&c.sections[0].role==SectionRole::verse);
    check(c.chords.size()==1&&c.chords[0].id==9007199254740993ULL&&c.chords[0].duration.value==1440&&c.chords[0].root==0&&c.chords[0].inversion==0&&c.chords[0].tension==75&&c.chords[0].explanation=="borrowed");
    check((c.chords[0].pitchClasses==std::vector<int>{0,3,7}));check((c.chords[0].voicing==std::vector<int>{48,51,55}));
    const auto& t=c.tracks[0];check(t.id==1&&t.name=="Bass \xd0\x91\xd0\xb0\xd1\x81 \xf0\x9f\x8e\xb5"&&t.role==Role::bass&&t.channel==16&&t.lowPitch==24&&t.highPitch==90);
    check(t.program==33&&t.bank&&t.bank->msb==2&&t.bank->lsb==7&&t.ownedPitches.none());
    check(t.notes.size()==1&&t.notes[0].id==UINT64_MAX-1&&t.notes[0].pitch==60&&t.notes[0].velocity==99&&t.notes[0].start.value==240&&t.notes[0].duration.value==720);
    check(t.controls.size()==1&&t.controls[0].id==9007199254740995ULL&&t.controls[0].tick.value==120&&t.controls[0].controller==64&&t.controls[0].value==127);
    auto saved=encodeCompositionJson(c);check(saved&&!saved.json.empty());auto again=decodeCompositionJson(saved.json);check(again.composition);check(encodeCompositionJson(*again.composition).json==saved.json);
    check(saved.json.find("\"18446744073709551615\"")!=std::string::npos);
    rejects(replace(fixture(),"\"schemaVersion\":2","\"schemaVersion\":1"));
    rejects(replace(fixture(),"\"ppq\":480,",""));
    rejects(replace(fixture(),"\"ppq\":480","\"ppq\":480,\"ppq\":480"));
    rejects(replace(fixture(),"\"ppq\":480","\"ppq\":480,\"\\u0070pq\":480"));
    rejects(replace(fixture(),"\"ppq\":480","\"ppq\":480,\"future\":{}"));
    for(auto bad:{"480.0","4.8e2","+480","0480","true","\"480\"","null","9223372036854775808","NaN"})rejects(replace(fixture(),"\"ppq\":480",std::string("\"ppq\":")+bad));
    for(auto bad:{"\"0\"","\"01\"","\"-1\"","\"18446744073709551616\"","18446744073709551615","\"1e3\"","\"\"","null"})rejects(replace(fixture(),"\"18446744073709551615\"",bad));
    for(auto bad:{"\\u0000","\\ud800","\\udc00","\\ud800x","\\ud800\\u0061","\\x20"})rejects(replace(fixture(),"exact",bad));
    for(auto bad:{std::string("\xc0\x80",2),std::string("\xed\xa0\x80",3),std::string("\xf4\x90\x80\x80",4),std::string("\xff",1),std::string("\0",1),std::string("\n",1)})rejects(replace(fixture(),"exact",bad));
    rejects(fixture()+"{}");rejects(replace(fixture(),"\"schemaVersion\":2,","\"schemaVersion\":2,,"));rejects(replace(fixture(),"\"ppq\":480","\"ppq\":[[[[[[[[[[480]]]]]]]]]]"));
    rejects(fixture().substr(0,fixture().size()-1)+",}");rejects(std::string(32*1024*1024+1,' '));
    auto reordered=replace(fixture(),"\"schemaVersion\":2,\"ppq\":480","\"ppq\":480,\"schemaVersion\":2");check(decodeCompositionJson(reordered).composition);
    auto badModel=c;badModel.tracks[0].notes[0].velocity=128;auto failed=encodeCompositionJson(badModel);check(!failed&&failed.json.empty()&&!failed.error.empty());
    badModel=c;badModel.source="\xff";check(!encodeCompositionJson(badModel));
    auto optional=c;optional.tracks[0].program.reset();optional.tracks[0].bank.reset();optional.tracks[0].ownedPitches.set(127);optional.tracks[0].ownedPitches.set(0);
    optional.timingNotice="quote\" slash\\ tab\t newline\n \x01";saved=encodeCompositionJson(optional);check(saved);again=decodeCompositionJson(saved.json);check(again.composition&&!again.composition->tracks[0].program&&!again.composition->tracks[0].bank&&again.composition->tracks[0].ownedPitches.count()==2&&again.composition->timingNotice==optional.timingNotice);
    for(int mode=0;mode<11;++mode){Config cfg;cfg.scale=Scale(mode);cfg.drums=true;auto g=generateLoop(cfg);check(g.composition);saved=encodeCompositionJson(*g.composition);check(saved);again=decodeCompositionJson(saved.json);check(again.composition&&encodeCompositionJson(*again.composition).json==saved.json);}
    for(int role=0;role<9;++role){optional=c;optional.tracks[0].role=Role(role);optional.tracks[0].channel=(role==3||role==4)?10:1;saved=encodeCompositionJson(optional);check(saved);again=decodeCompositionJson(saved.json);check(again.composition&&again.composition->tracks[0].role==Role(role));}
    for(int role=0;role<8;++role){optional=c;optional.sections[0].role=SectionRole(role);saved=encodeCompositionJson(optional);check(saved);again=decodeCompositionJson(saved.json);check(again.composition&&again.composition->sections[0].role==SectionRole(role));}
    auto dense=c;dense.sections.clear();dense.chords.clear();dense.tracks[0].notes.clear();dense.tracks[0].controls.clear();
    for(size_t i=0;i<131072;++i)dense.tracks[0].notes.push_back({i+10,60,90,{0},{480}});
    for(size_t i=0;i<65536;++i)dense.tracks[0].controls.push_back({i+200000,{int64_t(i%6241)},1,64});
    saved=encodeCompositionJson(dense);check(saved);again=decodeCompositionJson(saved.json);check(again.composition&&again.composition->tracks[0].notes.size()==131072&&again.composition->tracks[0].controls.size()==65536);
    auto tooMany=replace(saved.json,"\"notes\":[","\"notes\":[{\"id\":\"999999\",\"pitch\":60,\"velocity\":90,\"startTick\":0,\"durationTicks\":480},");rejects(tooMany);
    {
        auto split=dense;Track second;second.id=500000;second.name="Second";second.channel=1;
        auto& first=split.tracks[0];second.notes.assign(first.notes.begin()+65536,first.notes.end());first.notes.resize(65536);
        second.controls.assign(first.controls.begin()+32768,first.controls.end());first.controls.resize(32768);split.tracks.push_back(std::move(second));
        auto splitJson=encodeCompositionJson(split);check(splitJson);auto splitRead=decodeCompositionJson(splitJson.json);check(splitRead.composition&&splitRead.composition->tracks.size()==2);
        check(splitRead.composition->tracks[1].notes.size()==65536&&splitRead.composition->tracks[1].controls.size()==32768);
        rejects(replace(splitJson.json,"\"notes\":[","\"notes\":[{\"id\":\"999999\",\"pitch\":60,\"velocity\":90,\"startTick\":0,\"durationTicks\":480},"));
        rejects(replace(splitJson.json,"\"controls\":[","\"controls\":[{\"id\":\"999999\",\"tick\":0,\"controller\":1,\"value\":64},"));
    }
    dense.legacyJson.assign(4*1024*1024,'\x01');check(validate(dense));failed=encodeCompositionJson(dense);check(!failed&&failed.json.empty()&&!failed.error.empty());dense.legacyJson.clear();
    optional=c;optional.legacyJson.assign(4*1024*1024,'\x01');saved=encodeCompositionJson(optional);check(saved);again=decodeCompositionJson(saved.json);check(again.composition&&again.composition->legacyJson==optional.legacyJson);
    optional.legacyJson.push_back('a');check(!encodeCompositionJson(optional));
    optional=c;optional.tempos.clear();optional.keys.clear();optional.meters.clear();optional.endTick={491520};
    for(int i=0;i<4096;++i){optional.tempos.push_back({{i},400000});optional.keys.push_back({{i},i%12,Scale(i%11)});optional.meters.push_back({{i*120},1,16});}
    saved=encodeCompositionJson(optional);check(saved);again=decodeCompositionJson(saved.json);check(again.composition&&again.composition->tempos.size()==4096&&again.composition->meters.size()==4096&&again.composition->keys.size()==4096);
    rejects(replace(saved.json,"\"tempos\":[","\"tempos\":[{\"tick\":0,\"microsPerQuarter\":500000},"));
    optional=c;optional.id.assign(128,'x');optional.source.assign(1024,'x');optional.tracks[0].name.assign(128,'x');saved=encodeCompositionJson(optional);check(saved&&decodeCompositionJson(saved.json).composition);
    optional.source.push_back('x');check(!encodeCompositionJson(optional));
    optional=c;optional.tracks.clear();
    for(int ch=1;ch<=16;++ch){Track track;track.id=Id(100+ch);track.name="Track";track.channel=ch;track.role=ch==10?Role::drums:Role::bass;track.notes={{Id(200+ch),60,90,{0},{480}}};optional.tracks.push_back(track);}
    saved=encodeCompositionJson(optional);check(saved);again=decodeCompositionJson(saved.json);check(again.composition&&again.composition->tracks.size()==16);
    optional=c;optional.sections.clear();optional.chords.clear();optional.endTick={20000};
    for(size_t i=0;i<1024;++i)optional.sections.push_back({200000+i,"Section",{int64_t(i)},{int64_t(i+1)},SectionRole::verse});
    for(size_t i=0;i<16384;++i)optional.chords.push_back({100000+i,{int64_t(i)},{1},0,{0,3,7},{48,51,55},0,50,""});
    saved=encodeCompositionJson(optional);check(saved);again=decodeCompositionJson(saved.json);check(again.composition&&again.composition->sections.size()==1024&&again.composition->chords.size()==16384);
    rejects(replace(saved.json,"\"sections\":[","\"sections\":[{\"id\":\"999999\",\"name\":\"extra\",\"startTick\":0,\"endTick\":1,\"role\":\"intro\"},"));
    rejects(replace(saved.json,"\"chords\":[","\"chords\":[{\"id\":\"999999\",\"startTick\":0,\"durationTicks\":1,\"root\":0,\"pitchClasses\":[0],\"voicing\":[],\"inversion\":0,\"tension\":50,\"explanation\":\"\"},"));
    std::mt19937 rng(123);for(int i=0;i<2048;++i){std::string bytes(size_t(rng()%128),' ');for(auto& b:bytes)b=char(rng()%256);rejects(bytes);}
    auto original=fixture();for(size_t i=0;i<original.size();i+=7)rejects(std::string_view(original).substr(0,i));
    std::cout<<"composition JSON exact fields/uint64/Unicode, strict schema,131072 notes/65536 CC, provenance and malformed inputs passed\n";
}
