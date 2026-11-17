#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <stdexcept>
namespace offline {
inline juce::var generate(const juce::var& params){
 const juce::StringArray allowed{"provider","key","mode","style","bpm","bars","model","count","seed","description","action","timeSignature"};
 if(auto* object=params.getDynamicObject()){for(const auto& name:object->getProperties())if(!allowed.contains(name.name.toString()))throw std::runtime_error("Unsupported generation fields; specify loop bars explicitly");}
 else throw std::runtime_error("Invalid generation request");
 const juce::StringArray keys{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},modes{"minor","major","dorian","phrygian"},styles{"trap","boom-bap","r&b","ambient","nu-metal"};
 int root=keys.indexOf(params["key"].toString());if(root<0)root=0;
 int mode=modes.indexOf(params["mode"].toString());if(mode<0)mode=0;
 int style=styles.indexOf(params["style"].toString());if(style<0)style=0;
 const int scales[][7]={{0,2,3,5,7,8,10},{0,2,4,5,7,9,11},{0,2,3,5,7,9,10},{0,1,3,5,7,8,10}};
 const int progressions[][4]={{0,5,2,6},{0,3,4,0},{0,5,3,4},{0,3,5,4},{0,6,5,4}};
 const double densities[]={.66,.48,.60,.30,.60},syncs[]={.62,.35,.55,.18,.70};
 const double requestedBars=params.hasProperty("bars")?double(params["bars"]):4;
 if(requestedBars!=4&&requestedBars!=8)throw std::runtime_error("Legacy loop bars must be4or8; use Composer for16");
 const int bars=int(requestedBars);
 double requestedBpm=double(params["bpm"]);int bpm=std::isfinite(requestedBpm)&&requestedBpm!=0?juce::jlimit(40,240,int(std::round(juce::jlimit(40.0,240.0,requestedBpm)))):100;
 double requestedSeed=double(params["seed"]);
 auto base=params.hasProperty("seed")&&std::isfinite(requestedSeed)&&std::floor(requestedSeed)==requestedSeed?uint32_t(juce::int64(std::fmod(requestedSeed,4294967296.0))):uint32_t(juce::Random::getSystemRandom().nextInt());
 double requestedCount=double(params["count"]);
 int count=params.hasProperty("count")&&std::isfinite(requestedCount)?int(std::round(juce::jlimit(1.0,8.0,requestedCount))):8;
 auto pitch=[&](int degree,int octave){int oct=int(std::floor(degree/7.0)),idx=(degree%7+7)%7;return juce::jlimit(0,127,(octave+1)*12+root+scales[mode][idx]+oct*12);};
 juce::Array<juce::var> ideas;
 for(int variation=0;variation<count;++variation){
  uint32_t seed=base+uint32_t(variation+1)*2654435761u,state=seed?seed:1;
  auto random=[&](){state^=state<<13;state^=state>>17;state^=state<<5;return double(state)/4294967296.0;};
  juce::Array<juce::var> bass,chords,melody;
  auto note=[bars](juce::Array<juce::var>& list,int p,double start,double duration,int velocity,const char* layer){
   auto* n=new juce::DynamicObject;n->setProperty("pitch",p);n->setProperty("start",std::round(start*1000)/1000);n->setProperty("duration",juce::jmin(std::round(juce::jlimit(.0625,32.0,duration)*1000),bars*4000.0-std::round(start*1000))/1000);n->setProperty("velocity",juce::jlimit(1,127,velocity));n->setProperty("layer",layer);list.add(juce::var(n));
  };
  int progression[4];for(int i=0;i<4;++i)progression[i]=(progressions[style][i]+(random()<.15?1:0))%7;
  for(int bar=0;bar<bars;++bar){
   int degree=progression[bar%4];double barStart=bar*4;
   for(int i=0;i<(style==2||style==3?4:3);++i){double start=barStart+(random()<.2?.25:0);int velocity=66+int(random()*24);note(chords,pitch(degree+i*2,3),start,(style==3?3.8:style==4?2:3.5)-i*.06,velocity,"chords");}
   const double ambient[]={0,2},metal[]={0,.5,1,2,2.5,3},standard[]={0,1.5,2.5,3.5};const double* rhythm=style==3?ambient:style==4?metal:standard;int size=style==3?2:style==4?6:4;
   for(int i=0;i<size;++i){if(i>0&&random()<.19)continue;double next=i+1<size?rhythm[i+1]:4;bool passing=i==size-1&&random()<.35;int p=pitch(passing?(degree+4)%7:degree,2);int velocity=78+int(random()*35);note(bass,p,barStart+rhythm[i],juce::jmin(next-rhythm[i],1.1)*.84,velocity,"bass");}
   int previous=degree+7;const int offsets[]={-2,-1,0,0,0,1,2,4};
   for(int step=0;step<8;++step){if(random()>densities[style])continue;double start=barStart+step*.5+(random()<syncs[style]?.125:0);int candidate=(random()<.7?degree:progression[(bar+1)%4])+7;candidate+=offsets[int(random()*8)];if(std::abs(candidate-previous)>4)candidate=previous+(candidate>previous?3:-3);previous=candidate;double length=random()<.2?1:random()<.4?.5:.25;int velocity=64+int(random()*46);note(melody,pitch(candidate,style==4?3:4),start,juce::jmin(length,barStart+4-start),velocity,"melody");}
  }
  auto* metadata=new juce::DynamicObject;metadata->setProperty("key",keys[root]);metadata->setProperty("mode",modes[mode]);metadata->setProperty("style",styles[style]);metadata->setProperty("bars",bars);metadata->setProperty("bpm",bpm);metadata->setProperty("seed",juce::int64(seed));
  auto* layers=new juce::DynamicObject;layers->setProperty("bass",bass);layers->setProperty("chords",chords);layers->setProperty("melody",melody);
  auto* idea=new juce::DynamicObject;idea->setProperty("metadata",juce::var(metadata));idea->setProperty("layers",juce::var(layers));ideas.add(juce::var(idea));
 }
 return ideas;
}
}
