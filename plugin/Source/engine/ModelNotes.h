#pragma once

#include "Direction.h"
#include "Validation.h"
#include <span>

namespace composer::model {
inline constexpr int textVocab = 128256, longestTime = 10000, durationOffset = 10000;
inline constexpr int instrumentOffset = 11000, instruments = 129, midiBos = textVocab+55026;

struct Result {
 std::optional<Composition> composition;
 std::string error;
};

// AMT event triples use hundredths of a second, then a GM program and pitch.
// Decode the documented wire format directly into the 480 PPQ domain.
inline Result decode(std::span<const int> stream, const ResolvedConfig& resolved) {
 const auto& request = resolved.config;
 const auto& config = request.recipe;
 if (!resolved.valid() || !validateConfig(config) || request.mode!=RequestMode::loop ||
     request.duration.unit!=DurationUnit::bars ||
     (request.duration.amount!=4 && request.duration.amount!=8 && request.duration.amount!=16))
  return {{}, "MIDI-LLM supports 4, 8 or 16 bar loops"};
 if (stream.empty() || stream.size()>2046 || stream.size()%3!=0)
  return {{}, "MIDI-LLM returned incomplete notes"};

 Composition song;
 song.id = "midi-llm-" + std::to_string(config.seed);
 song.source = "midi-llm-llama-3.2-1b";
 song.style = styleName(config.style);
 song.seed = config.seed;
 song.endTick = {int64_t(request.duration.amount)*480*4*config.numerator/config.denominator};
 song.tempos = {{{0},int(std::llround(60000000.0/config.bpm))}};
 song.meters = {{{0},config.numerator,config.denominator}};
 song.keys = {{{0},config.root,config.scale}};
 song.timingNotice = "Generated onsets map to the requested tempo. Notes end at the requested loop boundary.";

 std::array<int, instruments> byProgram;
 byProgram.fill(-1);
 int melodicChannel = 1;
 Id id = 1;
 for (size_t index=0; index<stream.size(); index+=3) {
  const int64_t start = int64_t(stream[index])-textVocab;
  const int64_t duration = int64_t(stream[index+1])-textVocab-durationOffset;
  const int64_t sound = int64_t(stream[index+2])-textVocab-instrumentOffset;
  if (start<0 || start>=longestTime || duration<0 || duration>=1000 || sound<0 || sound>=instruments*128)
   return {{}, "MIDI-LLM returned an invalid note token"};
  const double ticksPerUnit = config.bpm*480.0/6000.0;
  const auto onset = int64_t(std::llround(start*ticksPerUnit));
  if (onset>=song.endTick.value) continue;
  const auto length = std::clamp(int64_t(std::llround(duration*ticksPerUnit)),int64_t(1),song.endTick.value-onset);
  const int program = int(sound/128);
  const bool drums = program==128;
  auto& trackIndex = byProgram[size_t(program)];
  if (trackIndex<0) {
   if (song.tracks.size()>=limits::tracks || (!drums && melodicChannel>16))
    return {{}, "MIDI-LLM exceeds the MIDI channel limit"};
   Track track;
   track.id = id++;
   track.channel = drums ? 10 : melodicChannel++;
   if (melodicChannel==10) ++melodicChannel;
   if (drums) {
    track.name = "Drums";
    track.role = Role::drums;
   } else {
    track.program = program;
    track.role = program>=32 && program<=39 ? Role::bass : program<=7 ? Role::chords : Role::lead;
    track.name = program<=7 ? "Keys" : track.role==Role::bass ? "Bass" : "Instrument "+std::to_string(program+1);
   }
   trackIndex = int(song.tracks.size());
   song.tracks.push_back(std::move(track));
  }
  song.tracks[size_t(trackIndex)].notes.push_back({id++,int(sound%128),96,{onset},{length}});
 }
 for (auto& track : song.tracks)
  std::stable_sort(track.notes.begin(), track.notes.end(), [](const Note& a, const Note& b){return a.start<b.start;});
 const auto valid = validate(song);
 if (!valid) return {{}, valid.message};
 return {std::move(song), {}};
}
}
