#pragma once
#include <juce_core/juce_core.h>
#include "Schedule.h"
#include <charconv>
#include <limits>
#include <string>
#include <string_view>
namespace wire {
inline bool member(const juce::var& value,std::initializer_list<const char*> allowed){if(!value.isString())return false;for(auto name:allowed)if(value.toString()==name)return true;return false;}
inline bool integer(const juce::var& value,double lo,double hi){return (value.isInt()||value.isInt64()||value.isDouble())&&std::isfinite(double(value))&&std::floor(double(value))==double(value)&&double(value)>=lo&&double(value)<=hi;}
inline bool meter(const juce::var& value){return value.isObject()&&value.getDynamicObject()->getProperties().size()==2&&integer(value["numerator"],4,4)&&integer(value["denominator"],4,4);}
// Tokenize before JUCE: its parser accepts malformed escapes and wraps integer literals.
inline juce::var document(std::string_view text){
 constexpr size_t limit=4*1024*1024;
 if(text.empty()||text.size()>limit||text.find('\0')!=std::string_view::npos||!juce::CharPointer_UTF8::isValidString(text.data(),int(text.size())))return {};
 std::string canonical;canonical.reserve(text.size());std::array<char,32> stack{};int depth=0;bool started=false;char previous=0;
 auto space=[](char c){return c==' '||c=='\t'||c=='\r'||c=='\n';};
 auto digit=[](char c){return c>='0'&&c<='9';};
 auto hex=[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');};
 for(size_t i=0;i<text.size();){
  char c=text[i];if(space(c)){canonical+=c;++i;continue;}
  if(started&&depth==0)return {};
  if(!started){if(c!='{')return {};started=true;}
  if(c=='"'){
   size_t start=i++;bool closed=false;
   while(i<text.size()){
    auto ch=static_cast<unsigned char>(text[i++]);if(ch=='"'){closed=true;break;}if(ch<32)return {};
    if(ch=='\\'){
     if(i==text.size())return {};char escape=text[i++];
     if(escape=='u'){
      if(i+4>text.size())return {};for(size_t j=i;j<i+4;++j)if(!hex(text[j]))return {};
      if(text.substr(i,4)=="0000")return {};i+=4;
     }else if(std::string_view("\"\\/bfnrt").find(escape)==std::string_view::npos)return {};
    }
   }
   if(!closed)return {};canonical.append(text.substr(start,i-start));previous='v';
  }else if(c=='-'||digit(c)){
   size_t start=i;if(c=='-')++i;if(i==text.size())return {};
   if(text[i]=='0'){++i;if(i<text.size()&&digit(text[i]))return {};}
   else {if(text[i]<'1'||text[i]>'9')return {};while(i<text.size()&&digit(text[i]))++i;}
   if(i<text.size()&&text[i]=='.'){++i;size_t digits=i;while(i<text.size()&&digit(text[i]))++i;if(i==digits)return {};}
   if(i<text.size()&&(text[i]=='e'||text[i]=='E')){++i;if(i<text.size()&&(text[i]=='+'||text[i]=='-'))++i;size_t digits=i;while(i<text.size()&&digit(text[i]))++i;if(i==digits)return {};}
   if(i<text.size()&&!space(text[i])&&text[i]!=','&&text[i]!=']'&&text[i]!='}')return {};
   double value=0;auto read=std::from_chars(text.data()+start,text.data()+i,value,std::chars_format::general);
   if(read.ec!=std::errc{}||read.ptr!=text.data()+i||!std::isfinite(value))return {};
   char number[64];auto written=std::to_chars(number,number+sizeof(number),value,std::chars_format::general,std::numeric_limits<double>::max_digits10);
   if(written.ec!=std::errc{})return {};auto normalized=std::string_view(number,size_t(written.ptr-number));canonical.append(normalized);
   if(text.substr(start,i-start).find_first_of(".eE")!=std::string_view::npos&&normalized.find_first_of(".eE")==std::string_view::npos)canonical+=".0";previous='v';
  }else if(c=='{'||c=='['){if(depth==int(stack.size()))return {};stack[depth++]=c;canonical+=c;++i;previous=c;}
  else if(c=='}'||c==']'){if(depth==0||previous==','||stack[depth-1]!=(c=='}'?'{':'['))return {};--depth;canonical+=c;++i;previous=c;}
  else if(c==','||c==':'){canonical+=c;++i;previous=c;}
  else {
   std::string_view literal;if(text.substr(i,4)=="true"||text.substr(i,4)=="null")literal=text.substr(i,4);else if(text.substr(i,5)=="false")literal=text.substr(i,5);else return {};
   i+=literal.size();if(i<text.size()&&!space(text[i])&&text[i]!=','&&text[i]!=']'&&text[i]!='}')return {};canonical.append(literal);previous='v';
  }
  if(canonical.size()>limit)return {};
 }
 juce::var result;if(!started||depth!=0||juce::JSON::parse(juce::String::fromUTF8(canonical.data(),int(canonical.size())),result).failed())return {};return result;
}
inline bool provider(const juce::var& value){return member(value,{"offline","openai","anthropic","openrouter","ollama","codex-cli","claude-cli"});}
inline bool reply(const juce::var& response,bool probe){
 if(!response.isObject()||response.getDynamicObject()->getProperties().size()!=5||!integer(response["protocolVersion"],1,1)||!provider(response["provider"])||!response["model"].isString()||response["model"].toString().isEmpty()||response["model"].toString().length()>100)return false;
 if(probe)return response["ok"].isBool()&&bool(response["ok"])&&response["message"].isString()&&response["message"].toString().length()<=512;
 auto ideas=response["ideas"],sources=response["sources"];if(!ideas.isArray()||ideas.size()<1||ideas.size()>8||!sources.isArray()||sources.size()!=ideas.size())return false;
 for(const auto& source:*sources.getArray())if(!member(source,{"offline","ai","offline-fallback","codex-cli","claude-cli"}))return false;return true;
}
inline bool schedule(const juce::var& idea,Schedule& s,bool strict=false){
 if(!idea.isObject()||!idea["metadata"].isObject()||!idea["layers"].isObject())return false;
 auto metadata=idea["metadata"],layers=idea["layers"];
 if(metadata.hasProperty("timeSignature")&&!meter(metadata["timeSignature"]))return false;
 if(strict&&(!meter(metadata["timeSignature"])||metadata.getDynamicObject()->getProperties().size()!=7||idea.getDynamicObject()->getProperties().size()!=2
   ||!member(metadata["key"],{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"})
   ||!member(metadata["mode"],{"minor","major","dorian","phrygian"})||!member(metadata["style"],{"trap","boom-bap","r&b","ambient","nu-metal"})
   ||!integer(metadata["bars"],4,8)||!integer(metadata["bpm"],40,240)||!integer(metadata["seed"],0,4294967295.0)||layers.getDynamicObject()->getProperties().size()!=3))return false;
 s.beats=double(metadata["bars"])*4;s.bpm=double(metadata["bpm"]);
 if((s.beats!=16&&s.beats!=32)||!std::isfinite(s.bpm)||s.bpm<40||s.bpm>240)return false;
 int channel=1;
 for(const char* name:{"bass","chords","melody"}){
  auto list=layers[name];if(!list.isArray()||list.size()>800)return false;
  for(const auto& n:*list.getArray()){
   if(!n.isObject())return false;
   if(strict&&(n.getDynamicObject()->getProperties().size()!=5||!n["layer"].isString()||n["layer"].toString()!=name))return false;
   for(const char* field:{"pitch","start","duration","velocity"})if(!n[field].isInt()&&!n[field].isInt64()&&!n[field].isDouble())return false;
   double pitch=n["pitch"],velocity=n["velocity"];
   if(!std::isfinite(pitch)||!std::isfinite(velocity)||pitch<0||pitch>127||velocity<1||velocity>127||pitch!=std::floor(pitch)||velocity!=std::floor(velocity))return false;
   if(!s.add(int(pitch),double(n["start"]),double(n["duration"]),int(velocity),channel))return false;
  }++channel;
 }
 if(!s.count)return false;s.sort();return true;
}
}
