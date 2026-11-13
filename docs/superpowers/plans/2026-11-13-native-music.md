# Native Musical Intelligence Implementation Plan

> Required execution: superpowers:executing-plans inline, TDD before code, independent whole-slice review. Task.md is the sole progress ledger; numbered steps are execution instructions. Approved architecture already authorizes this E02 work.

**Goal:** Generate real coherent composition-v2 loops entirely in standard C++ using shared harmony, motif development and distinct genre rhythms.

**Architecture:** Theory functions supply scale/chord/voice-leading rules. A shared harmony planner drives every melodic role; a deterministic motif and rhythm realization produces validated native documents. Worker/UI/full-song integration follows in E03/E04; existing offline generator stays working.

**Tech Stack:** C++20 standard library, existing composition-v2/CTest. No model, network, Node or new dependency in the engine.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, E02 theory/harmony/motif/rhythm requirements.

## Global Constraints

PPQ480, all12roots and all11 declared Scale variants; natural-minor descending melodic-minor policy explicit. Valid MIDI0–127, bounded instrument ranges, stable deterministic uint32 seed/version and IDs. Use existing validate()/resource limits, no callbacks changed. Manual artistic quality is not established by invariants. No release/merge or old task-status changes.

## Review Focus

- Negative diatonic degrees, high roots and extreme octave/degree arguments must never overflow or silently clamp pitches (Task1).
- Seventh/extensions on pentatonic/blues must avoid duplicate pitch classes; impossible register/voicing requests must fail (Task1).
- Borrowed chords/modulations must appear explicitly in key/chord metadata and shared bass/melody context (Task2).
- Same seed/config must produce byte-equivalent musical values across platforms, using integer PRNG/substreams rather than std::hash/distributions (Task2).
- Humanization, swing and6/8 timing must stay within bar/document boundaries and produce distinct genre patterns rather than changing tempo alone (Task2).

### Task 1: Scale, chord and voice-leading theory

**Files:** create plugin/Source/engine/Theory.h; create plugin/tests/TheoryTest.cpp; modify plugin/CMakeLists.txt.

**Interfaces:** scaleIntervals(Scale,bool descending=false)->vector<int>; degreePitch(const Key&,int degree,int octave,bool descending=false)->optional<int>; chordPitches(const Key&,int degree,int voices,int octave)->optional<vector<int>> (stack thirds;3–7voices; omit duplicate pitch classes); voiceLead(const vector<int>& pitchClasses,const vector<int>& previous,int low,int high)->optional<vector<int>>. Reject unknown keys, octave outside-1..9, degree beyond±1024 and impossible voicings. Ascending melodic minor uses raised6/7; descending explicitly uses natural minor.

1. Write failing C++ tests for every root/scale, C-major negative degree, harmonic/melodic-minor distinctions, triads/sevenths/extensions, pentatonic deduplication, root-crossing B voicing, bounded voice-leading motion and invalid inputs. Compile to observe missing Theory.h.
2. Implement exact interval tables, checked diatonic pitch arithmetic, third-stacking and minimum-displacement closed-position inversion selection. Ties resolve lexicographically for deterministic output.
3. Compile/run with C++20 strict warnings; add CMake/CTest theory_test. Run ASan/UBSan and existing composition tests. Record results and commit with task.md.

### Task 2: Shared harmony, motif and genre loop realization

**Files:** create plugin/Source/engine/{Config.h,Harmony.h,Generation.h}; create plugin/tests/GenerationTest.cpp; modify plugin/CMakeLists.txt.

**Interfaces:** Config holds root/scale/style(enum5legacy genres)/bpm/bars/meter/seed/complexity/humanization/swing, optional controlled modulation at a4bar boundary, explicit modalInterchange flag and harmonicBars0(auto)/1/2/4. validateConfig returns ValidationResult. planHarmony(const Config&)->HarmonyPlan{chords,keys,degrees,error}; generateLoop(const Config&)->GenerationResult{optional<Composition>,error}. Config range: BPM40–240, bars4/8/16/32, numerator1–12, denominator2/4/8/16, axes0–100. Musical roles initially bass/chords/lead; multitrack drum/arrangement realization remains E04. Limits/resources remain those of composition-v2.

1. Write tests before APIs: deterministic repeat/different seed; all genres/roots/scales valid; shared chord/bass/strong-tone melody context; repeated phrase contours; distinct fixed-seed rhythm signatures; bounded humanization and6/8; modal-interchange explanation and explicit key-map modulation; invalid config rejects without partial document. Include long180second-compatible loop and invalid extreme arguments.
2. Compile to observe missing generation API. Implement fixed style progressions/grooves, shared per-bar chords with minimum-motion voicings, deterministic two-bar motif and phrase/call-response variations. Derive role/bar PRNG seeds via fixed integer mixing; integer-only timing/velocity perturbations. At modulation transpose key context explicitly; borrowed harmony is labeled. Realization reads the planned pitch classes for bass/lead, not an independent scale-only random walk. Set IDs deterministically and validate before returning.
3. Add generation CMake/CTest target. Verify actual generated notes/invariants across the full root/scale/style matrix, regression native suite, standard-only sanitizers and independent source review. Run Node regression/sample checks before commit; preserve old generator/wire/processor behavior.
4. Record exact progress/remaining limits in task.md, push verified small commits to PR3 and update its description. E02 remains open if any required musical feature is still unimplemented/unverified; never equate this loop slice with a finished full-song Composer UI.
