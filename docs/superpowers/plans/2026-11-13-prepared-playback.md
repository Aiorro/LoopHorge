# Prepared Composition Playback Implementation Plan

> Execute inline with superpowers:executing-plans/TDD and one fresh review. Approved architecture and continuation authorize execution. Task.md remains the sole progress checklist.

**Goal:** Compile v2 compositions for indexed seek and render bounded MIDI blocks without allocating/freeing on the rendering thread.

**Architecture:** Worker-owned move-only PreparedSong contains compact note-state/CC lanes, sorted event references and precomputed tempo segments. Playback reads a const song and keeps a fixed16x128 counted ledger. Host PPQ and composition-seconds clocks share the event cursor; discontinuities flush/chase and overflow pauses until a safe restart.

**Tech Stack:** C++20 standard library, existing model/validator and CTest; no JUCE/network/process/file dependencies.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, Prepared playback and realtime ownership. This slice does not activate v2 in Processor: atomic three-slot publication, actual JUCE sink/audition, state envelope/UI/loop-range integration remain subsequent work.

## Global Constraints

Existing v2 version/resource/channel/density limits;64MiB document+actual prepared storage. Max256 held keys. Max2048 MIDI messages/block with384 reserved for releases; at most1664 ordinary event references processed/block. Flush requires at most256 NoteOff+48 pedal resets. Chase at most48 bank/program+256CC+256NoteOn, and no duplicate exact-target onset. Counted overlapping/retriggered notes; simultaneous unisons coalesce last source velocity as MIDI export does. No callback allocation/free, locks, waits, I/O, shared ownership or whole-song scans. Sink must be nonthrowing and preallocated. Legacy processor/state/bridge/UI unchanged. Before host origin/after song end no notes; source tempo used only for standalone clock. Stop/seek/change/overflow release CC64/66/69. Historically pedal-held released notes are not resurrected on seek.

## Review Focus

- Dense nested same-pitch lifetimes, simultaneous onsets, exact note-end/start and same-tick CC boundary must restore correct counts/last velocity and avoid duplicate chase/onset (Tasks1/2).
- Negative PPQ, stored origin, stop/resume/replacement, tempo changes and both sides of block boundaries must not lose/repeat MIDI events (Task2).
- Every16-channel setup/controller/key, maximum counts and huge blocks must obey event/work/release budgets, pausing persistently on overflow (Tasks1/2).
- Tiny32/64samples and multiple standalone tempo changes must use precomputed binary-search time conversion, not per-sample/whole-song scanning (Tasks1/2).
- Preparation memory/cancellation and render new/delete interception must verify actual capacities/lifetimes; no whole-plugin/slot/host allocation claim follows from this core (Tasks1/2).

### Task 1: Worker preparation and indexed chase

**Files:** create plugin/Source/engine/Prepared.h; plugin/tests/PreparedTest.cpp. Modify Validation.h/CMakeLists.txt/task.md.

**Interfaces:** PrepareResult{optional<PreparedSong> song;string error}; prepareComposition(const Composition&,const atomic<bool>*cancelled=nullptr). PreparedSong is move-only, constructed only by the validated builder. Public const methods: endTick(), durationSeconds(), storageBytes(), eventCount(), event(size_t)->optional<PlaybackPoint>, firstAfter(Tick), chase(Tick)->ChaseState, secondsAtTick(double)->optional<double>, tickAtSeconds(double)->optional<double>. PlaybackPoint carries tick/key/count/velocity/onset or control/value. ChaseState has bounded256notes with counts,256controls and16presets. ValidationResult::accountedBytes reports the existing successful conservative storage estimate, permitting exact prepared capacity substitution without changing validation rules.

1. Write failing tests for literal overlap/count/velocity snapshots on either side of boundaries, CC ordering/presets, origin/endpoint, tempo map conversions and cancellation. Expected: missing Prepared.h before production code.
2. Implement stable worker edge grouping by key/tick/off-before-on, compact note-state lanes with last-onset velocity and32bit count, sorted CC lanes with stable same-tick order, and event references sorted by time/off-control-on priority. Binary-search lanes for chase after all events at target; cursor starts strictly after target. Tempo segments precompute elapsed seconds. Check actual capacities plus source storage<=64MiB and cancellation at bounded preparation stages. Expected: exact fixtures pass.
3. Add maximum131072 same-pitch count,256keys/256CC/48preset setup, invalid density,4096tempo segments, memory/cancellation and query-allocation cases. Run strict compiler/full native+Node/sample and commit evidence. Expected: all pass; no legacy source change.

### Task 2: Bounded event-driven block renderer

**Files:** create plugin/Source/engine/Playback.h; plugin/tests/PlaybackTest.cpp. Modify CMakeLists.txt/README/task.md.

**Interfaces:** Playback::render(const PreparedSong&,const PlaybackBlock&,Sink)->PlaybackResult. PlaybackBlock has Clock{hostQuarterBeats,compositionSeconds}, position, sampleRate, bpm, hostStartQuarterBeat, samples, playing. Position is host absolute PPQ or standalone seconds supplied by caller; result.nextPosition advances appropriately. Sink(MidiPacket,int sampleOffset) is preallocated/nonthrowing. Result reports messages/events/overflow/invalidClock/ended. Each block reuses fixed state; const source lifetime remains caller-owned. Continuous blocks use cursor; source/clock/origin change, stop/start or position discontinuity flush/chase. Zero-size blocks leave transport untouched. Invalid clock flushes and flags error. Negative pre-origin blocks wait/cross origin; end is one-shot. No explicit loop-range control in this API; caller transport jumps are seeks.

1. Write failing tests for literal MIDI traces at overlap/boundaries, host vs standalone tempo maps,32/64buffers, non-sample-aligned onsets, origin crossing, stop/seek/replacement, invalid clocks, max256key/CC setup and2048message/work overflow with persistent pause/restart. Expected: missing Playback.h.
2. Implement bounded template sink renderer and fixed note ledger. Preflight per-event messages, reserve releases, cap invisible event traversal, emit first sample at/after event, and skip exact-target events already chased. Reset touched channels' pedals on release; don't restore pedal-only historical notes. Expected: all traces/budgets pass.
3. Intercept C++ allocation/deallocation during cold start/seek/steady/stop/dense/overflow renders and indexed queries with fixed sinks. Run full Linux VST3/Standalone/CTest, Node/sample, strict compilers and ASan/UBSan; commit/docs. Expected: no tracked new/delete and all suites pass. Run task-done and one fresh independent review; fix Critical/Important RED→GREEN, preserve exclusions and push existing PR3 without merge/release.
