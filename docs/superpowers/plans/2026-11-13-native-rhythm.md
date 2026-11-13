# Native Drum Groove and Time Feel Implementation Plan

> Execute inline with superpowers:executing-plans and test-driven-development, followed by one independent whole-slice review. Task.md owns all progress; numbered steps are instructions, not a competing checklist. This continues the approved E02 architecture.

**Goal:** Realize deterministic GM drums and explicit half/normal/double-time across the v2 loop's rhythm, retaining tempo, length and the current default melodic golden.

**Architecture:** A standard-C++ Rhythm.h shares bounded timing/feel transforms between bass, lead and drums. Config selects Feel and optional drums/fills/ghosts. Generation.h appends a separately seeded channel10 track with globally unique IDs; existing legacy processor/UI paths stay untouched.

**Tech Stack:** C++20, existing v2 model/validator/CTest; no dependencies, networking, inference or audio-callback changes.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, deterministic groove stage and channel10/resource constraints; supplied task-engine.md §4.4/4.7.

## Global Constraints

PPQ480; all existing Config bounds and v2 limits. GM drums on JUCE channel10, fixed kit/no bank/program/CC, pitch ownership35–59 leaving60–81 available for later percussion. Defaults: Feel::normal, drums=false (preserves existing three-track callers), ghostNotes=true, drumFills=true (only meaningful when drums enabled). New generator engineVersion2/source-v2 explicitly identifies the added algorithm version; model schema remains2. No claim of full-song/UI/export acceptance.

## Review Focus

- Extreme1/16 and12/2 meters with maximum swing/humanization must keep every onset/duration in the bar/document and avoid duplicate same-pitch onsets (both tasks).
- Repeated drum hits need bounded note lengths ending before the next same-pitch attack (Task1).
- Half-time must stretch a complete rhythm over two bars, including odd divisions; double-time repeats it twice per bar without altering tempo/length (Task2).
- Adding or toggling drums must not perturb existing melodic notes or their IDs (Task1).
- Fill/ghost switches must change actual output; seed, style and feel must influence reproducible rhythms, with invalid feel/config rejected atomically (both tasks).

### Task 1: GM drum grooves and loop integration

**Files:** create plugin/Source/engine/Rhythm.h and plugin/tests/RhythmTest.cpp; modify Config.h, Generation.h, GenerationTest.cpp and plugin/CMakeLists.txt.

**Interfaces:** DrumResult{optional<Track> track,string error}; generateDrums(const Config&)->DrumResult. Output is an independently valid channel10 GM Track; caller reassigns globally unique IDs when appending it to a composition. One style profile per existing Style, phrase-end fills every4thbar, optional low-velocity snare ghost notes, bounded jitter/swing, sorted deterministic notes and canonicalized same-pitch collisions. Move the existing onset helper unchanged into Rhythm.h for shared timing behavior.

1. Write failing tests for GM channel/range, deterministic repeat/changed seed, distinct five-genre signatures, active fills/ghost toggles, source melodic invariance after enabling drums, extreme meters and same-pitch lifetime ordering. Observe missing API compilation failure before implementation.
2. Add config flags, drum generator and actual generateLoop integration. Canonicalize duplicate same-pitch/start hits by strongest velocity, then shorten any overlap before the next same-pitch onset. Use a dedicated role seed so melodic PRNG streams/IDs stay unchanged.
3. Compile strict standard-only tests, add CTest rhythm target, run existing golden/regressions plus ASan/UBSan. Record tests/results and commit the working slice with task.md. Preserve old generator/protocol/state source.

### Task 2: Shared half-time/double-time behavior

**Files:** modify Config.h, Rhythm.h, Generation.h, RhythmTest.cpp, GenerationTest.cpp.

**Interfaces:** Feel{half,normal,doubleTime}, Config::feel; rhythmGrid(const vector<int>& units,int divisions,Feel,int bar)->Grid with pulses carrying transformed unit, sourceIndex and cycle. Half maps source positions into a two-bar phrase with exact integer numerators (supports odd divisions); double maps two copies to doubled divisions. Validate Feel before arithmetic. Bass/lead/drums all use the same grid transform and shared onset rule.

1. Write failing tests for expected4/4 snare positions: normal480/1440ticks, half960ticks, double240/720/1200/1680ticks; odd-division half-time across a bar pair; differing bass/lead counts with unchanged BPM/endTick; normal golden7194293116935115149 unchanged; invalid Feel rejection.
2. Implement shared grid, preserving source motif indexes/call-response phase and beat-end clipping. Scale fill density with feel. Keep transformation bounded by at most2x the small fixed profile sizes.
3. Run full native build/CTest, Node tests/sample comparisons and standard-library sanitizer tests. Request fresh independent whole-slice code review, regrade by user effect, fix Critical/Important once via RED→GREEN, retain explicit limits/minors in task.md. Push to existing PR3, no merge/release.

## Scope boundary

This completes additional groove capabilities and a first drum-role realization, not full arrangement or new UI/playback. Channel10 preview and bounded sixteen-channel song scheduling remain E04 integration. Later prompt/mood controls must preserve these typed config choices. Listening/real DAW acceptance and Windows golden verification require actual evidence.
