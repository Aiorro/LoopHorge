# Error Registry and Drag Lifetime Repairs

> Execute inline with superpowers:executing-plans/TDD and one fresh final review. User explicitly requests errors.md with statuses and continued work. task.md remains the only execution checklist.

**Goal:** Collect known findings with honest status/evidence, then close the two confirmed open T13 drag-lifetime defects.

**Architecture:** errors.md is a diagnostic register with stable IDs, status, severity and verification scope, linked to task.md. Atomically set close-on-exec when creating POSIX drag descriptors. Completion callback copies share a thread-safe lease holder that is cleared on completion or failed start, so keeping a finished callback does not keep the file locked.

**Spec:** ../specs/2026-11-12-midi-drag-design.md, retained lease/cache contract; task.md T13 deferred findings and the user's2026-11-15 registry instruction.

## Global Constraints

Preserve original15task rows and supplied task-engine payload. No competing task checkboxes/priorities in errors.md. Distinguish bugs, verification gaps, accepted limits and infrastructure failures; Fixed means regression passes only in its stated scope. Do not label all staged missing Composer features as bugs. Preserve24h cache retention, active native lease ownership,256entry/16MiB caps, exclusive writes and symlink checks. POSIX open flags must be atomic at creation, not a later fcntl race. Windows handles retain existing non-inheritance semantics. Completion can outlive/destroy source and run on another thread; retained copies/repeated completion must be safe. No waits/IO/allocations added to audio callback. No DAW/pluginval/Windows execution or release claim.

## Review Focus

- Child exec completion must be observed before dropping parent lease; close inherited resources before asserting to avoid orphan test children (Task1).
- Active drag must keep its file protected before completion, even if editor/source dies (Task1).
- Retained callback copies must not retain the file after completion or rejected start; repeated/concurrent completion must not double-close (Task1).
- All POSIX drag opens, including short-lived cleanup/write descriptors, require atomic close-on-exec (Task1).
- Registry entries must preserve historical evidence and show unresolved limitations honestly without replacing task.md (Task1).

### Task 1: Register findings and repair descriptor/completion lifetime

**Files:** create errors.md; modify AGENTS.md/task.md/plugin/Source/{DragMidi.h,MidiDragSource.h}, plugin/tests/{DragMidiTest.cpp,MidiDragSourceTest.cpp}, plugin/README.md.

**Interfaces:** Existing public drag/cache APIs remain unchanged. Descriptor fix adds O_CLOEXEC to POSIX opens only. Completion holder is shared between std::function copies and thread-safe, released on callback or failed dispatch; no raw source access after native dispatch. Test child mode is private to drag_midi_test, running the same executable via actual fork/exec with ready/release markers and bounded cleanup/waitpid.

1. Create registry from prior task/review evidence with stable ERR IDs and Fixed/Open/Investigating/Blocked statuses. Document relationship to task.md in AGENTS.md and keep original15 rows unchanged. Expected: no fabricated re-audit/platform pass, no competing checklist.
2. Write behavioral RED tests: active lease protects expired MIDI while exec child is alive; after parent releases it, child must not pin it. Child signals readiness only after exec and is released/joined before failed assertions. Source tests keep copied completion functions alive after completion and after failed native dispatch; expired files must become removable, including source destruction and concurrent repeated callbacks. Expected: existing code fails lease-cleanup assertions.
3. Implement atomic POSIX O_CLOEXEC on all three opens and a shared atomic shared_ptr lease holder for callbacks. Clear holder on actual completion and rejected start. Expected: focused drag/cache tests pass; active leases still protect before completion and no source access after dispatch.
4. Run full Linux VST3/Standalone/CTest, Node/sample equality, relevant native API/failure/worker checks and focused sanitizer probe. Update errors statuses only after GREEN, record exact evidence/task.md and commit. Fresh review once; fix Critical/Important RED→GREEN in one pass, ledger minors/rulings, update existing draft PR. No merge/release.
