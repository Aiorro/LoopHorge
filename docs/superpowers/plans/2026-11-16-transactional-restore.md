# ERR008 transactional native restore

User continues assigned P1 work; no new feature/design questions. Existing queue acceptance defines the behavior. Native implementation owner audit_native_safety, root verification/docs, one fresh final review. Basef2d3839.

1. Add a real Processor one-shot current-thread allocation sweep. Reproduce rejection after state/revision/mode changes without matching MIDI publication. Cover Legacy→Composer, Composer→Composer and legacy restore/recovery; state bytes/revision/mode/history/rendered source must stay unchanged on rejected operations and slots remain reusable.
2. Precompute all allocating status/candidate material before reserving/mutating. Keep state-to-playback publication nonthrowing, release reservations on precommit failures and prevent postpublication notification failures from being reported as a rejected transaction. Apply to native generated/selected/host restored v2 paths and connected legacy path if necessary. No audio callback allocation/locking/change to model/state schema.
3. Full Linux build/CTest, Node/sample equality, targeted sanitizer verification; one fresh independent review and one RED→GREEN fix pass if needed. Document allocator/host notification boundaries accurately, then update errors/task/PR. Other assigned defects remain open and features paused.
