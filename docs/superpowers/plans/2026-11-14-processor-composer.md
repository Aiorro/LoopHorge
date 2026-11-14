# Processor Composer State and Playback Implementation Plan

> Execute inline using superpowers:executing-plans/TDD with one fresh final review. Approved architecture and repeated continuation cover this dependency. Task.md is the only progress checklist.

**Goal:** Make a restored project-v2 play the document and mode it saves, while preserving exact legacy playback and rejecting stale worker results.

**Architecture:** Extend Processor's existing three atomic schedule slots with optional immutable PreparedSong, mode and origin. One mutex serializes all non-RT producers, including synchronous host restore; consumer exchanges only the slot index. Prepare and serialize candidate state before committing; no callback moves or destroys vectors. A mode change with old MIDI to release spends one callback draining the old ledger; the next callback starts the new mode at absolute host PPQ (or standalone seconds0), never merges two independent output budgets. Composer is initially MIDI-only and existing legacy edit/generate/export/drag controls are disabled for that mode; no fake v2 editor.

**Tech Stack:** C++20/JUCE8.0.8, existing ProjectJson/Prepared/Playback/PreparedMidi; no dependencies/network changes.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, state compatibility, ownership, worker cancellation, host timing and bounded output.

## Global Constraints

Keep old v1 numeric beats/loop scheduler unchanged; opening v1 never quantizes. Decode entire v2 candidate (32MiB) and prepare outside audio callback; invalid/unsupported/overbudget restore preserves state, revision, playback and history with sanitized status. Retained slot PreparedSong capacity<=64MiB, fixed Schedule/control memory excluded and total256MiB/peak/history integration remains open. Non-RT producers serialized by mutex; callback never takes that mutex. Reclaim only slots CAS-claimed free on producer; old pending is retired after exchange. Save cached validated bytes coherently with publication; v2 legacy edits retain inactive composition rather than downgrade. v1 restore retains original JSON bytes until an explicit edit. Restore increments revision/job generation and cancels busy work; generation commits only with matching captured revision/job and legacy mode. UI safety/disclosure only; multi-track editor, Composer generation controls, full audition and v2 UI export remain subsequent work. Host buffers need existing32768-byte reservation. Destruction joins worker after host has stopped callbacks; final teardown MIDI delivery remains host acceptance work.

## Review Focus

- Concurrent restore/worker/render must never pair a new mode/origin with an old song or reclaim an active vector (Task1).
- Legacy↔Composer transitions and zero blocks must preserve final old releases; new notes start on the following nonzero callback without changing host origin (Task1).
- Invalid restore must preserve saved bytes, revision, notes/history and running job; active/inactive corrupt payloads reject (Task1).
- A generation captured before edit/restore must not replace a newer project; repeated requests after cancellation must recover (Task1).
- Composer mode must never expose stale legacy editing/export/audition as if it affected the playing composition; v2 legacy edits must preserve inactive Composer data (Task1).

### Task 1: Coherent Processor integration

**Files:** modify plugin/Source/Processor.h, plugin/Source/Plugin.cpp, plugin/CMakeLists.txt, plugin/README.md, task.md; create plugin/tests/ComposerProcessorTest.cpp.

**Interfaces:** Processor::isComposerMode()->bool (non-RT locked UI getter); existing setStateInformation/getStateInformation accept/save valid v2 and retain v1 behavior. Slot metadata{Schedule data,optional<PreparedSong> song,bool composer,double origin}; producer reserve/reclaim under Processor mutex; render uses active-slot immutable fields. Existing public APIs guarded against Composer edits/generation. No separate publication path or raw independent mode atomics. Job generation/request revision checked before install. Both producer and callback must be quiescent before destruction, as before.

1. Write failing end-to-end test restoring Composer state with channel16, overlapping notes/CC/presets and nonzero origin, then exact getState round trip; confirm expected MIDI at host start, seeks/stop/replacement,32/64samples and standalone. Test v1→v2 and v2→v1 drain blocks, zero-size behavior, invalid restore state/revision preservation, inactive Composer preservation on a v2-legacy edit, and rejection of legacy edit/request in Composer. Expected: current Processor fails v2 restored-state assertion.
2. Implement prepared typed slot and transactional non-RT commit with cached state. Reserve/update slots only after candidate validation/serialization succeeds; reclaim returned vectors only on producer. Link LoopForge PUBLIC loopforge_prepared_midi for header consumers. Render Composer via Playback/PreparedMidiSink; spend one block flushing on mode change, then use absolute host origin or composition seconds. Preserve current legacy loop timing. UI disables legacy-only controls and status always discloses MIDI-only/editor/export limits in Composer. Expected: new focused tests plus existing native tests pass.
3. Add real stalled loopback response test: start old request, restore a valid v2 before releasing old response; current project must survive cancellation/late completion and a later v1 offline request recover. Add callback allocation/deallocation hooks and concurrent restore/render stress using actual Processor, preallocated buffers and fixed MIDI ledger. Test producer slot budgets with bounded generated sources. Run full native/Node/sample, ASan/UBSan (linked JUCE scope disclosed), TSan if available and existing HTTP scripts. Expected: no callback heap activity, mixed publication, hung worker, stale commit or unbalanced notes. Record actual evidence and remaining limits, commit and run task-done. Fresh review one pass; fix Critical/Important RED→GREEN with full suite, then update existing draft PR. No merge/release or unverified DAW claims.
