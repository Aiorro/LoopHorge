# Native Composer Direction Controls

> Execute inline with superpowers:executing-plans/TDD and one fresh final review. Approved architecture and repeated continuation authorize this E03/E04 integration. task.md owns progress; errors.md records newly reproduced defects.

**Goal:** Generate a real native Composer loop from direction text and fallback seed, inspect/play its committed document, and switch between preserved Legacy/Composer sources.

**Architecture:** Queue an immutable bounded Composer request on the existing processor worker; parse/generate/encode/prepare outside audio. Guard the final v2 commit by captured revision/job/mode and cancellation. Preserve the exact current legacy envelope; for the original unsaved demo, snapshot the same shared demo builder without changing initial v1 state/UI behavior. Mode selection reuses validated v2 restore with an optimistic commit guard. Native controls use the existing direction box and a uint32 fallback-seed field; all other Composer parameters come from text/defaults and are explained in a resolved summary.

**Tech Stack:** C++20/JUCE8.0.8, existing LocalPromptInterpreter/generateResolvedLoop/ProjectJson/Prepared; no network/server/model/dependencies.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, deterministic offline prompt/mood, source preservation, job/revision/mode commit policy, compatible state and honest capability boundaries.

## Global Constraints

Text<=4096UTF8bytes; uint32 caller seed is fallback, explicit text seed wins. Existing supported4/8/16/32bar loop backend and bass/chords/lead/drums/presets/mood are used; unsupported song/seconds/sections/blends/roles reject without truncation/current-state mutation. No stale/cancelled/error result publishes. Request captures base state before worker execution; no32MiB queued clone retained after job completion. V1 opening remains unquantized and v2 preserves inactive documents/origin. Current v2 codec stays unchanged: committed notes/maps/seed persist; direction UI/fallback seed are next-request editor controls, not yet a persisted full generation recipe/manual-pin UI. Explicitly document that limitation; E03 remains open for seven manual axes/Auto pins/recipe persistence, E04 for history/edit/export/audition/arrangement. Native mode stays MIDI-only/read-only for editing; legacy workflow remains functional. No audio callback changes/allocations/locks/IO. Keep original15task rows/requirements/samples unchanged.

## Review Focus

- New result must not replace a restore/edit/mode change queued while generation was resolving/preparing (Task1).
- Cancellation accepted before Composer commit must leave the project untouched; busy/status completion must not erase a later queued request (Task1).
- Unsaved demo fallback must preserve its musical source, and both saved mode documents must survive switches/round trips (Task1).
- Unsupported/invalid/oversized requests must retain current state and show diagnostics/default provenance, not silently substitute a loop (Tasks1/2).
- UI must not conflate legacy controls with text/default Composer parameters; edited input during busy is for the next request, preserved snapshots remain immutable (Task2).

### Task 1: Processor-owned native compose request

**Files:** modify plugin/Source/Processor.h; create plugin/tests/NativeComposeTest.cpp and register CTest.

**Interfaces:** requestComposer(const juce::String& direction,uint32_t fallbackSeed)->bool accepted; cancel()->bool cancellation sent (existing callers may ignore). canSwitchMode()->bool; selectMode(composer::ProjectMode)->bool optimistic whole-document mode switch. composerSummary()->juce::String; summary includes field values/origins and diagnostics. Private CommitGuard{revision,job,mode,generated}; restoreV2 accepts optional guard and prebuilt status, checks under the commit mutex. Job completion clears busy inside the same guarded commit and returns/continues without a later cleanup that could clear another request.

1. Add failing tests for missing request API, seeded RU/EN4track generation, exact repeated output and changed seed, unsupported/conflict/oversize preservation, original v1/demo retention, cancellation, source switches/origin, mode guards and actual MIDI. Expected: missing API compilation RED.
2. Implement request capture/worker resolve/generate/save/prepare and guarded commit; default demo notes shared between initial schedule and fallback snapshot, initial saved-state behavior unchanged. Capture readable text/default summary; do not echo private source JSON. Mode switch preserves both documents and stored origin. Expected: focused tests pass.
3. Prove guarded completion against newer edit/restore/mode and cancellation; real worker recovery plus old API suites. Run full native/Node/sample and record evidence; commit. Expected: no new state on failure/stale/cancel, old APIs pass.

### Task 2: Native controls and end-to-end UI

**Files:** modify plugin/Source/Plugin.cpp; extend NativeComposeTest.cpp or add native UI target; update README/task.md/errors.md.

**Interfaces:** Existing prompt box is direction; new uint32 fallback-seed editor and Composer/mode actions invoke Task1. In legacy mode action creates Composer when no saved Composer exists, otherwise views it; in Composer mode it returns to saved Legacy when available. Composer view exposes compose-again and editable next-direction controls, with read-only rendered notes and resolved text/default summary. Retain Open/Save project, cancel, play and current file/RT contracts.

1. Write failing real-editor tests for action/seed input/compose-again, busy guard, source switch and preserved v1; invalid seed must not queue or mutate. Expected: controls missing.
2. Implement bindings, compact two-mode layout/focus/tooltips and post-commit host dirty notification on the message thread. Show MIDI-only/read-only and supported-loop limits; hide legacy musical/provider fields in Composer. Expected: actual worker/UI interactions and minimum/expanded geometry pass; render/inspect native screenshots.
3. Full build/CTest, Node/sample, old API/failure/worker fixtures and scoped sanitizers; update errors immediately for reproduced bugs. Fresh whole-slice review once; Critical/Important RED→GREEN one pass, then publish existing draft PR. No merge/release or fresh Windows/DAW claim.
