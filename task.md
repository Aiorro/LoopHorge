# task.md - LoopForge AI MIDI execution checklist

**This is the only authoritative TODO list.** Read [AGENTS.md](AGENTS.md) for the mandatory workflow, tools, security, completion rules and engineering conventions. Do not restate those instructions here.

**Project:** LoopForge AI MIDI | **Repository:** `Aiorro/LoopHorge` | **Target:** reliable offline + optional AI composition, native JUCE VST3 and Standalone.

## How to work this list

- Execute tasks in order within the earliest incomplete milestone, unless blocked by a documented dependency.
- **Change `[ ]` to `[x]` only when the stated acceptance evidence exists.** Include actual test commands/results, commit, CI or artifact links next to the completed item.
- Keep failed, blocked, unfinished or merely attempted steps unchecked; record blockers below and continue independent work.
- This checklist reflects the **remote `main`** state when first created. Earlier Codex reported local `codex/loopforge-v0.2` / `dd4762f`, 13 local tests, and `docs/V0.2-AUDIT.md`; those have not yet been verified in the remote repo. Do not assume that work is lost or already merged.
- Scope changes and new issues are added to this file, not to an independent TODO document.

## M0 - Reconcile Codex work with the repository

- [x] **T01: Recover and preserve v0.2 work.** Check local working tree, branch and commit `dd4762f`; compare with remote `main`. Locate `docs/V0.2-AUDIT.md`, the security fixes, MIDI boundary fix and new tests. Do not overwrite local work. **Done:** verified diff and recovery/merge/rebase decision documented here.
- [x] **T02: Reconcile and publish verified fixes.** Retain the sanitized AI-provider/CLI error behavior and corrected note-end quantization if valid; add/keep regression tests. Run `npm test` and `npm run sample`. Push to an accessible branch if authorized; otherwise retain local commits and document push blocker. **Done:** tests pass and exact commit/test evidence is recorded; remote push state explicitly recorded.
- [ ] **T03: Audit remaining protocol contracts.** Review `src/schema.js`, `src/ai.js`, `src/agent-cli.js`, `src/mcp-server.js` and tests for bad JSON, overlong notes, oversized arrays, invalid providers, path traversal, CLI failures and error redaction. **Done:** focused tests cover confirmed risks and new changes pass CI or local tests.

## M1 - Native JUCE foundation

- [ ] **T04: Write minimal native architecture RFC.** Add `docs/JUCE-RFC.md`: JUCE target types, processBlock/host MIDI output, asynchronous service bridge, lifetime/thread ownership, preset/state persistence, build dependencies and threat model. **Done:** concrete interfaces, dependency versions and acceptance checks documented; no fake implementation claims.
- [ ] **T05: Create reproducible JUCE/CMake project.** Add `plugin/` C++20 source, `CMakeLists.txt`, suitable presets/toolchain setup and pinned JUCE version. Set targets for VST3 and Standalone (Windows x64 first), ensure no proprietary assets. **Done:** configure/build works in a real verified environment and log is recorded. If unavailable locally, leave unchecked and advance independent CI/configuration work.
- [ ] **T06: Implement minimal functioning audio plugin.** Proper JUCE AudioProcessor + editor, host tempo/transport query, safe basic MIDI passthrough/output and state serialization. No network or AI calls on realtime thread. **Done:** compiled plugin artifact and processor test/smoke evidence, not only source files.
- [ ] **T07: Configure native CI.** Windows x64 GitHub Actions job with pinned CMake, compiler and JUCE setup; build VST3 + Standalone and upload clearly marked unsigned artifacts. **Done:** link to an actually green native workflow and downloadable artifacts. Missing GitHub auth does not justify fake success.

## M2 - Real musical functionality inside DAW

- [ ] **T08: Stable composition + IPC contract.** Versioned JSON schema for notes/tracks, BPM, time signature, bars, key/scale and error responses; mock bridge tests, cancellation, timeout, reconnect and size bounds. **Done:** tests demonstrate successful and failed handoff.
- [ ] **T09: Connect local offline engine to JUCE.** Native app requests variations on a non-RT worker, receives validated note data, renders MIDI in host-aligned timeline and remains usable offline. **Done:** deterministic smoke demonstration and no audio-thread-blocking regressions.
- [ ] **T10: Implement native idea browser and piano roll MVP.** Select among 8 variations; inspect bass/chords/melody, play/stop, edit notes, quantize, undo/redo and export MIDI type 1. **Done:** functional UI with documented manual steps and passing targeted tests.
- [ ] **T11: AI provider switching + safe model use.** OpenAI, Anthropic, OpenRouter, Ollama, supported authenticated CLI bridge and optional MCP host tools where permitted. Model output validated before MIDI conversion; secrets never committed or exposed. **Done:** mocked provider tests for all adapters and at least one opt-in live integration check clearly labeled as such (live check remains optional if credentials are unavailable).
- [ ] **T12: MIDI drag/drop and DAW acceptance.** Drag generated MIDI to DAW where supported; verify VST3 loads in REAPER on Windows and host clock/MIDI export behave correctly. **Done:** recorded manual host validation, including DAW version; mark untested hosts explicitly.

## M3 - Release readiness

- [ ] **T13: Quality and safety review.** Run Node tests, JUCE tests, pluginval (if available), static analysis and stress checks for UI/AI boundary, plugin reload/state persistence and realtime safety. **Done:** reproducible logs and severity-ranked remaining issues.
- [ ] **T14: Ship a testable pre-release.** Document installation, unsigned binary warning, AI configuration, limitations and license dependencies; publish a downloadable Windows build artifact/release when verified. **Done:** actual release URL, SHA256 and installation smoke check.
- [ ] **T15: Close v0.2 milestone.** Consolidate test/build/CI evidence and known limitations into changelog. **Done:** checklist contains no undocumented claims; milestone only marked finished if T01-T14 dependencies and explicitly scoped acceptance items are met.

## Blockers and evidence (update in the same commit as progress)

| Task | Blocker or evidence | Next action | Status |
| --- | --- | --- | --- |
| T01/T02 | Prior Codex session reported local branch `codex/loopforge-v0.2`, commit `dd4762f`, and 13 tests; remote visibility not verified. | Inspect local git history and synchronize without overwriting. | Unverified |
| T05-T07 | Prior Codex environment reportedly lacked CMake/JUCE build tooling. | Check installed tools; add pinned Windows CI, then verify build output. | Unverified |
| T02/T07 | Prior Codex `gh`/GitHub API returned authentication/Forbidden errors. | Check current permissions; preserve local work and request access only if still blocked. | Unverified |

## Current execution pointer

**Active: T02/T03, T08/T09/T11/T13 remediation requested on 2026-11-11.** Apply the rules from [AGENTS.md](AGENTS.md). Keep this file updated continuously as tasks are actually verified.

## Current remediation design and execution (2026-11-11)

User requested code fixes for the five review findings. Preserve all 15 task IDs and this checklist as the sole task source. Work in the existing dedicated Codex branch; preserve untracked sample files.

T01 evidence: fetched origin/main at 28a4f91; HEAD 15e2e66 has four unique commits on each side. Merge main into codex/loopforge-v0.2, retain main AGENTS.md/CODEX_TASK.md and task structure, transfer only verified evidence. Baseline: npm test 26/26, Linux CMake build and CTest 3/3.

Implementation order and acceptance:
1. T02/T03: finish the merge, retain provider redaction and MIDI quantization tests, record verification and local-only publication state.
2. T08/T13: regress overlapping/repeated identical notes, stop/seek/loop/state replacement, tempo changes at 32/64 samples. Coalesce same-channel/pitch overlaps to a single MIDI voice with counted lifetimes, retain audition until final release. Use owned preallocated output storage swapped into the host buffer; cap extreme block event production and flush on recovery. Test allocations on first and later audio calls with alternating fresh host buffers, including dense schedules.
3. T09: native deterministic offline generator, eight selectable ideas with key/mode/style/bars/BPM/seed; no process startup or service dependency. Keep Node bridge for optional AI. Test generation without port 4177, valid bounds, determinism, variation and exportable state.
4. T08/T11: native model text field, provider readiness/probe, sanitized actionable auth/rate-limit diagnostics, cancel UI and worker cancellation. Forward AbortSignal and model through CLI, use bounded timeout and output, terminate POSIX process groups / Windows taskkill trees. Test fake CLI descendants on abort, timeout and overflow, HTTP disconnect propagation and recovery.
5. T13: full Node and native builds/tests, UI smoke, sample generation without overwriting user samples, independent final review. Actual Windows/DAW/live paid provider checks remain unverified unless executed.

Architecture choices: keep JUCE/Node and existing three-slot schedule handoff. Native offline rules avoid background service installation/lifecycle risks; optional AI still requires an explicitly started loopback service. Provider probe is explicit and may make a small billable request; UI must disclose that. No package/model downloads or release publication.

T02 evidence: merge retains main task management and all v0.2 code; npm test 26/26; npm run sample passed and existing sample bytes are identical. Local merge commit only; no push requested.
