#pragma once
#include <juce_core/juce_core.h>
#include "Schedule.h"
namespace wire {
inline bool member(const juce::var& value,std::initializer_list<const char*> allowed){if(!value.isString())return false;for(auto name:allowed)if(value.toString()==name)return true;return false;}
inline bool integer(const juce::var& value,double lo,double hi){return (value.isInt()||value.isInt64()||value.isDouble())&&std::isfinite(double(value))&&std::floor(double(value))==double(value)&&double(value)>=lo&&double(value)<=hi;}
inline bool meter(const juce::var& value){return value.isObject()&&value.getDynamicObject()->getProperties().size()==2&&integer(value["numerator"],4,4)&&integer(value["denominator"],4,4);}
// JUCE's parser accepts an object prefix; check complete framing before parsing it.
inline juce::var document(const juce::String& text){
 std::array<juce::juce_wchar,32> stack{};int depth=0;bool quoted=false,escaped=false,started=false;juce::juce_wchar previous=0;
 for(auto c:text){
  if(quoted){if(escaped)escaped=false;else if(c=='\\')escaped=true;else if(c=='"')quoted=false;else if(c<32)return {};continue;}
  bool space=c==' '||c=='\t'||c=='\r'||c=='\n';if(space)continue;
  if(started&&depth==0)return {};
  if(!started){if(c!='{')return {};started=true;}
  if(c=='"')quoted=true;
  else if(c=='{'||c=='['){if(depth==int(stack.size()))return {};stack[depth++]=c;}
  else if(c=='}'||c==']'){if(depth==0||previous==','||stack[depth-1]!=(c=='}'?'{':'['))return {};--depth;}
  previous=c;
 }
 juce::var result;if(!started||quoted||depth!=0||juce::JSON::parse(text,result).failed())return {};return result;
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
