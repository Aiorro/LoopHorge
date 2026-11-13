# Native Composition-v2 Foundation Implementation Plan

> For agentic workers: use superpowers:executing-plans inline with test-driven-development. Progress is recorded exclusively in task.md; numbered steps below describe execution, not a second checklist. Architecture/execution approved2026-11-13.

**Goal:** Provide real validated native composition-v2 values, tempo conversion and explicit loss-disclosing legacy import, without changing active legacy playback/state.

**Architecture:** Pure C++20 model/validator/time helpers; JUCE is confined to the legacy adapter. Validation returns structured errors and rejects unsupported versions, malformed timelines, unsafe channel/event/density/resource combinations atomically. Import returns an optional validated document and retains the original legacy JSON.

**Tech Stack:** C++20 standard library; existing pinned JUCE only for adapter/test fixture parsing; existing CMake/CTest, no dependencies/downloads.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, approved separate native-v2 approach.

## Global Constraints

PPQ480; max16 tracks,131072 notes,65536 CC,4096 points/map,1024sections,16384chords,30minutes,2000000quarter beats,256 simultaneous channel/pitch keys,64MiB document/prepared budget. Note pitches0–127, velocities1–127, JUCE channels1–16; shared10 fixed GM/disjoint pitch ownership/no bank/program/CC. Non-bank controllers<=16/channel; CC120–127 excluded. No audio callback changes, bridge-v1 changes or automatic legacy conversion. No new runtime/model dependencies. Follow AGENTS.md and keep T12–T15 unchanged.

## Review Focus

- Positive subtick legacy notes and notes at pattern end: preserve one tick and report quantization; no invalid endpoint or source mutation (Task2).
- Signed tick overflow/extreme vector capacity: reject safely before arithmetic/index allocations (Task1).
- Repeated same pitch and same-tick releases: count lifetimes without incorrectly exhausting the256 distinct-key budget (Task1).
- Meter/tempo changes at nonzero ticks: map integration and bar-boundary validation must use the preceding segment's meter (Task1).
- Optional/malformed legacy metadata: missing seed is explicit provenance/default; boolean/fractional/out-of-range metadata and saved-state version fail, not silently coerce (Task2).

### Task 1: Native model, validation and time

**Files:** create plugin/Source/engine/{Composition.h,Validation.h,Time.h}; create plugin/tests/CompositionTest.cpp; modify plugin/CMakeLists.txt.

**Interfaces:** namespace composer; Tick{int64_t value}, Seconds{double value}; Composition carries version/id/engineVersion/seed/endTick, tempo/meter/key maps, sections/chords/tracks/provenance. validate(const Composition&) -> ValidationResult{code,message}, explicit bool conversion; secondsAt(const Composition&,Tick) -> optional<Seconds>; tickAt(const Composition&,Seconds) -> optional<Tick>. Time APIs reject bad tempo maps/nonfinite/out-of-range input, nearest-tick inverse. Value types do not depend on JUCE.

1. Write assert-based behavior tests with valid multi-track fixture, all scalar/event bounds, duplicate IDs, map origin/order, meter changes at6/8 bar boundary, tempo integration (480ticks@500000us +480@1000000us =1.5s), inverse, note endpoint overflow, resource count/capacity and256/257 distinct keys. Test repeated same-pitch overlaps and shared10 ownership/controller rejection.
2. Compile with `c++ -std=c++20 -Wall -Wextra -Werror plugin/tests/CompositionTest.cpp -o /tmp/loopforge-composition-test`; observe failure for missing engine API, not accept a green test without implementation.
3. Implement the typed values and validation/time behavior. Use checked preflight counts and capacity estimates before scratch allocation. Sweep sorted note edges with release-before-on at equal ticks, counted channel/pitch keys. Reject unknown enum values and unsupported resolution/version.
4. Run the standalone compiler command and binary; expected valid fixtures accepted and every invalid mutation rejected. Add standard-only composition_test to CMake/CTest (enabled on Windows/Linux). Run sanitizer variant with undefined/address instrumentation and capacity/count stress.
5. Record RED/GREEN and actual commands in task.md; commit the independently usable model slice. Do not mark all E02 complete.

### Task 2: Explicit legacy idea/state migration

**Files:** create plugin/Source/engine/LegacyMigration.h; create plugin/tests/LegacyMigrationTest.cpp; modify plugin/CMakeLists.txt.

**Interfaces:** consumes composer::Composition and validate; produces importLegacyIdea(const juce::var&) and importLegacyState(const juce::var&) -> ImportResult{optional<Composition> composition,string error,string timingNotice}; no mutation/publication. Input legacy state version1 only. Reuse wire::schedule for existing note constraints, then explicitly validate metadata before conversion. Deterministic IDs from fixed field/order input; original payload retained as JSON. Preserve channels1/2/3, seed, key/mode/style and tempo. Missing legacy seed becomes0 with a visible provenance notice; no new randomness. Absolute starts/endpoints rounded separately to480ticks/quarter, clamp positive notes to one tick within pattern endpoint and disclose changed times.

1. Write real JUCE fixture test importing plugin/tests/idea.json; assert D minor/nu-metal/seed42, tempo600000us, end7680ticks, three tracks/channels and exact note times. Assert source JSON identical after import; repeat import yields identical IDs. Test subtick/tail timing, missing seed, bad/fractional/boolean seed, key/mode/style mismatch, invalid saved-state versions, invalid note, over-note-limit, invalid tempo and preservation of source.
2. Compile/run the new target before implementation to observe missing migration API. Configure a separate /workspace/loopforge-engine-build with existing /workspace/JUCE and native sysroot; no dependency fetch and no reuse of the UI build directory.
3. Implement adapter with bounded legacy source preflight, strict optional metadata checks and validated result; on any error return no document. Keep processor/state/wire-v1 untouched.
4. Build migration target; CTest composition_v2/legacy_migration must pass. Run npm test, npm run sample in this isolated worktree, full native build/CTest and both HTTP fixture scripts as appropriate. Review exact output rather than inferred success.
5. Record evidence in task.md and commit; verify source diff excludes legacy callback/protocol. Request one fresh independent code review of the whole foundation range and fix Important/Critical findings with failing-first regressions, then green suites. Push to existing separate draft PR3; no merge/release.

## Scope boundary

This plan implements the first E02 dependency slice, not the entire supplied specification. Musical planners, config/prompt generation, native UI mode, v2 JSON persistence, full-song playback/history/export and neural integration remain subsequent E02–E06 slices. The model is used by the real importer and tests now; no claim that unfinished UI or full composer is usable. Continue from the next unblocked slice after this plan is verified.
