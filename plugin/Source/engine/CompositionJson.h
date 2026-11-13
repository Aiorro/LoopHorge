#pragma once
#include "Validation.h"
#include "JsonCodec.h"
#include <new>
#include <utility>

namespace composer {
struct CompositionJsonResult {std::string json,error;explicit operator bool()const{return error.empty()&&!json.empty();}};
struct CompositionReadResult {std::optional<Composition> composition;std::string error;};
namespace jsonDetail {
inline constexpr std::array<std::string_view,11> scales={"major","natural-minor","harmonic-minor","melodic-minor","dorian","phrygian","lydian","mixolydian","major-pentatonic","minor-pentatonic","blues"};
inline constexpr std::array<std::string_view,9> roles={"bass","chords","lead","drums","percussion","counter-melody","arpeggio","pads","harmony"};
inline constexpr std::array<std::string_view,8> sectionRoles={"intro","verse","pre-chorus","chorus","bridge","breakdown","solo","outro"};
template<class Enum,size_t N>Enum enumeration(Reader& r,const std::array<std::string_view,N>& names){auto name=r.string(32);for(size_t i=0;i<N;++i)if(names[i]==name)return Enum(i);throw Invalid{};}
inline Id identity(Reader& r){auto s=r.string(20);if(s.empty()||s[0]<'1'||s[0]>'9')throw Invalid{};Id value=0;auto read=std::from_chars(s.data(),s.data()+s.size(),value);if(read.ec!=std::errc{}||read.ptr!=s.data()+s.size())throw Invalid{};return value;}
inline int number(Reader& r){return int(r.integer(INT32_MIN,INT32_MAX));}
inline Tick tick(Reader& r){return {r.integer()};}
inline void read(Reader& r,Tempo& t){r.object({"tick","microsPerQuarter"},[&](size_t i){if(i==0)t.tick=tick(r);else t.microsPerQuarter=number(r);});}
inline void read(Reader& r,Meter& m){r.object({"tick","numerator","denominator"},[&](size_t i){if(i==0)m.tick=tick(r);else if(i==1)m.numerator=number(r);else m.denominator=number(r);});}
inline void read(Reader& r,Key& k){r.object({"tick","root","scale"},[&](size_t i){if(i==0)k.tick=tick(r);else if(i==1)k.root=number(r);else k.scale=enumeration<Scale>(r,scales);});}
inline void read(Reader& r,Bank& b){r.object({"msb","lsb"},[&](size_t i){if(i==0)b.msb=number(r);else b.lsb=number(r);});}
inline void read(Reader& r,Note& n){
    if(r.notes++==limits::notes)throw Invalid{};
    r.object({"id","pitch","velocity","startTick","durationTicks"},[&](size_t i){switch(i){case 0:n.id=identity(r);break;case 1:n.pitch=number(r);break;case 2:n.velocity=number(r);break;case 3:n.start=tick(r);break;case 4:n.duration=tick(r);break;}});
}
inline void read(Reader& r,Control& c){
    if(r.controls++==limits::controls)throw Invalid{};
    r.object({"id","tick","controller","value"},[&](size_t i){switch(i){case 0:c.id=identity(r);break;case 1:c.tick=tick(r);break;case 2:c.controller=number(r);break;case 3:c.value=number(r);break;}});
}
template<class T>std::vector<T> collection(Reader& r,size_t maximum){return r.array<T>(maximum,[&]{T value;read(r,value);return value;});}
inline void read(Reader& r,Track& t){
    r.object({"id","name","role","channel","lowPitch","highPitch","program","bank","ownedPitches","notes","controls"},[&](size_t i){switch(i){
        case 0:t.id=identity(r);break;case 1:t.name=r.string(128);break;case 2:t.role=enumeration<Role>(r,roles);break;
        case 3:t.channel=number(r);break;case 4:t.lowPitch=number(r);break;case 5:t.highPitch=number(r);break;
        case 6:if(!r.null())t.program=number(r);break;
        case 7:if(!r.null()){Bank bank;read(r,bank);t.bank=bank;}break;
        case 8:{auto mask=r.string(128);if(mask.size()!=128||mask.find_first_not_of("01")!=std::string::npos)throw Invalid{};t.ownedPitches=std::bitset<128>(mask);break;}
        case 9:t.notes=collection<Note>(r,limits::notes);break;case 10:t.controls=collection<Control>(r,limits::controls);break;
    }});
}
inline void read(Reader& r,Section& s){r.object({"id","name","startTick","endTick","role"},[&](size_t i){switch(i){case 0:s.id=identity(r);break;case 1:s.name=r.string(128);break;case 2:s.start=tick(r);break;case 3:s.end=tick(r);break;case 4:s.role=enumeration<SectionRole>(r,sectionRoles);break;}});}
inline void read(Reader& r,Chord& c){
    r.object({"id","startTick","durationTicks","root","pitchClasses","voicing","inversion","tension","explanation"},[&](size_t i){switch(i){
        case 0:c.id=identity(r);break;case 1:c.start=tick(r);break;case 2:c.duration=tick(r);break;case 3:c.root=number(r);break;
        case 4:c.pitchClasses=r.array<int>(12,[&]{return number(r);});break;case 5:c.voicing=r.array<int>(16,[&]{return number(r);});break;
        case 6:c.inversion=number(r);break;case 7:c.tension=number(r);break;case 8:c.explanation=r.string(1024);break;
    }});
}
inline Composition readComposition(Reader& r){
    Composition c;r.object({"schemaVersion","ppq","engineVersion","seed","id","style","source","legacyJson","timingNotice","endTick","tempos","meters","keys","sections","chords","tracks"},[&](size_t i){switch(i){
        case 0:c.schemaVersion=number(r);if(c.schemaVersion!=2)throw Invalid{};break;case 1:c.ppq=number(r);if(c.ppq!=480)throw Invalid{};break;
        case 2:c.engineVersion=uint32_t(r.integer(1,UINT32_MAX));break;case 3:c.seed=uint32_t(r.integer(0,UINT32_MAX));break;
        case 4:c.id=r.string(128);break;case 5:c.style=r.string(1024);break;case 6:c.source=r.string(1024);break;case 7:c.legacyJson=r.string(limits::legacyBytes);break;case 8:c.timingNotice=r.string(1024);break;
        case 9:c.endTick=tick(r);break;case 10:c.tempos=collection<Tempo>(r,limits::mapPoints);break;case 11:c.meters=collection<Meter>(r,limits::mapPoints);break;case 12:c.keys=collection<Key>(r,limits::mapPoints);break;
        case 13:c.sections=collection<Section>(r,limits::sections);break;case 14:c.chords=collection<Chord>(r,limits::chords);break;case 15:c.tracks=collection<Track>(r,limits::tracks);break;
    }});return c;
}
// Schema-directed writing keeps numbers integral and IDs explicitly quoted.
struct SchemaWriter:Writer {
    template<std::integral T>void value(T n){integer(n);}
    void value(const std::string& s){string(s);}void value(std::string_view s){string(s);}
    void value(Tick t){integer(t.value);}
    template<class T>void value(const std::optional<T>& v){if(v)value(*v);else raw("null");}
    template<class T>void value(const std::vector<T>& v){raw("[");bool first=true;for(const auto& item:v){if(!first)raw(",");first=false;value(item);}raw("]");}
    template<class T>void field(std::string_view name,const T& v){key(name);value(v);}
    void id(Id n){field("id",std::to_string(n));}
    void value(const Bank& b){raw("{");field("msb",b.msb);field("lsb",b.lsb);raw("}");}
    void value(const Tempo& t){raw("{");field("tick",t.tick);field("microsPerQuarter",t.microsPerQuarter);raw("}");}
    void value(const Meter& m){raw("{");field("tick",m.tick);field("numerator",m.numerator);field("denominator",m.denominator);raw("}");}
    void value(const Key& k){raw("{");field("tick",k.tick);field("root",k.root);field("scale",scales[size_t(k.scale)]);raw("}");}
    void value(const Note& n){raw("{");id(n.id);field("pitch",n.pitch);field("velocity",n.velocity);field("startTick",n.start);field("durationTicks",n.duration);raw("}");}
    void value(const Control& c){raw("{");id(c.id);field("tick",c.tick);field("controller",c.controller);field("value",c.value);raw("}");}
    void value(const Section& s){raw("{");id(s.id);field("name",s.name);field("startTick",s.start);field("endTick",s.end);field("role",sectionRoles[size_t(s.role)]);raw("}");}
    void value(const Chord& c){raw("{");id(c.id);field("startTick",c.start);field("durationTicks",c.duration);field("root",c.root);field("pitchClasses",c.pitchClasses);field("voicing",c.voicing);field("inversion",c.inversion);field("tension",c.tension);field("explanation",c.explanation);raw("}");}
    void value(const Track& t){raw("{");id(t.id);field("name",t.name);field("role",roles[size_t(t.role)]);field("channel",t.channel);field("lowPitch",t.lowPitch);field("highPitch",t.highPitch);field("program",t.program);field("bank",t.bank);field("ownedPitches",t.ownedPitches.to_string());field("notes",t.notes);field("controls",t.controls);raw("}");}
    void value(const Composition& c){raw("{");field("schemaVersion",c.schemaVersion);field("ppq",c.ppq);field("engineVersion",c.engineVersion);field("seed",c.seed);field("id",c.id);field("style",c.style);field("source",c.source);field("legacyJson",c.legacyJson);field("timingNotice",c.timingNotice);field("endTick",c.endTick);field("tempos",c.tempos);field("meters",c.meters);field("keys",c.keys);field("sections",c.sections);field("chords",c.chords);field("tracks",c.tracks);raw("}");}
};
}
// Worker-only pure conversion. These functions do not change an active project.
inline CompositionJsonResult encodeCompositionJson(const Composition& c){
    try{auto valid=validate(c);if(!valid)return {{},valid.message};jsonDetail::SchemaWriter writer;writer.value(c);return {std::move(writer.output),{}};}
    catch(const jsonDetail::Invalid&){return {{},"Composition JSON exceeds limits or contains invalid UTF-8/NUL text"};}
    catch(const std::bad_alloc&){return {{},"Insufficient memory to save composition"};}
}
inline CompositionReadResult decodeCompositionJson(std::string_view text){
    if(text.empty()||text.size()>jsonDetail::maximumBytes)return {{},"Composition JSON exceeds byte limit or is empty"};
    try{jsonDetail::Reader reader{text};auto c=jsonDetail::readComposition(reader);if(!reader.done())throw jsonDetail::Invalid{};auto valid=validate(c);if(!valid)return {{},valid.message};return {std::move(c),{}};}
    catch(const jsonDetail::Invalid&){return {{},"Invalid composition-v2 JSON, field type or schema limit"};}
    catch(const std::bad_alloc&){return {{},"Insufficient memory to restore composition"};}
}
}
