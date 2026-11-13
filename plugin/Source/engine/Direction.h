#pragma once
#include "Config.h"
#include <array>
#include <bitset>
#include <charconv>
#include <string_view>

namespace composer {
enum class RequestMode {loop,song};
enum class DurationUnit {bars,seconds};
struct Duration {DurationUnit unit=DurationUnit::bars;double amount=16;};
struct GenreMix {Style secondary=Style::ambient;int primaryWeight=50;};
inline constexpr size_t roleCount=9;
struct CompositionConfig {
    Config recipe;
    Mood mood;
    Duration duration;
    RequestMode mode=RequestMode::loop;
    std::bitset<roleCount> instruments{7};
    std::array<std::optional<int>,roleCount> programs{};
    std::optional<GenreMix> blend;
    std::vector<SectionRole> sections;
    CompositionConfig(){recipe.bars=16;}
};
enum class Field {root,scale,style,bpm,duration,meter,seed,feel,humanization,swing,complexity,instruments,mode,darkness,energy,melancholy,tension,aggression,atmosphere,sections,count};
inline constexpr size_t fieldCount=size_t(Field::count);
enum class Origin {defaultValue,text,manual};
enum class Severity {info,warning,error};
struct Span {size_t start=0,length=0;};
struct Recognition {Field field=Field::count;Span span;std::string value;};
struct Diagnostic {Severity severity=Severity::warning;Field field=Field::count;Span span;std::string message;};
struct ManualOverrides {CompositionConfig values;std::bitset<fieldCount> pinned;};
struct ResolvedConfig {
    CompositionConfig config;
    std::array<Origin,fieldCount> origins{};
    std::vector<Recognition> recognized;
    std::vector<Diagnostic> diagnostics;
    bool valid()const{for(const auto& d:diagnostics)if(d.severity==Severity::error)return false;return true;}
};
inline const char* roleName(Role role){
    switch(role){case Role::bass:return "bass";case Role::chords:return "chords";case Role::lead:return "lead";
        case Role::drums:return "drums";case Role::percussion:return "percussion";case Role::counterMelody:return "countermelody";
        case Role::arpeggio:return "arpeggio";case Role::pads:return "pads";case Role::harmony:return "harmony";}
    return "invalid";
}
inline const char* scaleName(Scale scale){
    switch(scale){case Scale::major:return "major";case Scale::naturalMinor:return "natural minor";
        case Scale::harmonicMinor:return "harmonic minor";case Scale::melodicMinor:return "melodic minor";
        case Scale::dorian:return "dorian";case Scale::phrygian:return "phrygian";case Scale::lydian:return "lydian";
        case Scale::mixolydian:return "mixolydian";case Scale::majorPentatonic:return "major pentatonic";
        case Scale::minorPentatonic:return "minor pentatonic";case Scale::blues:return "blues";}
    return "invalid";
}
inline std::string fieldValue(const CompositionConfig& c,Field field){
    const auto& r=c.recipe;const auto& m=c.mood;
    switch(field){
        case Field::root:{const char* names[]{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};return r.root>=0&&r.root<12?names[r.root]:"invalid";}
        case Field::scale:return scaleName(r.scale);
        case Field::style:{if(c.blend&&(c.blend->primaryWeight<0||c.blend->primaryWeight>100))return "invalid";std::string value=styleName(r.style);if(c.blend)value+=" "+std::to_string(c.blend->primaryWeight)+"% + "+styleName(c.blend->secondary)+" "+std::to_string(100-c.blend->primaryWeight)+"%";return value;}
        case Field::bpm:return std::to_string(r.bpm);
        case Field::duration:{if((c.duration.unit!=DurationUnit::bars&&c.duration.unit!=DurationUnit::seconds)||!std::isfinite(c.duration.amount))return "invalid";char number[64];auto out=std::to_chars(number,number+64,c.duration.amount);if(out.ec!=std::errc{})return "invalid";return std::string(number,out.ptr)+(c.duration.unit==DurationUnit::bars?" bars":" seconds");}
        case Field::meter:return std::to_string(r.numerator)+"/"+std::to_string(r.denominator);
        case Field::seed:return std::to_string(r.seed);
        case Field::feel:return r.feel==Feel::half?"half":r.feel==Feel::normal?"normal":r.feel==Feel::doubleTime?"double":"invalid";
        case Field::humanization:return std::to_string(r.humanization);
        case Field::swing:return std::to_string(r.swing);
        case Field::complexity:return std::to_string(m.complexity);
        case Field::mode:return c.mode==RequestMode::loop?"loop":c.mode==RequestMode::song?"song":"invalid";
        case Field::darkness:return std::to_string(m.darkness);case Field::energy:return std::to_string(m.energy);
        case Field::melancholy:return std::to_string(m.melancholy);case Field::tension:return std::to_string(m.tension);
        case Field::aggression:return std::to_string(m.aggression);case Field::atmosphere:return std::to_string(m.atmosphere);
        case Field::instruments:{std::string value;for(size_t i=0;i<roleCount;++i)if(c.instruments[i]){if(!value.empty())value+=", ";value+=roleName(Role(i));if(c.programs[i])value+=" (GM"+std::to_string(*c.programs[i])+")";}return value;}
        case Field::sections:{std::string value;for(auto role:c.sections)value+=std::to_string(int(role))+",";return value;}
        case Field::count:return {};
    }
    return {};
}
inline void copyField(CompositionConfig& to,const CompositionConfig& from,Field f){
    switch(f){
        case Field::root:to.recipe.root=from.recipe.root;break;case Field::scale:to.recipe.scale=from.recipe.scale;break;
        case Field::style:to.recipe.style=from.recipe.style;to.blend=from.blend;break;
        case Field::bpm:to.recipe.bpm=from.recipe.bpm;break;case Field::duration:to.duration=from.duration;break;
        case Field::meter:to.recipe.numerator=from.recipe.numerator;to.recipe.denominator=from.recipe.denominator;break;
        case Field::seed:to.recipe.seed=from.recipe.seed;break;case Field::feel:to.recipe.feel=from.recipe.feel;break;
        case Field::humanization:to.recipe.humanization=from.recipe.humanization;break;case Field::swing:to.recipe.swing=from.recipe.swing;break;
        case Field::complexity:to.mood.complexity=from.mood.complexity;break;
        case Field::instruments:to.instruments=from.instruments;to.programs=from.programs;break;
        case Field::mode:to.mode=from.mode;break;case Field::sections:to.sections=from.sections;break;
        case Field::darkness:to.mood.darkness=from.mood.darkness;break;case Field::energy:to.mood.energy=from.mood.energy;break;
        case Field::melancholy:to.mood.melancholy=from.mood.melancholy;break;case Field::tension:to.mood.tension=from.mood.tension;break;
        case Field::aggression:to.mood.aggression=from.mood.aggression;break;case Field::atmosphere:to.mood.atmosphere=from.mood.atmosphere;break;
        case Field::count:break;
    }
}
inline ValidationResult validateDirection(const CompositionConfig& c){
    auto fail=[](const char* text){return ValidationResult{ValidationCode::identity,text};};
    if(!validateConfig(c.recipe))return fail("Invalid loop recipe parameters");
    if(!validMood(c.mood))return fail("Mood axes must be between0 and100");
    if(c.mode!=RequestMode::loop&&c.mode!=RequestMode::song)return fail("Invalid request mode");
    if(c.duration.unit!=DurationUnit::bars&&c.duration.unit!=DurationUnit::seconds)return fail("Invalid duration unit");
    if(!std::isfinite(c.duration.amount)||c.duration.amount<=0)return fail("Duration must be positive and finite");
    if(c.duration.unit==DurationUnit::bars){
        if(std::floor(c.duration.amount)!=c.duration.amount||c.duration.amount>4096)return fail("Requested bars exceed supported bounds");
        double seconds=c.duration.amount*4*c.recipe.numerator/c.recipe.denominator*std::llround(60000000.0/c.recipe.bpm)/1000000.0;
        if(seconds>limits::seconds)return fail("Requested duration exceeds thirty minutes");
    }else if(c.duration.amount>limits::seconds)return fail("Requested duration exceeds thirty minutes");
    if(c.instruments.none())return fail("At least one instrument is required");
    for(const auto& program:c.programs)if(program&&(*program<0||*program>127))return fail("Invalid instrument program");
    if(c.blend&&(c.blend->secondary<Style::trap||c.blend->secondary>Style::nuMetal||c.blend->secondary==c.recipe.style||c.blend->primaryWeight<0||c.blend->primaryWeight>100))return fail("Invalid two-genre mix");
    if(c.sections.size()>32)return fail("Too many requested sections");
    for(auto role:c.sections)if(role<SectionRole::intro||role>SectionRole::outro)return fail("Invalid section role");
    return {};
}
class IPromptInterpreter {
public:
    virtual ~IPromptInterpreter()=default;
    virtual ResolvedConfig interpret(std::string_view,uint32_t callerSeed,const ManualOverrides&)const=0;
};
}
