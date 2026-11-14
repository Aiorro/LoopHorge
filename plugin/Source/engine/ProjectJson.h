#pragma once
#include "CompositionJson.h"
#include "../Wire.h"
#include <cmath>

namespace composer {
enum class ProjectMode { legacy,composer };
// Non-RT candidate only. The controller must validate/prepare before committing
// revision, cancellation, publication and UI state as one operation.
struct ProjectState {
    ProjectMode activeMode=ProjectMode::legacy;
    double hostStartQuarterBeat=0;
    std::optional<std::string> legacyStateJson;
    std::optional<Composition> composition;
};
struct ProjectReadResult {std::optional<ProjectState> project;std::string error;};
namespace projectDetail {
inline void validateProject(const ProjectState& p){
    using jsonDetail::Invalid;
    if(!std::isfinite(p.hostStartQuarterBeat))throw Invalid{};
    switch(p.activeMode){
        case ProjectMode::legacy:if(!p.legacyStateJson)throw Invalid{};break;
        case ProjectMode::composer:if(!p.composition)throw Invalid{};break;
        default:throw Invalid{};
    }
    size_t bytes=sizeof(ProjectState);
    if(p.legacyStateJson){
        const auto& raw=*p.legacyStateJson;
        if(raw.empty()||raw.size()>limits::legacyBytes||raw.capacity()>limits::documentBytes-bytes)throw Invalid{};
        bytes+=raw.capacity();
        auto legacy=wire::document(raw);Schedule schedule;
        if(!wire::integer(legacy["version"],1,1)||!wire::schedule(legacy["idea"],schedule))throw Invalid{};
    }
    if(p.composition){auto valid=validate(*p.composition);if(!valid||valid.accountedBytes>limits::documentBytes-bytes)throw Invalid{};}
}
// JSON number grammar is checked before from_chars (which also accepts some
// non-JSON spellings). Parent schema checks the following delimiter.
inline double readOrigin(jsonDetail::Reader& r){
    r.space();const size_t start=r.pos;
    auto digit=[&]{return r.pos<r.input.size()&&r.input[r.pos]>='0'&&r.input[r.pos]<='9';};
    if(r.pos<r.input.size()&&r.input[r.pos]=='-')++r.pos;
    if(!digit())throw jsonDetail::Invalid{};
    if(r.input[r.pos]=='0')++r.pos;else while(digit())++r.pos;
    if(r.pos<r.input.size()&&r.input[r.pos]=='.'){++r.pos;if(!digit())throw jsonDetail::Invalid{};while(digit())++r.pos;}
    if(r.pos<r.input.size()&&(r.input[r.pos]=='e'||r.input[r.pos]=='E')){
        ++r.pos;if(r.pos<r.input.size()&&(r.input[r.pos]=='+'||r.input[r.pos]=='-'))++r.pos;
        if(!digit())throw jsonDetail::Invalid{};
        while(digit())++r.pos;
    }
    double value=0;auto parsed=std::from_chars(r.input.data()+start,r.input.data()+r.pos,value,std::chars_format::general);
    if(parsed.ec!=std::errc{}||parsed.ptr!=r.input.data()+r.pos||!std::isfinite(value))throw jsonDetail::Invalid{};
    return value;
}
}
inline CompositionJsonResult encodeProjectJson(const ProjectState& p){
    try{
        projectDetail::validateProject(p);jsonDetail::SchemaWriter w;
        w.raw("{");w.key("version");w.integer(2);w.key("activeMode");w.string(p.activeMode==ProjectMode::legacy?"legacy":"composer");
        w.key("hostStartQuarterBeat");char number[64];auto end=std::to_chars(number,number+sizeof(number),p.hostStartQuarterBeat,std::chars_format::general,std::numeric_limits<double>::max_digits10);
        if(end.ec!=std::errc{})throw jsonDetail::Invalid{};
        w.raw({number,size_t(end.ptr-number)});
        w.key("legacyStateJson");if(p.legacyStateJson)w.string(*p.legacyStateJson);else w.raw("null");
        w.key("composition");if(p.composition)w.value(*p.composition);else w.raw("null");
        w.raw("}");return {std::move(w.output),{}};
    }catch(const jsonDetail::Invalid&){return {{},"Invalid project-v2 state, text or resource limit"};}
    catch(const std::bad_alloc&){return {{},"Insufficient memory to save project"};}
}
inline ProjectReadResult decodeProjectJson(std::string_view text){
    if(text.empty()||text.size()>jsonDetail::maximumBytes)return {{},"Project JSON exceeds byte limit or is empty"};
    try{
        jsonDetail::Reader r{text};ProjectState p;
        r.object({"version","activeMode","hostStartQuarterBeat","legacyStateJson","composition"},[&](size_t i){switch(i){
            case 0:if(r.integer()!=2)throw jsonDetail::Invalid{};break;
            case 1:{auto mode=r.string(8);if(mode=="legacy")p.activeMode=ProjectMode::legacy;else if(mode=="composer")p.activeMode=ProjectMode::composer;else throw jsonDetail::Invalid{};break;}
            case 2:p.hostStartQuarterBeat=projectDetail::readOrigin(r);break;
            case 3:if(!r.null())p.legacyStateJson=r.string(limits::legacyBytes);break;
            case 4:if(!r.null())p.composition=jsonDetail::readComposition(r);break;
        }});
        if(!r.done())throw jsonDetail::Invalid{};
        projectDetail::validateProject(p);return {std::move(p),{}};
    }catch(const jsonDetail::Invalid&){return {{},"Invalid project-v2 JSON, field type or resource limit"};}
    catch(const std::bad_alloc&){return {{},"Insufficient memory to restore project"};}
}
}
