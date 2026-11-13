#pragma once
#include <bitset>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace composer {
struct Tick { int64_t value=0; auto operator<=>(const Tick&) const = default; };
struct Seconds { double value=0; };
using Id=uint64_t;
enum class Scale { major, naturalMinor, harmonicMinor, melodicMinor, dorian, phrygian,
                   lydian, mixolydian, majorPentatonic, minorPentatonic, blues };
enum class Role { bass, chords, lead, drums, percussion, counterMelody, arpeggio, pads, harmony };
enum class SectionRole { intro, verse, preChorus, chorus, bridge, breakdown, solo, outro };
struct Note { Id id=0; int pitch=60,velocity=90; Tick start,duration; };
struct Control { Id id=0; Tick tick; int controller=0,value=0; };
struct Bank { int msb=0,lsb=0; };
struct Track {
    Id id=0; std::string name; Role role=Role::bass; int channel=1,lowPitch=0,highPitch=127;
    std::optional<int> program; std::optional<Bank> bank;
    std::bitset<128> ownedPitches;
    std::vector<Note> notes; std::vector<Control> controls;
};
struct Tempo { Tick tick; int microsPerQuarter=500000; };
struct Meter { Tick tick; int numerator=4,denominator=4; };
struct Key { Tick tick; int root=0; Scale scale=Scale::naturalMinor; };
struct Section { Id id=0; std::string name; Tick start,end; SectionRole role=SectionRole::verse; };
struct Chord {
    Id id=0; Tick start,duration; int root=0;
    std::vector<int> pitchClasses,voicing;
    int inversion=0,tension=50; std::string explanation;
};
struct Composition {
    int schemaVersion=2,ppq=480; uint32_t engineVersion=1,seed=0;
    std::string id,style,source,legacyJson,timingNotice;
    Tick endTick;
    std::vector<Tempo> tempos; std::vector<Meter> meters; std::vector<Key> keys;
    std::vector<Section> sections; std::vector<Chord> chords; std::vector<Track> tracks;
};
namespace limits {
inline constexpr size_t tracks=16,notes=131072,controls=65536,mapPoints=4096,sections=1024,chords=16384;
inline constexpr size_t documentBytes=64*1024*1024,legacyBytes=4*1024*1024;
inline constexpr int64_t maxTick=2000000LL*480;
inline constexpr double seconds=1800;
}
}
