#include "../Source/engine/ProjectJson.h"
#include "../Source/engine/Generation.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace composer;
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
std::string replace(std::string s,std::string_view from,std::string_view to){auto p=s.find(from);check(p!=std::string::npos);s.replace(p,from.size(),to);return s;}
void rejects(std::string_view json){auto r=decodeProjectJson(json);check(!r.project&&!r.error.empty()&&r.error.size()<512);}
void rejectsWrite(const ProjectState& p){auto r=encodeProjectJson(p);check(!r&&r.json.empty()&&!r.error.empty());}
int main(int argc,char** argv){
    check(argc==2);
    auto idea=juce::File(argv[1]).loadFileAsString().toStdString();
    // Dropping extensions, normalizing numeric tokens or quantizing beats must fail these assertions.
    idea=replace(idea,"\"start\":0","\"start\":0.000000000000000123");
    const std::string legacy="{ \"version\": 1, \"selection\":7, \"extension\":{\"precise\":1.234567890123456789e-2}, \"idea\":"+idea+" }\n";
    check(wire::document(legacy).isObject());
    auto generated=generateLoop(Config{});check(generated.composition);
    ProjectState p;p.activeMode=ProjectMode::composer;p.composition=std::move(generated.composition);p.hostStartQuarterBeat=-123.45678901234567;
    p.composition->tracks[0].notes[0].id=UINT64_MAX;
    auto saved=encodeProjectJson(p);check(saved);
    check(saved.json.find("\"version\":2")!=std::string::npos&&saved.json.find("\"activeMode\":\"composer\"")!=std::string::npos);
    auto read=decodeProjectJson(saved.json);check(read.project&&read.project->activeMode==ProjectMode::composer&&!read.project->legacyStateJson);
    check(read.project->hostStartQuarterBeat==p.hostStartQuarterBeat&&read.project->composition->tracks[0].notes[0].id==UINT64_MAX);
    check(encodeProjectJson(*read.project).json==saved.json);
    p.legacyStateJson=legacy;
    for(auto mode:{ProjectMode::legacy,ProjectMode::composer}){
        p.activeMode=mode;saved=encodeProjectJson(p);check(saved);read=decodeProjectJson(saved.json);
        check(read.project&&read.project->activeMode==mode&&read.project->legacyStateJson==legacy&&read.project->composition);
        check(encodeProjectJson(*read.project).json==saved.json);
    }
    p.activeMode=ProjectMode::legacy;p.composition.reset();saved=encodeProjectJson(p);check(saved);read=decodeProjectJson(saved.json);check(read.project&&!read.project->composition&&read.project->legacyStateJson==legacy);
    const auto good=saved.json;
    check(decodeProjectJson(replace(good,"\"version\":2,\"activeMode\":\"legacy\"","\"activeMode\":\"legacy\",\"version\":2")).project);
    for(auto bad:{"2.0","2e0","true","\"2\"","null"})rejects(replace(good,"\"version\":2",std::string("\"version\":")+bad));
    p.activeMode=ProjectMode::composer;p.composition=generateLoop(Config{}).composition;
    auto both=encodeProjectJson(p);check(both);
    rejects(replace(both.json,"\\\"version\\\": 1","\\\"version\\\": 9")); // inactive legacy must validate
    rejects(replace(both.json,"\"schemaVersion\":2","\"schemaVersion\":9"));
    p.activeMode=ProjectMode::legacy;p.composition.reset();

    rejects(replace(good,"\"version\":2","\"version\":1"));
    rejects(replace(good,"\"version\":2","\"version\":3"));
    rejects(replace(good,"\"version\":2,",""));
    rejects(replace(good,"\"version\":2","\"version\":2,\"version\":2"));
    rejects(replace(good,"\"version\":2","\"version\":2,\"\\u0076ersion\":2"));
    rejects(replace(good,"\"version\":2","\"version\":2,\"future\":{}"));
    rejects(replace(good,"\"activeMode\":\"legacy\"","\"activeMode\":\"composer\""));
    rejects(replace(good,"\"activeMode\":\"legacy\"","\"activeMode\":\"future\""));
    rejects(replace(good,"\"composition\":null","\"composition\":{}")); // inactive still validates
    rejects(good+"{}");rejects(good.substr(0,good.size()-1));
    rejects(replace(good,"\"composition\":null","\"composition\":"+std::string(10000,'[')+"null"+std::string(10000,']')));
    const auto origin=std::string("\"hostStartQuarterBeat\":")+"-123.45678901234567";
    for(auto bad:{"NaN","Infinity","1e999","1e-999","+1","01","1.",".1","--1","true","null","\"1\"","[1]","1e","1e+","0x1"})rejects(replace(good,origin,std::string("\"hostStartQuarterBeat\":")+bad));
    for(double value:{0.0,-0.0,0.125,-0.125,1e20,std::numeric_limits<double>::min(),std::numeric_limits<double>::denorm_min(),std::numeric_limits<double>::max()}){
        p.hostStartQuarterBeat=value;auto s=encodeProjectJson(p);check(s);auto r=decodeProjectJson(s.json);check(r.project&&r.project->hostStartQuarterBeat==value);
    }
    p.hostStartQuarterBeat=0;
    for(auto raw:{"{}","{\"version\":2,\"idea\":{}}","{\"version\":1,\"idea\":{}}","{\"version\":1,\"idea\":null}"}){p.legacyStateJson=raw;rejectsWrite(p);}
    p.legacyStateJson=legacy;p.activeMode=ProjectMode::composer;rejectsWrite(p);p.activeMode=ProjectMode::legacy;p.legacyStateJson.reset();rejectsWrite(p);
    p.legacyStateJson=legacy;p.activeMode=ProjectMode(99);rejectsWrite(p);p.activeMode=ProjectMode::legacy;
    for(auto n:{std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){p.hostStartQuarterBeat=n;rejectsWrite(p);}p.hostStartQuarterBeat=0;
    p.composition=generateLoop(Config{}).composition;p.composition->tracks[0].notes[0].velocity=128;rejectsWrite(p);p.composition.reset();
    // Removing the outer limit or checking only decoded components would accept this oversized envelope.
    rejects(std::string(32*1024*1024+1,' '));
    p.legacyStateJson=legacy+std::string(4*1024*1024-legacy.size(),' ');
    auto legacyLimit=encodeProjectJson(p);check(legacyLimit&&decodeProjectJson(legacyLimit.json).project);
    rejects(replace(legacyLimit.json,"\",\"composition\":"," \",\"composition\":"));
    p.legacyStateJson->push_back(' ');rejectsWrite(p);
    p.legacyStateJson=legacy;p.legacyStateJson->reserve(64*1024*1024);rejectsWrite(p);p.legacyStateJson.reset();
    // Escaped legacy bytes are preserved, not reserialized by JUCE.
    p.legacyStateJson=legacy;auto escaped=encodeProjectJson(p);check(escaped);rejects(replace(escaped.json,"selection","\\u0000"));
    // Literal composition and legacy components are individually bounded, but their combined encoding can exceed32MiB.
    p.composition=generateLoop(Config{}).composition;p.composition->legacyJson.assign(4*1024*1024,'\x01');
    check(encodeCompositionJson(*p.composition));
    auto extension=std::string(4*1024*1024-legacy.size()-30,'\x01');
    // Use backslashes: valid UTF8/JSON, large encoded expansion in the retained legacy envelope.
    extension.assign(extension.size(),'\\');
    juce::DynamicObject::Ptr object=new juce::DynamicObject;object->setProperty("version",1);object->setProperty("idea",wire::document(idea));
    object->setProperty("extension",juce::String::fromUTF8(extension.data(),int(extension.size()/2)));
    p.legacyStateJson=juce::JSON::toString(juce::var(object.get()),true).toStdString();
    check(p.legacyStateJson->size()<=4*1024*1024);rejectsWrite(p);
    // Total reader boundary: padding is legal JSON but still belongs to the envelope byte budget.
    auto exact=good+std::string(32*1024*1024-good.size(),' ');check(decodeProjectJson(exact).project);exact.push_back(' ');rejects(exact);
    check(!decodeProjectJson(legacy).project); // old restore must remain its own unchanged path
    std::cout<<"project envelope checks passed\n";
}
