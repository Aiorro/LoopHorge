#pragma once
#include "Direction.h"
#include <initializer_list>

namespace composer {
namespace promptDetail {
using Word=std::u32string_view;
struct Token {std::u32string text;size_t begin=0,end=0;};
inline bool digit(char32_t c){return c>=U'0'&&c<=U'9';}
inline bool word(char32_t c){return (c>=U'a'&&c<=U'z')||c==U'#'||c==U'&'||c==U'%'||c==U'-'||(c>=0xa0&&!(c>=0x2000&&c<=0x206f));}
inline bool tokenize(std::string_view input,std::vector<Token>& tokens){
    struct Point {char32_t c;size_t begin,end;};std::vector<Point> points;
    for(size_t i=0;i<input.size();){
        const size_t begin=i;auto byte=static_cast<unsigned char>(input[i++]);char32_t c=byte;int extra=0;
        if(byte>=0xc2&&byte<=0xdf){c=byte&31;extra=1;}else if(byte>=0xe0&&byte<=0xef){c=byte&15;extra=2;}
        else if(byte>=0xf0&&byte<=0xf4){c=byte&7;extra=3;}else if(byte>=0x80)return false;
        for(int n=0;n<extra;++n){if(i==input.size())return false;auto next=static_cast<unsigned char>(input[i++]);if((next&0xc0)!=0x80)return false;c=(c<<6)|(next&63);}
        constexpr char32_t minimum[]{0,0x80,0x800,0x10000};
        if(c<minimum[extra]||c>0x10ffff||(c>=0xd800&&c<=0xdfff)||(c<32&&c!=9&&c!=10&&c!=13)||(c>=0x7f&&c<=0x9f))return false;
        if(c>=U'A'&&c<=U'Z')c+=32;
        if(c>=U'А'&&c<=U'Я')c+=32;
        if(c==U'Ё'||c==U'ё')c=U'е';
        if(c==0xa0)c=U' ';
        if((c>=0x2010&&c<=0x2014)||c==0x2212)c=U'-';
        if(c==0x266f)c=U'#';
        if(c==0x266d)c=U'b';
        points.push_back({c,begin,i});
    }
    for(size_t i=0;i<points.size();){
        const auto start=i;bool number=digit(points[i].c)||((points[i].c==U'.'||points[i].c==U',')&&i+1<points.size()&&digit(points[i+1].c))||((points[i].c==U'-'||points[i].c==U'+')&&i+1<points.size()&&(digit(points[i+1].c)||points[i+1].c==U'-'||points[i+1].c==U'+'||points[i+1].c==U'.'||points[i+1].c==U','));
        if(number){
            while(i<points.size()){
                auto c=points[i].c;
                if(digit(c)||c==U'.'||c==U'/'||c==U'-'||c==U'+'){++i;continue;}
                if(c==U','&&i+1<points.size()&&digit(points[i+1].c)){++i;continue;}
                if(c==U'%' ){++i;break;}
                if(c==U'e'&&i+1<points.size()&&(digit(points[i+1].c)||points[i+1].c==U'-'||points[i+1].c==U'+')){++i;continue;}
                break;
            }
        }else if(word(points[i].c)){
            while(i<points.size()&&word(points[i].c)){
                if(points[i].c==U'-'&&i+1<points.size()&&(digit(points[i+1].c)||points[i+1].c==U'-'||points[i+1].c==U'+'||points[i+1].c==U'.'||points[i+1].c==U','))break;
                ++i;
            }
        }
        else {++i;continue;}
        Token token;token.begin=points[start].begin;token.end=points[i-1].end;
        for(size_t j=start;j<i;++j)token.text+=points[j].c;
        tokens.push_back(std::move(token));
    }
    return true;
}
inline std::string ascii(Word word){std::string out;for(auto c:word){if(c>127)return {};out+=char(c);}return out;}
inline bool real(Word word,double& value){
    const std::pair<Word,int> words[]={{U"one",1},{U"two",2},{U"three",3},{U"four",4},{U"five",5},{U"six",6},{U"eight",8},{U"sixteen",16},{U"один",1},{U"одна",1},{U"одну",1},{U"два",2},{U"две",2},{U"три",3},{U"четыре",4},{U"пять",5},{U"шесть",6},{U"восемь",8},{U"шестнадцать",16}};
    for(const auto& item:words)if(word==item.first){value=item.second;return true;}
    auto s=ascii(word);if(s.empty())return false;std::replace(s.begin(),s.end(),',','.');std::string_view text=s;if(text.front()=='+')text.remove_prefix(1);
    auto parsed=std::from_chars(text.data(),text.data()+text.size(),value,std::chars_format::general);
    return parsed.ec==std::errc{}&&parsed.ptr==text.data()+text.size()&&std::isfinite(value);
}
inline bool integer(Word word,int64_t& value){
    auto s=ascii(word);if(s.empty())return false;std::string_view text=s;if(text.front()=='+')text.remove_prefix(1);
    auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
    return parsed.ec==std::errc{}&&parsed.ptr==text.data()+text.size();
}
inline size_t match(const std::vector<Token>& tokens,size_t at,Word phrase){
    size_t count=0;
    while(!phrase.empty()){
        auto space=phrase.find(U' ');auto part=phrase.substr(0,space);
        if(at+count>=tokens.size()||tokens[at+count].text!=part)return 0;
        ++count;if(space==Word::npos)break;phrase.remove_prefix(space+1);
    }
    return count;
}
struct Alias {Word phrase;int value;};
inline std::optional<std::pair<int,size_t>> lookup(const std::vector<Token>& t,size_t i,std::initializer_list<Alias> aliases){
    std::optional<std::pair<int,size_t>> best;
    for(const auto& a:aliases){size_t n=match(t,i,a.phrase);if(n&&(!best||n>best->second))best={{a.value,n}};}
    return best;
}
inline std::optional<int> root(Word word){
    const std::pair<Word,int> names[]={{U"c",0},{U"b#",0},{U"c#",1},{U"db",1},{U"d",2},{U"d#",3},{U"eb",3},{U"e",4},{U"fb",4},{U"f",5},{U"e#",5},{U"f#",6},{U"gb",6},{U"g",7},{U"g#",8},{U"ab",8},{U"a",9},{U"a#",10},{U"bb",10},{U"b",11},{U"cb",11},{U"до",0},{U"ре",2},{U"ми",4},{U"фа",5},{U"соль",7},{U"ля",9},{U"си",11}};
    for(const auto& n:names)if(word==n.first)return n.second;
    auto dash=word.find(U'-');if(dash!=Word::npos){auto base=root(word.substr(0,dash));auto tail=word.substr(dash+1);
        if(base&&(tail==U"sharp"||tail==U"диез"))return (*base+1)%12;
        if(base&&(tail==U"flat"||tail==U"бемоль"))return (*base+11)%12;}
    return {};
}
inline std::optional<std::pair<int,Scale>> compactKey(Word word){
    for(size_t split=1;split<word.size();++split){auto prefix=word.substr(0,split),tail=word.substr(split);if(!prefix.empty()&&prefix.back()==U'-')prefix.remove_suffix(1);
        auto key=root(prefix);
        if(key&&(tail==U"minor"||tail==U"min"||tail==U"минор"))return {{*key,Scale::naturalMinor}};
        if(key&&(tail==U"major"||tail==U"maj"||tail==U"мажор"))return {{*key,Scale::major}};
    }
    return {};
}
inline bool negation(Word w){return w==U"no"||w==U"not"||w==U"without"||w==U"без"||w==U"не";}
inline std::optional<std::pair<int,size_t>> instrument(const std::vector<Token>& t,size_t i){
    return lookup(t,i,{{U"bass",0},{U"бас",0},{U"басом",0},{U"баса",0},{U"chords",1},{U"аккорды",1},{U"аккордов",1},{U"piano",1},{U"пианино",1},{U"фортепиано",1},
        {U"lead",2},{U"melody",2},{U"мелодия",2},{U"мелодии",2},{U"drums",3},{U"drum",3},{U"ударные",3},{U"ударных",3},{U"ударными",3},{U"барабаны",3},{U"барабанов",3},
        {U"percussion",4},{U"перкуссия",4},{U"перкуссии",4},{U"countermelody",5},{U"контрмелодия",5},{U"arpeggio",6},{U"арпеджио",6},{U"pads",7},{U"pad",7},{U"пэды",7},{U"пэдами",7},{U"пады",7},{U"strings",7},{U"струнные",7},{U"harmony",8},{U"guitar",8},{U"гитара",8}});
}

class Parser {
    std::string_view input;const ManualOverrides& manual;ResolvedConfig result;
    std::vector<Token> tokens;std::vector<bool> used;std::vector<Field> numberOwner;
    std::array<int,fieldCount> priority{};
    struct Conflict {Field field;int priority;size_t diagnostic;};std::vector<Conflict> conflicts;
    struct StyleMention {Style style;std::optional<int> weight;size_t at,count;};std::vector<StyleMention> styles;
    std::array<std::optional<bool>,roleCount> roles{};bool hasPositive=false,hasInstruments=false;
    std::array<std::optional<int>,roleCount> programs{};Span instrumentSpan{};
    bool instrumentExclusion=false;
    Span span(size_t i,size_t count=1)const{return {tokens[i].begin,tokens[i+count-1].end-tokens[i].begin};}
    void cover(size_t i,size_t count=1){for(size_t j=i;j<i+count;++j)used[j]=true;}
    void problem(Field f,const char* message,Span s={},Severity severity=Severity::error){
        if(severity==Severity::error&&f!=Field::count&&manual.pinned[size_t(f)])severity=Severity::warning;
        result.diagnostics.push_back({severity,f,s,message});
    }
    template<class Update> void set(Field f,size_t i,size_t count,int confidence,Update update){
        auto candidate=result.config;update(candidate);auto value=fieldValue(candidate,f);size_t k=size_t(f);
        result.recognized.push_back({f,span(i,count),value});cover(i,count);
        if(confidence>priority[k])for(const auto& conflict:conflicts)if(conflict.field==f&&conflict.priority<confidence){
            auto& diagnostic=result.diagnostics[conflict.diagnostic];diagnostic.severity=Severity::info;
            diagnostic.message="A more specific value supersedes the earlier hint conflict";
        }
        if(priority[k]&&fieldValue(result.config,f)!=value){
            if(confidence==priority[k]){conflicts.push_back({f,confidence,result.diagnostics.size()});problem(f,"Conflicting explicit values; pin a manual value or correct the direction",span(i,count));return;}
            if(confidence<priority[k])return;
        }
        if(confidence>=priority[k]){copyField(result.config,candidate,f);priority[k]=confidence;result.origins[k]=Origin::text;}
    }
    bool connected(size_t a,size_t b)const{
        size_t begin=tokens[a].end,end=tokens[b].begin;
        for(size_t i=begin;i<end;++i){
            const auto byte=static_cast<unsigned char>(input[i]);
            if(byte==' '||byte=='\t'||byte==':'||byte=='=')continue;
            if(byte==0xc2&&i+1<end&&static_cast<unsigned char>(input[i+1])==0xa0){++i;continue;}
            if(byte==0xe2&&i+2<end&&static_cast<unsigned char>(input[i+1])==0x80){
                auto last=static_cast<unsigned char>(input[i+2]);if((last>=0x80&&last<=0x8a)||last==0xaf){i+=2;continue;}}
            return false;
        }
        return true;
    }
    std::optional<size_t> number(size_t i,Field f,bool prefix){
        auto candidate=[&](size_t n){double x;return (numberOwner[n]==Field::count||numberOwner[n]==f)&&real(tokens[n].text,x);};
        std::optional<size_t> before,after;
        if(i&&connected(i-1,i)&&candidate(i-1))before=i-1;
        if(i+1<tokens.size()&&connected(i,i+1)&&candidate(i+1))after=i+1;
        return prefix?(after?after:before):(before?before:after);
    }
    void numeric(size_t i,Field field,bool prefix,int kind=0){
        auto n=number(i,field,prefix);
        if(!n){cover(i);problem(field,"Parameter requires a valid numeric value",span(i));return;}
        const size_t begin=std::min(i,*n),count=std::max(i,*n)-begin+1;
        double value=0;int64_t whole=0;bool valid=real(tokens[*n].text,value);
        if(field!=Field::duration)valid=integer(tokens[*n].text,whole);
        if(field==Field::bpm)valid=valid&&whole>=40&&whole<=240;
        else if(field==Field::seed)valid=valid&&whole>=0&&uint64_t(whole)<=4294967295ULL;
        else if(field!=Field::duration)valid=valid&&whole>=0&&whole<=100;
        if(field==Field::duration){if(kind==1)value*=60;valid=valid&&std::isfinite(value)&&value>0;
            valid=valid&&(kind==2?(value<=4096&&std::floor(value)==value):value<=1800);}
        cover(begin,count);numberOwner[*n]=field;
        if(!valid){problem(field,"Numeric value is malformed or outside supported bounds",span(begin,count));return;}
        set(field,begin,count,2,[&](auto& c){switch(field){
            case Field::bpm:c.recipe.bpm=int(whole);break;case Field::seed:c.recipe.seed=uint32_t(whole);break;
            case Field::duration:c.duration={kind==2?DurationUnit::bars:DurationUnit::seconds,value};break;
            case Field::humanization:c.recipe.humanization=int(whole);break;case Field::swing:c.recipe.swing=int(whole);break;
            case Field::complexity:c.mood.complexity=int(whole);break;case Field::darkness:c.mood.darkness=int(whole);break;
            case Field::energy:c.mood.energy=int(whole);break;case Field::melancholy:c.mood.melancholy=int(whole);break;
            case Field::tension:c.mood.tension=int(whole);break;case Field::aggression:c.mood.aggression=int(whole);break;
            case Field::atmosphere:c.mood.atmosphere=int(whole);break;default:break;}});
    }
    void role(size_t i,size_t length,int which,bool include){
        size_t k=size_t(which);hasInstruments=true;hasPositive=hasPositive||include;instrumentSpan=span(i,length);
        if(roles[k]&&*roles[k]!=include)problem(Field::instruments,"An instrument is both requested and excluded",span(i,length));
        else roles[k]=include;
        if(include&&(tokens[i].text==U"piano"||tokens[i].text==U"пианино"||tokens[i].text==U"фортепиано"))programs[k]=0;
        result.recognized.push_back({Field::instruments,span(i,length),std::string(include?"include ":"exclude ")+roleName(Role(which))});
        cover(i,length);
    }
    void finishStyles(){
        std::vector<StyleMention> unique;
        for(const auto& item:styles){auto existing=std::find_if(unique.begin(),unique.end(),[&](const auto& s){return s.style==item.style;});
            if(existing==unique.end())unique.push_back(item);else if(item.weight){if(existing->weight&&existing->weight!=item.weight)problem(Field::style,"Conflicting genre weights",span(item.at,item.count));else existing->weight=item.weight;}}
        if(unique.empty())return;
        if(unique.size()>2){problem(Field::style,"At most two genres can be resolved",span(unique[2].at,unique[2].count));return;}
        auto candidate=result.config;candidate.recipe.style=unique[0].style;
        if(unique.size()==1){if(unique[0].weight&&*unique[0].weight!=100)problem(Field::style,"A partial genre weight requires a second genre",span(unique[0].at,unique[0].count));}
        else {
            int first=unique[0].weight.value_or(unique[1].weight?100-*unique[1].weight:50);
            int second=unique[1].weight.value_or(100-first);
            if(first<0||first>100||second<0||second>100||first+second!=100)problem(Field::style,"Genre weights must sum to100",span(unique[1].at,unique[1].count));
            else candidate.blend=GenreMix{unique[1].style,first};
            if(!unique[0].weight||!unique[1].weight)problem(Field::style,"Missing genre weights default to50/50 or the explicit weight complement",span(unique[1].at,unique[1].count),Severity::info);
        }
        copyField(result.config,candidate,Field::style);result.origins[size_t(Field::style)]=Origin::text;
        for(const auto& item:styles)result.recognized.push_back({Field::style,span(item.at,item.count),fieldValue(result.config,Field::style)});
    }
    void scan(size_t i){
        const auto w=Word(tokens[i].text);
        if(w==U"-"&&i+1<tokens.size()&&connected(i,i+1)){
            double value=0;if(real(tokens[i+1].text,value)){cover(i);problem(Field::count,"Spaced minus is ambiguous; write a signed number without a space",span(i,2));return;}
        }
        if(w==U"with"||w==U"include"||w==U"add"||w==U"but"||w==U"с"||w==U"но"||w==U"добавь"||w==U"включи"||w==U"оставь"){
            instrumentExclusion=false;cover(i);return;
        }
        if(negation(w)){
            if(i+1<tokens.size())if(auto found=instrument(tokens,i+1)){role(i+1,found->second,found->first,false);instrumentExclusion=true;cover(i);return;}
            cover(i);problem(Field::count,"Negation is supported for instrument names only",span(i));return;
        }
        if(auto found=instrument(tokens,i)){role(i,found->second,found->first,!instrumentExclusion);return;}
        if(auto found=lookup(tokens,i,{{U"nu-metal",4},{U"nu metal",4},{U"numetal",4},{U"ню-метал",4},{U"ню метал",4},{U"ню-металл",4},{U"trap",0},{U"трэп",0},{U"треп",0},{U"boom-bap",1},{U"boom bap",1},{U"бум-бэп",1},{U"бум бэп",1},{U"r&b",2},{U"rnb",2},{U"r n b",2},{U"ambient",3},{U"эмбиент",3},{U"эмбиэнт",3}})){
            size_t length=found->second;std::optional<int> weight;
            auto percent=[&](size_t at){if(used[at])return false;auto text=Word(tokens[at].text);if(text.empty()||text.back()!=U'%')return false;int64_t value=0;
                if(!integer(text.substr(0,text.size()-1),value)||value<0||value>100)problem(Field::style,"Invalid genre percentage",span(at));else weight=int(value);cover(at);return true;};
            if(i&&connected(i-1,i))percent(i-1);
            if(!weight&&i+length<tokens.size()&&connected(i+length-1,i+length))percent(i+length);
            styles.push_back({Style(found->first),weight,i,length});cover(i,length);return;
        }
        if(w==U"min"&&number(i,Field::duration,false)){numeric(i,Field::duration,false,1);return;}
        if(auto found=lookup(tokens,i,{{U"natural minor",1},{U"натуральный минор",1},{U"harmonic minor",2},{U"гармонический минор",2},{U"melodic minor",3},{U"мелодический минор",3},
            {U"major pentatonic",8},{U"мажорная пентатоника",8},{U"minor pentatonic",9},{U"минорная пентатоника",9},{U"major",0},{U"maj",0},{U"мажор",0},{U"мажоре",0},
            {U"minor",1},{U"min",1},{U"минор",1},{U"миноре",1},{U"dorian",4},{U"дорийский",4},{U"дорийском",4},{U"phrygian",5},{U"фригийский",5},{U"фригийском",5},
            {U"lydian",6},{U"лидийский",6},{U"mixolydian",7},{U"миксолидийский",7},{U"blues",10},{U"блюз",10}})){
            set(Field::scale,i,found->second,2,[&](auto& c){c.recipe.scale=Scale(found->first);});
            if(i){size_t at=i-1;int accidental=0;auto prior=Word(tokens[at].text);
                if(prior==U"flat"||prior==U"бемоль"||prior==U"sharp"||prior==U"диез"){accidental=(prior==U"flat"||prior==U"бемоль")?-1:1;if(at)--at;}
                if(auto key=root(tokens[at].text))set(Field::root,at,i-at,2,[&](auto& c){c.recipe.root=(*key+accidental+12)%12;});}
            return;
        }
        if(auto key=compactKey(w)){
            set(Field::root,i,1,2,[&](auto& c){c.recipe.root=key->first;});set(Field::scale,i,1,2,[&](auto& c){c.recipe.scale=key->second;});return;
        }
        if(w==U"key"||w==U"тональность"){
            cover(i);
            if(i+1<tokens.size()){
                if(auto key=root(tokens[i+1].text)){
                    int value=*key;size_t count=2;
                    if(i+2<tokens.size()){auto accidental=Word(tokens[i+2].text);
                        if(accidental==U"flat"||accidental==U"бемоль"){value=(value+11)%12;count=3;}
                        if(accidental==U"sharp"||accidental==U"диез"){value=(value+1)%12;count=3;}}
                    set(Field::root,i,count,2,[&](auto& c){c.recipe.root=value;});return;
                }
                if(auto key=compactKey(tokens[i+1].text)){
                    set(Field::root,i,2,2,[&](auto& c){c.recipe.root=key->first;});set(Field::scale,i,2,2,[&](auto& c){c.recipe.scale=key->second;});return;
                }
            }
            problem(Field::root,"Key requires a recognized note name",span(i));return;
        }
        auto plain=ascii(w);if(plain.find('/')!=std::string::npos){auto slash=plain.find('/');int64_t top=0,bottom=0;
            if(integer(w.substr(0,slash),top)&&integer(w.substr(slash+1),bottom)&&top>=1&&top<=12&&(bottom==2||bottom==4||bottom==8||bottom==16))set(Field::meter,i,1,2,[&](auto& c){c.recipe.numerator=int(top);c.recipe.denominator=int(bottom);});
            else {cover(i);problem(Field::meter,"Invalid time signature",span(i));}return;}
        if(w==U"bpm"||w==U"tempo"||w==U"темп"){numeric(i,Field::bpm,w!=U"bpm");return;}
        if(w==U"seed"||w==U"сид"){numeric(i,Field::seed,true);return;}
        if(w==U"minute"||w==U"minutes"||w==U"min"||w==U"минуты"||w==U"минут"||w==U"минуту"){numeric(i,Field::duration,false,1);return;}
        if(w==U"second"||w==U"seconds"||w==U"sec"||w==U"секунд"||w==U"секунды"||w==U"секунду"){numeric(i,Field::duration,false,0);return;}
        if(w==U"bar"||w==U"bars"||w==U"тактов"||w==U"такты"||w==U"такт"){numeric(i,Field::duration,false,2);return;}
        const std::pair<Word,Field> labels[]={{U"humanization",Field::humanization},{U"humanize",Field::humanization},{U"гуманизация",Field::humanization},{U"swing",Field::swing},{U"свинг",Field::swing},{U"complexity",Field::complexity},{U"сложность",Field::complexity},
            {U"darkness",Field::darkness},{U"мрачность",Field::darkness},{U"energy",Field::energy},{U"энергия",Field::energy},{U"melancholy",Field::melancholy},{U"меланхолия",Field::melancholy},
            {U"tension",Field::tension},{U"напряжение",Field::tension},{U"aggression",Field::aggression},{U"агрессия",Field::aggression},{U"atmosphere",Field::atmosphere},{U"атмосфера",Field::atmosphere}};
        for(const auto& label:labels)if(w==label.first){numeric(i,label.second,true);return;}
        if(auto found=lookup(tokens,i,{{U"half-time",0},{U"half time",0},{U"халфтайм",0},{U"double-time",2},{U"double time",2},{U"даблтайм",2},{U"normal time",1}})){set(Field::feel,i,found->second,2,[&](auto& c){c.recipe.feel=Feel(found->first);});return;}
        if(auto found=lookup(tokens,i,{{U"loop",0},{U"луп",0},{U"петля",0},{U"song",1},{U"composition",1},{U"песня",1},{U"песню",1},{U"композиция",1},{U"композицию",1}})){set(Field::mode,i,found->second,2,[&](auto& c){c.mode=RequestMode(found->first);});return;}
        if(auto found=lookup(tokens,i,{{U"pre chorus",2},{U"pre-chorus",2},{U"intro",0},{U"вступление",0},{U"вступлением",0},{U"verse",1},{U"куплет",1},{U"chorus",3},{U"припев",3},{U"припевом",3},{U"bridge",4},{U"бридж",4},{U"breakdown",5},{U"брейкдаун",5},{U"solo",6},{U"соло",6},{U"outro",7},{U"аутро",7}})){
            cover(i,found->second);if(result.config.sections.size()<32)result.config.sections.push_back(SectionRole(found->first));else problem(Field::sections,"Too many requested sections",span(i,found->second));
            result.origins[size_t(Field::sections)]=Origin::text;result.recognized.push_back({Field::sections,span(i,found->second),fieldValue(result.config,Field::sections)});return;}
        struct Hint {Word word;Field field;int value;};
        const Hint hints[]={{U"dark",Field::darkness,80},{U"мрачный",Field::darkness,80},{U"мрачную",Field::darkness,80},{U"мрачная",Field::darkness,80},{U"темный",Field::darkness,80},{U"темную",Field::darkness,80},{U"bright",Field::darkness,20},{U"светлый",Field::darkness,20},
            {U"energetic",Field::energy,80},{U"энергичный",Field::energy,80},{U"calm",Field::energy,20},{U"спокойный",Field::energy,20},
            {U"melancholic",Field::melancholy,80},{U"меланхоличный",Field::melancholy,80},{U"меланхоличным",Field::melancholy,80},{U"happy",Field::melancholy,20},{U"веселый",Field::melancholy,20},
            {U"tense",Field::tension,80},{U"напряженный",Field::tension,80},{U"relaxed",Field::tension,20},
            {U"aggressive",Field::aggression,80},{U"heavy",Field::aggression,80},{U"агрессивный",Field::aggression,80},{U"агрессивными",Field::aggression,80},{U"тяжелый",Field::aggression,80},{U"тяжелым",Field::aggression,80},{U"soft",Field::aggression,20},{U"мягкий",Field::aggression,20},
            {U"atmospheric",Field::atmosphere,80},{U"cinematic",Field::atmosphere,80},{U"атмосферный",Field::atmosphere,80},{U"атмосферную",Field::atmosphere,80},{U"кинематографический",Field::atmosphere,80},{U"кинематографическими",Field::atmosphere,80},{U"кинематографичным",Field::atmosphere,80},{U"dry",Field::atmosphere,20},
            {U"complex",Field::complexity,80},{U"сложный",Field::complexity,80},{U"simple",Field::complexity,20},{U"простой",Field::complexity,20}};
        for(const auto& hint:hints)if(w==hint.word){set(hint.field,i,1,1,[&](auto& c){switch(hint.field){case Field::darkness:c.mood.darkness=hint.value;break;case Field::energy:c.mood.energy=hint.value;break;case Field::melancholy:c.mood.melancholy=hint.value;break;case Field::tension:c.mood.tension=hint.value;break;case Field::aggression:c.mood.aggression=hint.value;break;case Field::atmosphere:c.mood.atmosphere=hint.value;break;case Field::complexity:c.mood.complexity=hint.value;break;default:break;}});return;}
    }
public:
    Parser(std::string_view text,uint32_t seed,const ManualOverrides& overrides):input(text),manual(overrides){result.config.recipe.seed=seed;}
    ResolvedConfig run(){
        if(input.size()>4096||!tokenize(input,tokens)){problem(Field::count,"Direction exceeds4096bytes or contains invalid UTF-8/control data",{0,input.size()});return result;}
        used.resize(tokens.size());numberOwner.resize(tokens.size(),Field::count);
        for(size_t i=0;i<tokens.size();++i){
            if(i){auto gap=input.substr(tokens[i-1].end,tokens[i].begin-tokens[i-1].end);if(gap.find_first_of(".;!?\r\n")!=std::string_view::npos)instrumentExclusion=false;}
            if(!used[i])scan(i);
        }
        finishStyles();
        if(hasInstruments){if(hasPositive)result.config.instruments.reset();for(size_t i=0;i<roleCount;++i)if(roles[i])result.config.instruments[i]=*roles[i];
            result.config.programs=programs;result.origins[size_t(Field::instruments)]=Origin::text;result.recognized.push_back({Field::instruments,instrumentSpan,fieldValue(result.config,Field::instruments)});}
        for(size_t i=0;i<fieldCount;++i)if(manual.pinned[i]){
            if(Field(i)==Field::sections&&manual.values.sections.size()>32){problem(Field::count,"Manual section list exceeds limit");continue;}
            copyField(result.config,manual.values,Field(i));result.origins[i]=Origin::manual;
        }
        if(result.origins[size_t(Field::duration)]==Origin::defaultValue&&result.config.mode==RequestMode::song)result.config.duration={DurationUnit::seconds,180};
        if(result.origins[size_t(Field::mode)]==Origin::defaultValue&&result.config.duration.unit==DurationUnit::seconds){result.config.mode=RequestMode::song;result.origins[size_t(Field::mode)]=result.origins[size_t(Field::duration)];}
        const Word fillers[]={U"create",U"generate",U"make",U"a",U"an",U"the",U"in",U"with",U"and",U"for",U"at",U"of",U"please",U"full",U"blend",U"mix",U"создай",U"сгенерируй",U"в",U"на",U"с",U"и",U"для",U"пожалуйста",U"полную",U"полный",U"полная",U"музыку"};
        for(size_t i=0;i<tokens.size();++i)for(auto filler:fillers)if(tokens[i].text==filler)used[i]=true;
        for(size_t i=0;i<tokens.size();){if(used[i]){++i;continue;}size_t start=i;while(i<tokens.size()&&!used[i])++i;
            auto region=span(start,i-start);result.diagnostics.push_back({Severity::warning,Field::count,region,"Unrecognized fragment: "+std::string(input.substr(region.start,region.length))});}
        auto checked=validateDirection(result.config);if(!checked)problem(Field::count,checked.message.c_str());
        return result;
    }
};
}
class LocalPromptInterpreter final:public IPromptInterpreter {
public:
    ResolvedConfig interpret(std::string_view text,uint32_t seed,const ManualOverrides& manual={})const override{return promptDetail::Parser(text,seed,manual).run();}
};
}
