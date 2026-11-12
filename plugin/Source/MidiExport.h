#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Wire.h"
inline juce::MemoryBlock encodeMidi(const juce::var& idea){
 Schedule schedule;if(!wire::schedule(idea,schedule))return {};
 const int patternEnd=int(schedule.beats*480);
 juce::MidiFile file;file.setTicksPerQuarterNote(480);juce::MidiMessageSequence tempo;
 tempo.addEvent(juce::MidiMessage::tempoMetaEvent(int(60000000.0/schedule.bpm)));tempo.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4,4));
 auto end=juce::MidiMessage::endOfTrack();end.setTimeStamp(patternEnd);tempo.addEvent(end);file.addTrack(tempo);int channel=1;
 for(const char* name:{"bass","chords","melody"}){
  juce::MidiMessageSequence track;track.addEvent(juce::MidiMessage::textMetaEvent(3,name));auto list=idea["layers"][name];
  for(const auto& n:*list.getArray()){
   int start=juce::jlimit(0,patternEnd-1,int(std::lround(double(n["start"])*480)));
   int finish=juce::jlimit(start+1,patternEnd,int(std::lround((double(n["start"])+double(n["duration"]))*480)));
   auto on=juce::MidiMessage::noteOn(channel,int(n["pitch"]),(juce::uint8)int(n["velocity"]));on.setTimeStamp(start);track.addEvent(on);
   auto off=juce::MidiMessage::noteOff(channel,int(n["pitch"]));off.setTimeStamp(finish);track.addEvent(off);
  }
  track.addEvent(end);file.addTrack(track);++channel;
 }
 juce::MemoryOutputStream bytes;if(!file.writeTo(bytes,1))return {};return bytes.getMemoryBlock();
}
