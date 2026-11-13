#include "../Source/engine/PromptCompose.h"
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace composer;
#define check(x) do {if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
ResolvedConfig parse(std::string_view text,const ManualOverrides& manual={}){return LocalPromptInterpreter{}.interpret(text,42,manual);}
bool warning(const ResolvedConfig& r){for(const auto& d:r.diagnostics)if(d.severity==Severity::warning)return true;return false;}
void spans(const ResolvedConfig& r,std::string_view input){for(const auto& m:r.recognized)check(m.span.start<=input.size()&&m.span.length<=input.size()-m.span.start);}
int main(){
    auto r=parse("");check(r.valid());check(r.config.recipe.root==0&&r.config.recipe.scale==Scale::naturalMinor&&r.config.recipe.bpm==100);
    check(r.config.duration.unit==DurationUnit::bars&&r.config.duration.amount==16&&r.config.mode==RequestMode::loop);
    check(r.config.recipe.seed==42&&r.config.mood.energy==50);
    for(auto source:r.origins)check(source==Origin::defaultValue);
    auto generated=generateResolvedLoop(r);check(generated.composition&&generated.composition->endTick.value==30720);

    const std::string en="Create a dark atmospheric nu-metal composition in D minor, 105 BPM, 3 minutes, with aggressive drums, heavy bass, melancholic piano, cinematic pads and an emotional chorus.";
    r=parse(en);check(r.valid());spans(r,en);
    check(r.config.recipe.root==2&&r.config.recipe.scale==Scale::naturalMinor&&r.config.recipe.style==Style::nuMetal&&r.config.recipe.bpm==105);
    check(r.config.duration.unit==DurationUnit::seconds&&r.config.duration.amount==180&&r.config.mode==RequestMode::song);
    check(r.config.mood.darkness==80&&r.config.mood.atmosphere==80&&r.config.mood.aggression==80&&r.config.mood.melancholy==80);
    check(r.config.instruments.test(size_t(Role::drums))&&r.config.instruments.test(size_t(Role::pads)));
    check(r.config.programs[size_t(Role::chords)]==0);check(!generateResolvedLoop(r).composition);
    const std::string ru="Создай МРАЧНУЮ атмосферную композицию NU METAL в D MINOR, 105 BPM, на три минуты, с агрессивными ударными, тяжёлым басом, меланхоличным фортепиано, кинематографическими пэдами и эмоциональным припевом.";
    auto translated=parse(ru);check(translated.valid());spans(translated,ru);
    check(translated.config.recipe.root==r.config.recipe.root&&translated.config.recipe.bpm==105&&translated.config.duration.amount==180);
    check(translated.config.mood.darkness==80&&translated.config.mood.atmosphere==80&&translated.config.mood.aggression==80);

    r=parse("ТЁМНЫЙ trap в C♯ minor, 8 тактов, 110BPM, 6/8, seed99, пианино и ударные, без баса");check(r.valid());
    check(r.config.recipe.root==1&&r.config.recipe.seed==99&&r.config.recipe.numerator==6&&r.config.recipe.denominator==8);
    check(r.config.mood.darkness==80&&r.origins[size_t(Field::seed)]==Origin::text);
    generated=generateResolvedLoop(r);check(generated.composition&&generated.composition->tracks.size()==2&&generated.composition->endTick.value==11520);
    check(generated.composition->tracks[0].role==Role::chords&&generated.composition->tracks[0].program==0);
    check(generated.composition->tracks[1].role==Role::drums&&generated.composition->tracks[1].channel==10);
    check(parse("РЕ БЕМОЛЬ МАЖОР").config.recipe.root==1);check(parse("РЕ БЕМОЛЬ МАЖОР").config.recipe.scale==Scale::major);
    check(parse("Bbminor").config.recipe.root==10);check(parse("C# harmonic minor").config.recipe.scale==Scale::harmonicMinor);
    check(parse("3min").config.duration.amount==180);check(parse("C min").config.recipe.scale==Scale::naturalMinor);check(parse("1.5 minutes").config.duration.amount==90);check(parse("3,5 минуты").config.duration.amount==210);check(parse(",5 minutes").config.duration.amount==30);check(!parse("100,50BPM").valid());check(parse("180 seconds").config.duration.amount==180);
    check(parse("seed4294967295").config.recipe.seed==std::numeric_limits<uint32_t>::max());
    check(parse("half-time").config.recipe.feel==Feel::half);check(parse("double time").config.recipe.feel==Feel::doubleTime);
    check(parse("humanization35 swing70 complexity90").config.recipe.humanization==35);
    check(parse("humanization35 swing70 complexity90").config.mood.complexity==90);
    check(parse("darkness90 dark").valid()&&parse("darkness90 dark").config.mood.darkness==90);
    check(!parse("dark bright").valid());check(!parse("100 bpm 120 bpm").valid());
    check(!parse("D minor C major").valid());check(!parse("drums and no drums").valid());check(!parse("not trap").valid());
    r=parse("make ultraviolet jazz");check(r.valid()&&warning(r));
    r=parse("cinematic");check(r.config.recipe.root==0&&r.origins[size_t(Field::root)]==Origin::defaultValue);

    ManualOverrides manual;manual.pinned.set(size_t(Field::bpm));manual.values.recipe.bpm=111;
    r=parse("100 bpm 120 bpm D minor",manual);check(r.valid()&&warning(r));check(r.config.recipe.bpm==111&&r.config.recipe.root==2);
    check(r.origins[size_t(Field::bpm)]==Origin::manual);
    manual.values.recipe.bpm=0;check(!parse("100 bpm",manual).valid());
    manual={};manual.pinned.set(size_t(Field::darkness));manual.values.mood.darkness=70;
    r=parse("dark bright",manual);check(r.valid()&&warning(r)&&r.config.mood.darkness==70);
    manual={};manual.pinned.set(size_t(Field::instruments));manual.values.instruments.reset();manual.values.instruments.set(size_t(Role::chords));
    r=parse("drums and no drums",manual);check(r.valid()&&generateResolvedLoop(r).composition);
    check(!parse("no bass no chords no melody no drums").valid());

    r=parse("blend trap70% and ambient30%");check(r.valid()&&r.config.blend&&r.config.blend->primaryWeight==70&&r.config.blend->secondary==Style::ambient);
    check(!generateResolvedLoop(r).composition);check(!parse("trap70% ambient40%").valid());
    r=parse("trap ambient");check(r.valid()&&r.config.blend&&r.config.blend->primaryWeight==50);bool defaultWeights=false;for(const auto& d:r.diagnostics)if(d.severity==Severity::info)defaultWeights=true;check(defaultWeights);
    r=parse("loop 8 bars with pads");check(r.valid()&&!generateResolvedLoop(r).composition);
    r=parse("loop 3 bars");check(r.valid()&&r.config.duration.amount==3&&!generateResolvedLoop(r).composition);
    r=parse("intro chorus outro");check(r.valid()&&r.config.sections.size()==3&&!generateResolvedLoop(r).composition);
    for(const auto* text:{"0 BPM","-40 BPM","100.5 BPM","999999999999999999999999 BPM","13/8","4/3","0/4","seed-1","seed1.5","seed4294967296","energy101","humanization-1","0 minutes","-3 minutes","NaN seconds","infinity minutes"}){if(parse(text).valid())std::cerr<<"accepted invalid direction: "<<text<<"\n";check(!parse(text).valid());}
    check(!parse(std::string(4097,'x')).valid());check(parse(std::string(4096,' ')).valid());
    for(const auto& invalid:{std::string("dark\0bright",11),std::string("\xC0\xAF",2),std::string("\xED\xA0\x80",3),std::string("\xF4\x90\x80\x80",4),std::string("\xE2\x82",2),std::string("\x80",1),std::string("\x01",1)})check(!parse(invalid).valid());
    {CompositionConfig malformed;malformed.blend=GenreMix{Style::ambient,std::numeric_limits<int>::min()};check(fieldValue(malformed,Field::style)=="invalid");}
    {auto pinned=ManualOverrides{};pinned.pinned.set(size_t(Field::root));pinned.values.recipe.root=12;check(!parse("D minor",pinned).valid());}
    {auto pinned=ManualOverrides{};pinned.pinned.set(size_t(Field::instruments));pinned.values.programs[size_t(Role::chords)]=128;check(!parse("piano",pinned).valid());}
    check(parse("seed42, bpm110").config.recipe.bpm==110);check(parse("tempo100bpm").valid());
    check(!parse("100... BPM").valid());check(!parse("100? BPM").valid());check(parse("42\xE2\x80\xA8" "bpm110").config.recipe.bpm==110);check(parse("105\xC2\xA0" "BPM").config.recipe.bpm==105);
    for(const auto& sample:{en,ru,std::string("D minor 8 bars 100 bpm seed42 with piano and drums")}){
        auto a=parse(sample),b=parse(sample);check(a.valid()&&b.valid());check(a.config.recipe.seed==b.config.recipe.seed&&a.diagnostics.size()==b.diagnostics.size());spans(a,sample);
    }
    check(!parse("trap NaN% ambient").valid());
    {auto piano=generateResolvedLoop(parse("piano")),chords=generateResolvedLoop(parse("chords")),bass=generateResolvedLoop(parse("bass"));
        check(piano.composition&&chords.composition&&bass.composition);check(piano.composition->id!=chords.composition->id);check(chords.composition->id!=bass.composition->id);}
    r=parse("loop 8 bars D minor 100 bpm seed42 with piano and drums dark aggressive");
    check(r.valid());generated=generateResolvedLoop(r);check(generated.composition&&validate(*generated.composition));
    {
        int failures=0;auto expect=[&](bool value,const char* name){if(!value){std::cerr<<"review regression: "<<name<<"\n";++failures;}};
        for(const auto* text:{"key C sharp minor","key D flat major","тональность ре бемоль мажор","key C#minor"}){
            auto key=parse(text);expect(key.valid()&&key.config.recipe.root==1,text);
        }
        for(const auto* text:{"trap70% ambient","70% trap 30% ambient"}){auto mix=parse(text);expect(mix.valid()&&mix.config.blend&&mix.config.blend->primaryWeight==70,text);}
        auto first=parse("dark bright darkness90"),last=parse("darkness90 dark bright");
        expect(first.valid()&&last.valid()&&first.config.mood.darkness==90&&last.config.mood.darkness==90,"numeric priority in both orders");
        auto a=generateResolvedLoop(first),b=generateResolvedLoop(last);expect(a.composition&&b.composition&&a.composition->id==b.composition->id,"numeric priority actual generation");
        auto excluded=parse("without bass, chords and lead");expect(!excluded.valid()&&!generateResolvedLoop(excluded).composition,"coordinated exclusions do not become inclusions");
        auto selected=parse("without bass, chords and lead with drums");auto onlyDrums=generateResolvedLoop(selected);
        expect(selected.valid()&&onlyDrums.composition&&onlyDrums.composition->tracks.size()==1&&onlyDrums.composition->tracks[0].role==Role::drums,"negative list with explicit positive reset");
        auto noBassDrums=generateResolvedLoop(parse("no bass and drums"));
        expect(noBassDrums.composition&&noBassDrums.composition->tracks.size()==2&&noBassDrums.composition->tracks[0].role==Role::chords&&noBassDrums.composition->tracks[1].role==Role::lead,"negative list output tracks");
        expect(!parse("- 120 BPM").valid(),"spaced negative tempo is not positive tempo");
        expect(!parse("- 8 bars").valid()&&!generateResolvedLoop(parse("- 8 bars")).composition,"spaced negative bars never render positively");
        check(failures==0);
    }
    uint32_t fuzz=42;
    for(size_t trial=0;trial<2048;++trial){std::string input;for(size_t i=0;i<trial%129;++i){fuzz^=fuzz<<13;fuzz^=fuzz>>17;fuzz^=fuzz<<5;input+=char(fuzz&255);}
        auto result=parse(input);spans(result,input);if(result.valid())check(validateDirection(result.config));}
    std::cout<<"RU/EN offline prompt: typed settings, sources, conflicts, UTF8, manual precedence and real loop execution passed\n";
}
