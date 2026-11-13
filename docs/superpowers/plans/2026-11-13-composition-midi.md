# Composition-v2 MIDI Export Implementation Plan

> Execute inline with superpowers:executing-plans/TDD and one fresh independent review. The approved architecture and repeated continuation authorize execution; task.md remains the only progress checklist.

**Goal:** Encode every validated composition-v2 as bounded standard MIDI type1, independently decoded by JUCE tests.

**Architecture:** A standard-library-only encoder consumes immutable Composition, validates before allocation, then writes a conductor and one track per source track. Counted same-pitch lifetimes match existing retrigger behavior. This is an E04 integration dependency; the legacy exporter/UI remain operational.

**Tech Stack:** C++20, existing composition validator, JUCE MidiFile for independent test decoding, existing CMake/CTest.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, maps/state/export and compatibility; task-engine.md MIDI export requirements.

## Global Constraints

PPQ480, type1, max16source tracks/131072notes/65536CC/4096points per map/1024sections/30minutes/960000000ticks; existing validator remains authoritative. No new dependency, network, file I/O or audio-thread work. Reject invalid documents atomically; return no partial bytes. Bounded encoded bytes32MiB. Preserve all notes/channels/presets/maps/trailing rests; do not change v1 or invoke the exporter in processBlock.

## Review Focus

- Same-pitch overlaps/repeated onsets/nested releases must not terminate a later note; boundary offs precede ons (Task1).
- Track/channels16 and shared percussion10 keep assignment; bank select precedes program/notes, and end-of-song sustain/sostenuto/hold2 release cannot latch sound (Task1).
- Meter/tempo/key changes and silent final bars retain exact ticks; non-major/minor modes carry exact text plus a documented notation key signature (Task1).
- Valid very-large ticks exceed four-byte SMF delta capacity; insert zero-payload sequencer-specific metadata to span gaps, never overflow/truncate (Task1).
- Invalid/unordered source notes or controls, maximum event counts and malformed documents must yield deterministic encoding or explicit failure without input mutation (Task1).

### Task 1: Bounded v2 MIDI encoder

**Files:** create plugin/Source/engine/Midi.h and plugin/tests/CompositionMidiTest.cpp; modify plugin/CMakeLists.txt, plugin/README.md and task.md.

**Interfaces:** MidiResult { vector<uint8_t> bytes; string error; explicit operator bool() const; }; encodeCompositionMidi(const Composition&)->MidiResult. Failure always has empty bytes/nonempty error. Success contains MThd type1 PPQ480 and conductor+source tracks. Worker-only function; no integration into legacy playback.

1. Write tests using independently hand-derived fixtures and JUCE decoding: header/track count/PPQ, source names/channels/velocities/tick endpoints, two tempos, changing3/4→5/8 meter, Cmajor→Ddorian labels, sections, CC/presets and valid max tick gaps. Decode actual generated mood/prompt/drum output too. Expected: new API missing when compiled, before production code.
2. Implement validation and bounded binary writing with explicit status bytes. Stable sort source note edges by tick/off-before-on, suppress nonfinal lifetime offs; retrigger held keys with off/on. Emit initial bank0/32 then program, actual controls before ons, source names and EOT at exact end on every track. Reset touched CC64/66/69 at end. Standard signatures: major/minor (harmonic/melodic use minor base), diatonic modes use relative-major accidental set, pentatonic/blues use tonic major/minor base. Exact tonic/scale text at every key point discloses the distinction. Tempo/meter/section markers go in conductor. Split excessive deltas using FF7F00 padding. Expected: decoded fixtures preserve all stated semantics.
3. Exercise131072 simultaneous same-pitch lifetimes (32bit count), all16channels, shared disjoint percussion, unsorted source data, invalid resource/map/identity input and deterministic source immutability. Run full Linux VST3/Standalone build/CTest, Node/sample, strict standard-only smoke and ASan/UBSan. Expected: all tests pass; old sample bytes unchanged.
4. Document developer API and limits; record results in task.md and commit. Run task-done with full CTest, then one independent review. Critical/Important fixes require reproducing RED→GREEN; record deferred limits without claiming UI/state/arrangement/playback complete. Push existing draft PR3 without merge/release.
