# Offline Direction and Mood Implementation Plan

> Execute inline with superpowers:executing-plans and TDD, followed by one fresh independent whole-slice review. Task.md is the only progress checklist. Approved architecture already authorizes E03; no additional design-approval pause.

**Goal:** Resolve bounded RU/EN musical directions into typed, reviewable settings and generate supported loops whose seven mood axes affect real notes.

**Architecture:** Pure C++ Mood/Performance controls extend the existing core without changing neutral musical output. A replaceable local interpreter returns CompositionConfig plus source/span/conflict diagnostics. A narrow execution adapter honors supported requests and explicitly rejects song/extra-role/blend requests until their actual synthesis is integrated.

**Tech Stack:** C++20 standard library, existing composition-v2/CTest; no models, networking, process launches, JUCE or runtime dependencies.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, deterministic musical pipeline and manual/text precedence; task-engine.md §4.1/4.5/8. Keep existing UI/processor/bridge-v1 working.

## Global Constraints

Input<=4096UTF-8 bytes, strict Unicode scalar decoding, no embedded NUL/control data. Defaults C natural minor/nu-metal/100BPM/4/4/16bar loop,180seconds for explicit song mode, humanization20, all seven mood axes50. Caller provides a seed chosen once; interpreter has no fresh randomness. Pinned manual > explicit numeric text > adjective hints > defaults. Incompatible equal-priority mentions error unless that field is pinned manually, in which case show an overridden warning. Unknown fragments are visible warnings. Supported loop lengths4/8/16/32; requested seconds/other lengths remain exact and execution rejects unsupported modes rather than truncating. All prior resource/channel/note constraints remain.

## Review Focus

- Malformed/overlong UTF-8, NUL, negatives, fractional BPM, huge seed/numbers and unsupported meters must never silently coerce (Task2).
- Root letters must match whole musical tokens/context, including sharps/flats and RU solfège, not letters inside other words (Task2).
- Text conflicts and negated instruments must be visible; manual pins override only their chosen fields and leave other diagnostics intact (Task2).
- Every mood axis changes musical values in fixed-seed comparisons; neutral mood preserves golden7194293116935115149 and invalid mood/performance rejects atomically (Task1).
- Long duration, multiple genres/sections and unsupported instrument roles are preserved for preview but cannot masquerade as a rendered short loop (Task2).

### Task 1: Seven-axis musical mood

**Files:** create plugin/Source/engine/Mood.h and plugin/tests/MoodTest.cpp; modify Config.h, Harmony.h, Rhythm.h, Generation.h, GenerationTest.cpp and plugin/CMakeLists.txt.

**Interfaces:** Mood has darkness/energy/melancholy/tension/aggression/atmosphere/complexity (0–100, default50). deriveMood(const Mood&)->optional<Performance>; Performance holds bounded octaveOffset, velocityOffset, gatePercent, chordGatePercent, bassStride/leadStride, extraChordVoices, tensionOffset, Contour{seeded,rising,falling}. Config::performance defaults neutral. generateMoodLoop(Config,const Mood&)->GenerationResult validates both inputs, uses mood.complexity as the complexity source and delegates to generateLoop after deriving controls.

1. Write failing tests for all axes at0/100, neutral equivalence, actual register/density/velocity/gate/contour/voicing effects, source-config immutability, invalid data and joint extremes across roots/scales/styles/feels. Observe missing mood API before production code.
2. Implement integer derivation: darkness shifts octaves at25/75; energy affects stride and velocity; melancholy chooses rising/falling contour; tension controls extra voices and harmonic tension; aggression changes velocity/articulation; atmosphere lengthens chord gates and spaces lead; complexity feeds the existing rhythmic/voicing control. Preserve explicit ghost/fill switches. Apply controls to actual emitted notes with range/endpoint checks.
3. Use engineVersion3/source-v3 for the added algorithm. Replace growing decimal config IDs with an injective canonical hex encoding of validated bounded fields/seed/modulation/performance, keeping IDs within128bytes without hashes/platform-dependent serialization. Neutral note golden stays unchanged; metadata-version assertion changes explicitly.
4. Run strict tests/CMake, existing native regression and ASan/UBSan; record actual results and commit with task.md.

### Task 2: Deterministic local RU/EN interpreter and execution adapter

**Files:** create plugin/Source/engine/{Direction.h,Prompt.h,PromptCompose.h}; create plugin/tests/PromptTest.cpp; modify plugin/CMakeLists.txt and developer documentation.

**Interfaces:** CompositionConfig holds a native loop recipe, seven-axis mood, exact Duration{bars|seconds,amount}, loop/song mode, requested role set/presets, optional two-genre mix and requested section roles. Duration is authoritative; recipe.bars is set only at loop execution. Field/Origin enums, byte Span, Recognition and Diagnostic records explain resolved values. ManualOverrides{values,pinned-fields}; ResolvedConfig{config,origins,recognized,diagnostics}::valid(). IPromptInterpreter::interpret(string_view,uint32_t callerSeed,const ManualOverrides&)->ResolvedConfig; LocalPromptInterpreter implements deterministic dictionary/grammar parsing. generateResolvedLoop(const ResolvedConfig&)->GenerationResult honors exact supported duration/roles/presets/mood; rejects inconsistent/unsupported capability requests with no partial result.

1. Write failing RU/EN tests: the supplied3minute D-minor nu-metal example, mixed case/ё, accidentals, numeric tempo/meter/duration/seed/axes, two-genre normalized weights, role negations and piano preset. Cover all field defaults/origins, numeric-vs-adjective specificity, conflicts, manual pins and invalid manual data; unknown text warnings; invalid UTF-8/NUL/oversize/extreme numbers.
2. Implement strict bounded UTF-8 tokenization retaining original byte spans, ASCII/Cyrillic casefold/ё normalization and common synonyms/inflections. Parse whole tokens/longest phrases; never infer arbitrary language comprehension. Use checked from_chars/finite numeric handling. Two genres retain normalized weights; conflicting or unsupported negation is diagnostic. Require nonempty selected instruments and valid presets/mood/duration.
3. Implement supported loop execution: exact bars/meter/seed/key/style/feel, selected bass/chords/lead/drums tracks and piano program0. No unavailable role is silently dropped. Seconds/song/multiple sections/blended synthesis report capability errors until real arrangement/blend implementation. This gate does not make a valid parsed full-song request invalid for preview.
4. Add standard-only CMake/CTest targets; compile parser tests with UTF-8 source on MSVC. Run native build/CTest, Node/sample checks, sanitizers and one independent code review. Fix Critical/Important via failing-first regressions, document limits and push PR3. UI controls and v2 song scheduling/state/export remain later integration, not claimed complete here.
