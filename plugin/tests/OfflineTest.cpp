#include "../Source/Processor.h"
#include <iostream>
#define check(v) do { if(!(v)){std::cerr<<"check at "<<__LINE__<<"\n";std::abort();} }while(false)
void wait(Processor& p){for(int i=0;i<100&&p.busy;++i)juce::Thread::sleep(10);check(!p.busy);}
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI init;Processor p;p.prepareToPlay(48000,64);
 const juce::String body="{\"provider\":\"offline\",\"seed\":42,\"key\":\"D\",\"mode\":\"minor\",\"style\":\"ambient\",\"bars\":8,\"bpm\":87}";
 p.request(body);wait(p);std::cerr<<p.getStatus()<<'\n';check(p.ideaCount()==8);
 juce::StringArray originals;const int scale[]={0,2,3,5,7,8,10};
 for(int i=0;i<8;++i){p.choose(i);auto idea=p.getIdea();check(int(idea["metadata"]["bars"])==8&&int(idea["metadata"]["bpm"])==87);check(idea["metadata"]["key"].toString()=="D");
  auto json=juce::JSON::toString(idea);check(!originals.contains(json));originals.add(json);
  for(const char* layer:{"bass","chords","melody"}){auto notes=idea["layers"][layer];check(notes.size()>0);for(const auto& n:*notes.getArray()){
   check(double(n["start"])>=0&&double(n["duration"])>0&&double(n["start"])+double(n["duration"])<=32);bool inScale=false;for(int interval:scale)inScale|=(int(n["pitch"])-2+120)%12==interval;check(inScale);
  }}
 }
 p.request(body);wait(p);for(int i=0;i<8;++i){p.choose(i);check(juce::JSON::toString(p.getIdea())==originals[i]);}
 p.request("{\"provider\":\"offline\",\"seed\":43}");wait(p);check(juce::JSON::toString(p.getIdea())!=originals[0]);
 for(const char* mode:{"minor","major","dorian","phrygian"})for(const char* style:{"trap","boom-bap","r&b","ambient","nu-metal"}){
  auto params=juce::JSON::parse(body);params.getDynamicObject()->setProperty("mode",mode);params.getDynamicObject()->setProperty("style",style);
  auto generated=offline::generate(params);check(generated.size()==8);for(const auto& idea:*generated.getArray()){auto* state=new juce::DynamicObject;state->setProperty("version",1);state->setProperty("idea",idea);auto text=juce::JSON::toString(juce::var(state));p.setStateInformation(text.toRawUTF8(),int(text.getNumBytesAsUTF8()));check(juce::JSON::toString(p.getIdea())==juce::JSON::toString(idea));}
 }
 if(argc>1){auto generated=offline::generate(juce::JSON::parse(body));check(juce::File(argv[1]).replaceWithText(juce::JSON::toString(generated)));}
 std::cout<<"eight native offline ideas, deterministic seed, bounds and scale passed\n";
}
