# Native Composer controls: independent review and one fix pass

Reviewed range:6f18a53524779997b544b43082f3734bc39a9283..1859a1955fb7a020adc8dcab137beaecc40c0478. Fresh gpt-6-astra/high reviewer; read-only, independently reran CTest34/34 and built a separate real-editor insertion probe. No Critical or Minor finding. Two Important findings were reproduced; no second review round substituted for regression verification.

1. Seed input character/length filtering silently changed-1→1,1.5→15 and10000000000→1000000000. The actual mode action committed seed1. ERR-057: real insertTextAtCaret tests failed before removing the seed filter. Complete spelling/range now validates before queueing; negative/decimal/alphabetic/11digit/uint32overflow inputs remain visible and do not queue.
2. Direction filtering truncated4110ASCIIbytes to4096characters, removing an unsupported song suffix and generating a prefix. ERR-058: after the seed fix the new direction insertion test still failed; removing the direction filter made it pass. Full ASCII and multibyte oversized input remains visible and requestComposer rejects above4096UTF8bytes before queueing. No truncated request is submitted.

The prior setText-only UI tests bypassed JUCE's insertion filters. Regression coverage now exercises the actual input boundary and production action, not just numeric parsing. MSVC /utf-8 is propagated to Processor consumers because Prompt.h includes RU source literals; fresh Windows compilation remains unverified.

Post-fix full Linux VST3/Standalone build and CTest34/34 passed; Node42/42, sample equality passed. Native protocol fixtures passed before the input-only fix; final scoped sanitizer evidence is recorded in task.md. Snapshot harness replaces PNG contents rather than appending another stream. ERR-056's fixed12px scrollable summary and ASCII separator were visually checked at1000x740/1600x1100.

## Review evidence and scope decisions

The reviewer found no additional concurrency defect: guarded revision/job/mode/cancellation publication and busy clearing under the commit lock prevent late result/cleanup replacement. Exact current Legacy envelope, demo musical source, inactive Composer and host origin are retained. Every Legacy variation/history entry is outside the approved current-document retention contract; this is an explicit scope boundary, not a newly certified feature.

Final: Ruling: Unknown prompt fragments remain warnings under the existing interpreter contract, while recognized unsupported requests reject. Users can inspect warnings; this review does not certify exhaustive RU/EN recognition. Cost if wrong: an unrecognized musical intent may use defaults; broader language acceptance stays open in E03/E06.

Final: Ruling: Fallback seed is classified as default origin; direction/seed controls describe the next request, not loaded-project provenance. Committed notes/maps/seed persist, but no recipe is invented. Cost if wrong: request reconstruction/manual provenance remains unavailable; full recipe UI/persistence stays open.

Final: Ruling: Full recipe persistence/Auto pins/mood controls, song arrangement, Composer editing/history/export/audition remain staged E03/E04 work, with MIDI-only/read-only disclosure. Current-document retention does not retain all Legacy variations/history. Cost if wrong: treating this slice as the full product would lose expected workflows/history; these tasks are not marked complete.

Final: Ruling: No callback code changed; reviewed local tests do not certify whole-wrapper allocation/race freedom, aggregate256MiB/peak memory, Windows/macOS, DAW/pluginval/OS chooser/drag or final teardown MIDI delivery. Cost if wrong: unverified resource/platform/host failures remain possible; ERR-006/007/009/010 and E06 remain open.

Final: Ruling: Severe-OOM transactional recovery remains best-effort under ERR-008, separate from ordinary input validation, which is fixed and tested. Cost if wrong: allocation exhaustion can leave partial control state; no unconditional transactional OOM guarantee.

The reviewer did not independently rerun Node/sample, protocol fixtures or sanitizers; those are implementer-run evidence with explicit instrumentation boundaries. Existing filesystem/slow-storage/crash-recovery behavior and unrelated repository code were not re-audited. No deferred Minor finding in this review. Existing ERR-003 meter-coverage Minor remains unchanged.
