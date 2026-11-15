# First assigned P1 remediation batch
User authorized continuing assigned fixes; feature freeze retained. Scope: ERR068 Node atomic export, ERR067 safe model parse errors, ERR010 native lifecycle cleanup. Disjoint owners root/export, audit_cli_security/schema, audit_native_safety/Processor. ERR008 remains next sequential native task. No new feature/state schema/dependency.

1. Reproduce each current defect with real consumer-facing tests before production changes.
2. Export: encode/validate first; unique sibling created exclusively, write/flush/close checked before atomic rename, remove only owned staging on failure, preserve old bytes and report captured byte count. Test partial write, flush/close/rename failures and retry; Linux real RLIMIT where available.
3. Redaction: fixed safe generated JSON diagnostic; malformed API/CLI contents never echoed in MCP/legacyHTTP/v1, preserve safe actionable validation errors.
4. Lifecycle: preserve MIDI ownership across lifecycle timing resets, drain once on next nonempty block before new events; zero-length callbacks keep debt, both modes32/64 samples and allocation guards. No callback IO/locks/allocations. No promise of destruction delivery without callback.
5. Full relevant suites/sample, independent fresh whole-batch review and one RED→GREEN fix pass. Registry/task updated only from passing evidence. Publish existing draft PR; no merge/release/platform claims.
