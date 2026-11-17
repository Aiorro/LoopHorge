# Composer note editing — 2026-11-17

Editing vertical after87ba3a2, within R02. This is progress toward the loop release, not closure of R02 or release readiness.

## Result

The existing piano roll now edits Composer tracks using their actual IDs, names and MIDI channels. Add/move/resize/delete, quarter-beat quantize and Undo/Redo operate on the committed typed Composition. View projection uses quarter beats and explicit meter bar starts; decimal string IDs preserve all64bits. Canonical note sorting retains the edited selection by ID. Other tracks, maps, controllers, presets and existing IDs are preserved. Pure half-open range replacement is available to the next scoped-regeneration step; it is not a completed regeneration UI.

The real editor switches between overview and Edit Notes without exposing Legacy generation controls. Track selection and keyboard operations are exercised through the actual editor, not a mock. Snapshot mutation cannot change processor state. Save/reopen preserves edited notes; restoring a whole document resets musical history. Next-request direction/seed/mood drafts survive musical Undo.

Interactive documents are limited to16meter bars,8192notes and8MiB serialized project. Candidates must remain inside the same limits before publication. Existing larger documents retain their data, overview, playback and export. History keeps at most32states and16MiB of serialized retained state. All model/project/view/history preparation occurs outside RT before guarded publication; stale revisions/jobs reject. This does not prove whole-process256MiB peaks or exhaustive scheduling/allocation behavior.

## Tests and fixes

New model/Processor/piano-roll tests were run RED before implementation. Source model allocation-failure sweep covers add/update/delete/range replacement and recovery. New integration cases cover four tracks, canonical selection, quantize/Undo, stale revision, draft preservation,32Undo/Redo steps, edited save/reopen,3/4axis, full uint64 IDs and old longer documents.

Five defects were reproduced before this editor was published:

- ERR079: restoring a long read-only document into Edit Notes dereferenced its absent track projection. Real-editor timer regression SIGSEGV→GREEN; force overview and guard absent arrays.
- ERR080:8192→8193notes could publish a read-only result hiding Undo. Dense-cap Processor regression RED→GREEN; reject before state/history mutation.
- ERR081: ownership was required for sole channel10tracks although the existing model requires it only for shared percussion. Sole-drum add/update/range cases RED→GREEN; shared ownership still enforced by final validation.
- ERR082: delete-last/add indexed an empty prior note array. Actual piano roll SIGSEGV→GREEN; bound selection and retain-1for an empty list.
- ERR083: entering Edit Notes scrolled to highest pitches. First-note viewport assertion RED→GREEN; scroll follows the selected track.

Focused RED logs: `/tmp/composer-editing-red.log`, `/tmp/composer-piano-red.log`, `/tmp/composer-readonly-red.log`, `/tmp/composer-editor-limit-red.log`, `/tmp/loop-edit-sole-drum-red.log`, `/tmp/composer-empty-layer-red.log`; viewport RED is recorded in the initial full fix-suite output and ledger. No product source is intentionally left failing.

Final evidence:

- Linux VST3/Standalone/all targets build: `/tmp/composer-editor-final-build.log`; current test recompilation: `/tmp/composer-editor-test-final-build.log`.
- CTest42/42,0failed: `/tmp/composer-editor-ctest-final.log`.
- Node93/93,0failed,0skipped, with CMake PATH: `/tmp/composer-editor-node.log`.
- Native API, protocol failure/cancel/recovery and stale worker fixtures: `/tmp/composer-editor-{api,failures,worker}.log`.
- Isolated Node sample generated,992MIDIbytes; hashes of both user sample files unchanged.
- ASan/UBSan pure model: `/tmp/composer-editor-model-san.log`; actual piano-roll test plus instrumented actual Plugin.cpp: `/tmp/composer-editor-ui-san.log`. Vendor modules are not fully instrumented; no TSan/whole-host claim.
-1MiB-stack integration tests: `/tmp/composer-editor-small-stack.log`; processor test owners are heap allocated. This is not Windows execution.
- Minimum/maximum actual editor snapshots inspected: `/tmp/loopforge-composer-edit-{min,large}.png`. Fontconfig cache warnings are emitted by this headless environment; tests exit0.
- Original T01–T15 rows and supplied task-engine.md payload remain unchanged. errors.md now has83unique IDs:76Fixed/4Open/1Investigating/2Blocked.

## Independent review and rulings

Fresh gpt-6.1-sol/high review inspected connected model, history, publication, UI and tests read-only. Earlier frontier model quota was unavailable, so the capable available reviewer was chosen explicitly. Verdict:0Critical/3Important/1Minor, ready with fixes. The three Important findings above were each reproduced and fixed in one TDD pass with green full suites; no second review was used as proof of correction.

Rulings:

- Small-loop editing bounds are independent of import/model bounds. Reason: bounded projection/history without truncating old music. Cost: unusually dense/large imported loops remain read-only.
- History32states/16MiB, cleared on full restore/new generation/mode switch; recipe draft remains independent. Reason: Undo changes committed music without rolling back newer request settings. Cost: large editable documents retain fewer than32steps; mode switch loses the current musical history.
- Reuse existing piano roll and typed immutable edits rather than another editor. Reason: preserve working controls and data boundaries. Cost: view/model/serialized history have additional bounded non-RT copies; aggregate memory acceptance remains open.
- Reviewer-declined remaining controls/scoped regeneration, audition/seamless loop, Windows/REAPER/chooser/drag/package, aggregate memory/slow IO and historical whole-repo audit remain task.md work. Reason: this vertical only claims note editing. Cost: the full packaged product scenario is still incomplete.
- Reviewer did not independently instrument exhaustive new history allocation failures/host-worker interleavings. Publication ownership was inspected; existing restore-allocation/RT/worker suites and fresh sanitizers passed. Cost: no universal allocation/concurrency guarantee; R05/E06 stay open.

Deferred minor: Mood/Auto exit says SHOW OVERVIEW even when it returns to the prior piano roll. The destination works; its label is misleading. The prior unconfirmed Compose/host-restore fallback-seed race remains recorded in task.md and the foundation review; no controlled reproduction or broad concurrency claim is added here.

## External acceptance

Foundation87ba3a2 is pushed. Native workflow44035856842 failed: Windows failure,Ubuntu cancellation,no artifacts; Windows job133878441747 has empty step summaries and logs404BlobNotFound. Exact cause remains unknown. Prototype/Web workflows also report failure. These facts do not identify a source/build failure without execution logs. Linux tooling/artifacts do not establish Windows or REAPER acceptance.

The Linux foundation engineering ZIP has SHA2566c496b9448616e425879d5d7b30371f0ab10d6bdf13f441ba29166c72982a448; it predates this editor. Package manifest host/install/license flags are false. Actual Windows build/package/clean-machine tests, JUCE/VST3 redistribution clearance and real host/MIDI drag acceptance remain open. No public release or main merge performed.
