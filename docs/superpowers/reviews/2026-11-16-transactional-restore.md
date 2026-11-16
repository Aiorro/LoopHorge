# ERR008 transactional restore verification

Basef2d3839; next assigned P1 under the user-directed feature freeze. Changes: Processor.h, new RestoreAllocationTest.cpp and CMake target. No model/state schema or audio callback changes. Native owner implemented; root independently reviewed source/logs and ran Node/sample, sanitizer and protocol verification.

RED: host one-shot allocation failures produced29preservation/recovery assertion failures, including historical106 and legacy41. Mode allocation0 escaped before its stateJson capture entered the try block (two additional RED cases). Fix prepares allocating status values before reservation/mutation, immediately owns legacy DynamicObject preparation with var, and commits state/runtime through noexcept moves before publication. Mode capture is inside the handler; generated/mode advisory notifications use a noexcept best-effort boundary rather than reporting successful publication as rejection.

GREEN: host allocation sweeps107legacy→Composer,107Composer→Composer,42legacy→legacy,42Composer→legacy; selected-mode sweeps155/143. Total596positions plus historical106 and explicit generated/mode notification boundary bad_alloc/recovery. Tests require rejected bytes/revision/replacement/mode/undo-redo/rendered source preservation, exact success bytes/matching playback and repeated pending-slot recovery. The notification wrapper injects a genuine one-shot C++new exception at the call boundary on Linux; no claim that every worker allocation is swept.

Full Linux VST3/Standalone build and CTest36/36 passed. Node55/55/sample byte equality passed. Allocation test+actual Plugin.cpp ASan/UBSan passed, linked other JUCE/GUI/audio/SDK modules uninstrumented. Initial sanitizer compile diagnosed legal malloc/free-backed replacement new/delete as mismatched after inlining; sanitizer-only -fno-inline avoids that optimizer diagnostic while retaining -Werror (pre-existing indentation warnings remain visible). Production compile options unchanged.

Fresh independent gpt-6-astra/high review found no Critical/Important/Minor; copied and ran allocation, ComposerProcessor and NativeCompose binaries, independently confirming zero sweep failures, source/publication/cancellation and callback allocation guards. Did not mutate source/shared build or independently run full build/sanitizers. No second review/fix pass needed.

Final: Ruling: Verification covers finite recoverable C++new/new[] failures in tiny host/mode fixtures, not all worker/vendor/malloc failures or sustained system OOM. Cost if wrong: untested allocator/resource conditions may still fail; ERR007/resource acceptance remains open.

Final: Ruling: Host notification is advisory/best-effort after commit. Boundary injection proves successful publication is not reported as rejection, but does not certify queue-internal flag recovery or actual callback delivery. Cost if wrong: host dirty/display notification can be missed under exceptional memory pressure; persisted bytes and active playback remain coherent.

Final: Ruling: No fresh Windows/DAW, whole-wrapper allocation/race, no-callback lifecycle or broader musical-quality certification. Cost if wrong: platform/host/resource behavior remains unverified; existing external acceptance and queued P2 tasks stay open.

No deferred Minor from this review. Next queue item ERR070 CLI root-exit/inherited-pipe lifecycle; features remain paused.
