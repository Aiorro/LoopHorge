# Loop release foundation — 2026-11-17

Based on6673691 and the supplied finalization prompt. New product scope: native offline loops<=16bars, no songs/sections/new models. Original task-engine.md remains preserved; user correction and supplied prompt supersede incompatible historical scope. task.md now contains one finite active release queue; prior execution evidence is archived verbatim as literal history, not a competing checklist.

## Generation boundary (R01)

Config/generateLoop/generateDrums and resolved Composer accept4/8/16. Legacy/web/API/MCP retain4/8; unsupported bars and unknown root generation fields (including duration aliases) reject before work/provider dispatch. Composition codecs/playback/export keep larger-document bounds. A simulated old32bar source with endpoint61440and tail60000still restores/exports; rejected32/17/100text/manual requests preserve it. Prior32bar success stress fixtures use max16; the stale modulation28was moved to12after full-suite RED exposed the mismatch.

RED: `/tmp/loop-length-{node-red,config-red,legacy-red,native-red}.log`. Unknown duration fields separately reproduced silent4bar output and now reject (`/tmp/loop-length-fields-{red,green}.log` and native regression). The generation limit is not a document migration. Portable recipes remain exact draft data; unsupported directions reject on Compose, not by silently rewriting recipe files. Legacy's supported contract was not expanded into a v1 schema rewrite. Cost: old callers that sent undocumented extra generation fields must remove them instead of relying on ignored input.

## Host/project workspace (R03 exposed controls)

Project-v3 adds a strict validated generationRecipeJson field; plain v2 remains its original exact five-field schema and v1 bytes are preserved. Writer chooses v3 only with recipe metadata. Typed workspace draft updates do not advance musical revision or cancel the captured current job. Host/project saves overlay latest valid draft, while worker results use captured input. External-restore epochs prevent stale UI draft writes; new editors restore input and Auto pins, and close flushes pending valid edits. Opening/closing an untouched old document does not add metadata: the existing ComposerProcessor regression caught this during implementation and now passes via rendered-input equality.

Invalid seeds remain visible/rejected at Compose; only last valid typed seed is persisted. Current portable recipes cover seven mood pins; other existing manual API calls still generate but cannot claim a portable recipe for unexposed fields. R02 must extend matching recipe support when adding such controls.

Canonical serialized size is cached outside publication, avoiding full model parse per keystroke. Compactly escaped older large documents may load even when normalized writing would exceed32MiB; new draft additions then refuse while raw old bytes remain savable. ERR078 reproduced a second failure path: a larger later Legacy publication could invalidate a retained draft's save budget. Pre-publication guards now reject that result before source/history mutation. WorkspaceRecipeTest proves both near-limit paths, stale epoch, invalid preservation, pre-generation saves, in-flight next settings and v1/v2 compatibility. Codec uses direct initializer-list calls; an interim conditional temporary-list implementation failed the new v3 roundtrip test and was corrected before publication.

## Packaging path (R06 engineering tooling)

scripts/package-native.cjs validates real x64 executable structures, clean pinned JUCE, complete recursive VST3 resources, fixed modes/times, hashes/manifests and deterministic CMake ZIP. It refuses existing destinations, uses owned scratch and does not install/publish anything. Seven fixture tests pass, including failure preservation, malformed/missing inputs, notices, symlinks, differing umask and extracted executable permission. docs/NATIVE-PACKAGING.md contains portable Windows/Linux installation/removal/routing and actual dependency notices. Included notices do not establish JUCE/VST3 redistribution clearance.

## Final verification of this checkpoint

- Full native Linux VST3/Standalone/all targets build: `/tmp/loop-release-build-final.log`.
- CTest39/39,0failed: `/tmp/loop-release-ctest-final.log`. The first full pass found the stale Rhythm modulation fixture; a later pass found the untouched-editor state mutation; both original regressions pass in the final suite.
- npm test with CMake PATH:93/93,0failed,0skipped: `/tmp/loop-release-node-full.log`.
- Native API, failure/framing/timeout/cancel/recovery and stale worker fixtures passed: `/tmp/loop-release-{api,failures,worker}.log`.
- Sample generated with current unchanged Node music source in an isolated temp source tree; user sample files unchanged.
- ASan/UBSan main tests plus actual Plugin.cpp passed for real UI and workspace/big-state/publication cases: `/tmp/loop-release-{ui,workspace}-san.log`. Linked vendor modules are not fully instrumented; no whole-host/TSan claim.
- Original15Txxdefinitions/statuses and supplied task-engine payload remain unchanged. User finalization prompt is separately preserved.

## Independent reviews and rulings

Length review identified the Rhythm modulation mismatch, fixed via its failing test/full suite. It found no production cap or old-document regression. Root also treated undocumented duration normalization as a user-visible length-contract problem and corrected it in the one test-first fix pass.

The first workspace/packaging reviewer hit model quota before completion, after tracing the save-budget invariant risk. Root independently reproduced/fixed ERR078. A fresh available-model fallback completed the review after that fix and found no confirmed Critical/Important issues. No second approval review was used to certify test fixes. A partial LoopEditing implementation task also stopped on model quota; its model code/tests are being continued separately and are not part of the packaged foundation claim.

Deferred minor: host restore can race between seed parsing and draft sync in Compose, potentially pairing a refreshed direction with previously parsed fallback seed. No controlled reproduction was provided; the review did not classify it blocking. Cost if it occurs: one request may use an unexpected fallback seed. Keep it visible for targeted integration acceptance; no broad concurrency guarantee is claimed.

Rulings: recipes are draft DTOs, generation enforces limits; old long state is not truncated; unsupported non-UI manual pins do not gain false recipe replay guarantees; ordinary failure-safe staging is not power-loss certification. Costs/limits are explicit above rather than silently closing broad E03/E04 acceptance.

## Remaining release gates

This is not the completed product. Composer editing/Undo/Redo/selection regeneration and audition/loop workflow still need R02; actual current Windows/REAPER/pluginval/chooser/drag acceptance needs R04; aggregate peak memory/slow IO/listening and applicable risk decisions need R05. Latest native run44023446517 at6673691 has Windows cancelled/Ubuntufailed/no artifacts/empty steps/log404; exact cause unknown. R06 needs the actual Windows package, clean-machine test and chosen dependency licensing route. No public release or main merge performed. A Linux engineering candidate only demonstrates the packaging path, with manifest acceptance flags false.
