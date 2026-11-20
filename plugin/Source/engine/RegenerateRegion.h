#pragma once

#include "Generation.h"
#include "LoopEditing.h"
#include <charconv>
#include <string_view>

namespace composer {
namespace regionDetail {
inline std::optional<Config> committedConfig(const std::string& id){
 constexpr std::string_view prefix="loop-v5-";
 if(!id.starts_with(prefix))return {};
 const auto end=id.find('-',prefix.size());
 const auto hex=std::string_view(id).substr(prefix.size(),end==std::string::npos?end:end-prefix.size());
 if(hex.size()!=60&&hex.size()!=66)return {};
 std::array<int,33> bytes{};
 for(size_t i=0;i<hex.size()/2;++i){
  const auto* first=hex.data()+2*i;
  auto parsed=std::from_chars(first,first+2,bytes[i],16);
  if(parsed.ec!=std::errc{}||parsed.ptr!=first+2)return {};
 }
 Config c;
 c.root=bytes[0];c.scale=Scale(bytes[1]);c.style=Style(bytes[2]);c.bpm=bytes[3];c.bars=bytes[4];
 c.numerator=bytes[5];c.denominator=bytes[6];c.complexity=bytes[7];c.humanization=bytes[8];c.swing=bytes[9];
 c.harmonicBars=bytes[10];c.modalInterchange=bytes[11]!=0;c.drums=bytes[12]!=0;
 c.ghostNotes=bytes[13]!=0;c.drumFills=bytes[14]!=0;c.feel=Feel(bytes[15]);
 for(unsigned i=0;i<4;++i)c.seed|=uint32_t(bytes[16+i])<<(i*8);
 c.performance.octaveOffset=bytes[20]-1;c.performance.velocityOffset=bytes[21]-32;
 c.performance.gatePercent=bytes[22];c.performance.chordGatePercent=bytes[23];
 c.performance.bassStride=bytes[24];c.performance.leadStride=bytes[25];
 c.performance.extraChordVoices=bytes[26]-1;c.performance.tensionOffset=bytes[27]-50;
 c.performance.contour=Contour(bytes[28]);
 if((bytes[29]!=0)!=(hex.size()==66))return {};
 if(bytes[29])c.modulation=Modulation{bytes[30],bytes[31],Scale(bytes[32])};
 if(!validateConfig(c)||!id.starts_with(musicDetail::configId(c)))return {};
 return c;
 }
}
// Keep committed harmony and timing. Mix a variation seed with the edit
// revision so each request uses the same deterministic offline note generator.
inline LoopEditResult regenerateRegion(const Composition& source,Id trackId,int firstBar,int lastBar,uint32_t variation) {
 const auto valid=validate(source);
 if(!valid)return {{},valid.message};
 const auto* target=loopEditDetail::track(source,trackId);
 if(!target||source.source!="native-motif-rules-v5"||source.meters.size()!=1||source.chords.empty()||source.keys.empty())
  return {{},"Region regeneration requires a native loop with committed harmony and a fixed meter"};
 auto committed=regionDetail::committedConfig(source.id);
 if(!committed)return {{},"The native loop recipe could not be recovered"};
 Config config=*committed;
 if(config.numerator!=source.meters.front().numerator||config.denominator!=source.meters.front().denominator)
  return {{},"Committed meter differs from the native loop recipe"};
 const int64_t barTicks=480LL*4*config.numerator/config.denominator;
 if(source.endTick.value!=config.bars*barTicks||firstBar<1||lastBar<firstBar||lastBar>config.bars)
  return {{},"Select a valid range of bars"};
 const Tick start{(firstBar-1)*barTicks},end{lastBar*barTicks};
 config.seed=musicDetail::mix(config.seed^variation);
 for(const auto& note:target->notes)if(note.start>=start&&note.start<end)
  config.seed=musicDetail::mix(config.seed^uint32_t(note.id)^uint32_t(note.id>>32));
 if(target->role==Role::chords){
  std::vector<Note> replacement;
  musicDetail::Random random(config.seed);
  for(const auto& chord:source.chords){
   const int64_t left=std::max(start.value,chord.start.value);
   const int64_t right=std::min(end.value,chord.start.value+chord.duration.value);
   if(left>=right)continue;
   const int velocity=std::clamp(72+config.performance.velocityOffset+random.offset(16),1,127);
   const int styleGate=config.style==Style::ambient?98:config.style==Style::nuMetal?55:88;
   const int gate=std::clamp(styleGate*config.performance.gatePercent*config.performance.chordGatePercent/10000+random.offset(12),1,100);
   for(int pitch:chord.voicing){
    if(pitch<target->lowPitch||pitch>target->highPitch)continue;
    int64_t onset=left;
    for(const auto& previous:target->notes)
     if(previous.pitch==pitch&&previous.start<start)
      onset=std::max(onset,previous.start.value+previous.duration.value);
    if(onset<right)
     replacement.push_back({0,pitch,velocity,{onset},{std::max<int64_t>(1,(right-onset)*gate/100)}});
   }
  }
  if(replacement.empty())return {{},"No compatible chord attacks fit inside these bars"};
  return replaceLoopRange(source,trackId,start,end,replacement);
 }

 HarmonyPlan harmony;
 harmony.keys=source.keys;
 harmony.chords=source.chords;
 if(harmony.chords.front().start.value!=0)return {{},"Committed harmony has no starting chord"};
 for(int bar=0;bar<config.bars;++bar){
  int64_t start=bar*barTicks;
  const auto& chord=musicDetail::at(harmony.chords,start);
  auto key=source.keys.front();
  for(const auto& k:source.keys){if(k.tick.value>start)break;key=k;}
  if(chord.explanation.starts_with("Modal interchange"))
   key.scale=key.scale==Scale::major?Scale::naturalMinor:Scale::major;
  auto intervals=scaleIntervals(key.scale);
  auto degree=std::find(intervals.begin(),intervals.end(),(chord.root-key.root+12)%12);
  if(degree==intervals.end())return {{},"Committed harmony is outside the supported key context"};
  harmony.contexts.push_back(key);
  harmony.degrees.push_back(int(degree-intervals.begin()));
 }
 auto generated=generateLoop(config,&harmony);
 if(!generated.composition)return {{},generated.error};
 std::vector<Note> replacement;
 for(const auto& track:generated.composition->tracks){
  if(track.role!=target->role)continue;
  for(auto note:track.notes){
   if(note.start<start||note.start>=end)continue;
   if(target->role!=Role::drums)
    note.pitch=musicDetail::registerPitch(note.pitch,note.pitch,target->lowPitch,target->highPitch);
   if(note.pitch<target->lowPitch||note.pitch>target->highPitch)continue;
   if(target->ownedPitches.any()&&!target->ownedPitches.test(size_t(note.pitch)))continue;
   note.duration.value=std::min(note.duration.value,end.value-note.start.value);
   replacement.push_back(note);
  }
 }
 if(replacement.empty())return {{},"No compatible notes could be generated for this track"};
 return replaceLoopRange(source,trackId,start,end,replacement);
}
}
