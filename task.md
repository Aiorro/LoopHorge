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
- [x] **T03: Audit remaining protocol contracts.** Review `src/schema.js`, `src/ai.js`, `src/agent-cli.js`, `src/mcp-server.js` and tests for bad JSON, overlong notes, oversized arrays, invalid providers, path traversal, CLI failures and error redaction. **Done:** focused tests cover confirmed risks and new changes pass CI or local tests.

## M1 - Native JUCE foundation

- [x] **T04: Write minimal native architecture RFC.** Retain and update the canonical `docs/rfcs/0001-native-plugin.md` (the existing v0.2 RFC): JUCE target types, processBlock/host MIDI output, asynchronous service bridge, lifetime/thread ownership, preset/state persistence, build dependencies and threat model. **Done:** concrete interfaces, dependency versions and acceptance checks documented; no fake implementation claims.
- [x] **T05: Create reproducible JUCE/CMake project.** Add `plugin/` C++20 source, `CMakeLists.txt`, suitable presets/toolchain setup and pinned JUCE version. Set targets for VST3 and Standalone (Windows x64 first), ensure no proprietary assets. **Done:** configure/build works in a real verified environment and log is recorded. If unavailable locally, leave unchecked and advance independent CI/configuration work.
- [x] **T06: Implement minimal functioning audio plugin.** Proper JUCE AudioProcessor + editor, host tempo/transport query, safe basic MIDI passthrough/output and state serialization. No network or AI calls on realtime thread. **Done:** compiled plugin artifact and processor test/smoke evidence, not only source files.
- [x] **T07: Configure native CI.** Windows x64 GitHub Actions job with pinned CMake, compiler and JUCE setup; build VST3 + Standalone and upload clearly marked unsigned artifacts. **Done:** link to an actually green native workflow and downloadable artifacts. Missing GitHub auth does not justify fake success.

## M2 - Real musical functionality inside DAW

- [x] **T08: Stable composition + IPC contract.** Versioned JSON schema for notes/tracks, BPM, time signature, bars, key/scale and error responses; mock bridge tests, cancellation, timeout, reconnect and size bounds. **Done:** tests demonstrate successful and failed handoff.
- [x] **T09: Offline generation in JUCE with optional Node bridge.** Native app generates eight variations directly on its non-RT worker, validates them, renders host-aligned MIDI and remains usable without Node. Preserve the optional Node AI bridge. Scope updated on explicit user request to remove installation-time service dependence. **Done:** deterministic smoke demonstration and no audio-thread-blocking regressions.
- [ ] **T10: Implement native idea browser and piano roll MVP.** Select among 8 variations; inspect bass/chords/melody, play/stop, edit notes, quantize, undo/redo and export MIDI type 1. **Done:** functional UI with documented manual steps and passing targeted tests.
- [x] **T11: AI provider switching + safe model use.** OpenAI, Anthropic, OpenRouter, Ollama, supported authenticated CLI bridge and optional MCP host tools where permitted. Model output validated before MIDI conversion; secrets never committed or exposed. **Done:** mocked provider tests for all adapters and at least one opt-in live integration check clearly labeled as such (live check remains optional if credentials are unavailable).
- [ ] **T12: MIDI drag/drop and DAW acceptance.** Drag generated MIDI to DAW where supported; verify VST3 loads in REAPER on Windows and host clock/MIDI export behave correctly. **Done:** recorded manual host validation, including DAW version; mark untested hosts explicitly.

## M3 - Release readiness

- [ ] **T13: Quality and safety review.** Run Node tests, JUCE tests, pluginval (if available), static analysis and stress checks for UI/AI boundary, plugin reload/state persistence and realtime safety. **Done:** reproducible logs and severity-ranked remaining issues.
- [ ] **T14: Ship a testable pre-release.** Document installation, unsigned binary warning, AI configuration, limitations and license dependencies; publish a downloadable Windows build artifact/release when verified. **Done:** actual release URL, SHA256 and installation smoke check.
- [ ] **T15: Close v0.2 milestone.** Consolidate test/build/CI evidence and known limitations into changelog. **Done:** checklist contains no undocumented claims; milestone only marked finished if T01-T14 dependencies and explicitly scoped acceptance items are met.

## Blockers and evidence (update in the same commit as progress)

| Task | Blocker or evidence | Next action | Status |
| --- | --- | --- | --- |
| T01/T02 | Main 28a4f91 reconciled with v0.2 15e2e66 in local merge 3e46129; all native/security work retained. | Review/push the local branch when desired; no remote publication requested in this turn. | Locally verified |
| T03/T11 | Node 36/36 on Linux, all four HTTP provider mocks and both CLI adapters; abort/timeout/overflow/root-exit/HTTP-disconnect tree cleanup. | Run the same lifecycle suite on Windows 10+; paid/provider checks need opt-in credentials. | Linux verified; Windows/live pending |
| T04-T06/T09 | Linux CMake build produced VST3 and Standalone; CTest 7/7. Native eight-idea offline generator, state/transport/UI and actual JUCE MIDI conversion passed. Eight normalized seeded ideas match Node output. | Observe Windows native CI and validate actual host behavior. | Linux verified |
| T12-T15 | Windows/native CI is verified at 902b191; actual DAW, pluginval and release validation remain unavailable. Native editing/undo/drag-to-DAW remain separate acceptance work. | Observe CI artifacts; run host/pluginval checks before release claims. | Unverified |
| T08 | Draft2020-12 schema and additive v1 routes implemented; generation/probe/error/cancel/deadline/reconnect/version/framing checks passed over real sockets. | Observe CI for this implementation commit. | Locally verified |

## Current execution pointer

**Next: T10 native editing. T07 and T08 now have recorded evidence below.** The requested remediation is implemented and verified locally; T13 retains platform/host acceptance gaps. Apply the rules from [AGENTS.md](AGENTS.md). Keep this file updated continuously as tasks are actually verified.

## Current remediation design and execution (2026-11-11)

User requested code fixes for the five review findings. Preserve all 15 task IDs and this checklist as the sole task source. Work in the existing dedicated Codex branch; preserve untracked sample files.

T01 evidence: fetched origin/main at 28a4f91; HEAD 15e2e66 has four unique commits on each side. Merge main into codex/loopforge-v0.2, retain main AGENTS.md/CODEX_TASK.md and task structure, transfer only verified evidence. Baseline: npm test 26/26, Linux CMake build and CTest 3/3.

Implementation order and acceptance:
1. T02/T03: finish the merge, retain provider redaction and MIDI quantization tests, record verification and local-only publication state.
2. T08/T13: regress overlapping/repeated identical notes, stop/seek/loop/state replacement, tempo changes at 32/64 samples. Coalesce same-channel/pitch overlaps to a single MIDI voice with counted lifetimes, retain audition until final release. Reserve MIDI output in JUCE wrapper/device preparation; cap extreme block event production and flush on recovery. Test allocations on first and later audio calls with alternating fresh host buffers, including dense schedules.
3. T09: native deterministic offline generator, eight selectable ideas with key/mode/style/bars/BPM/seed; no process startup or service dependency. Keep Node bridge for optional AI. Test generation without port 4177, valid bounds, determinism, variation and exportable state.
4. T08/T11: native model text field, provider readiness/probe, sanitized actionable auth/rate-limit diagnostics, cancel UI and worker cancellation. Forward AbortSignal and model through CLI, use bounded timeout and output, terminate POSIX process groups / Windows 10+ Job Objects with atomic process-creation ownership. Test fake CLI descendants on abort, timeout and overflow, HTTP disconnect propagation and recovery.
5. T13: full Node and native builds/tests, UI smoke, sample generation without overwriting user samples, independent final review. Actual Windows/DAW/live paid provider checks remain unverified unless executed.

Architecture choices: keep JUCE/Node and existing three-slot schedule handoff. Native offline rules avoid background service installation/lifecycle risks; optional AI still requires an explicitly started loopback service. Provider probe is explicit and may make a small billable request; UI must disclose that. No package/model downloads or release publication.

T02 evidence: merge retains main task management and all v0.2 code; npm test 26/26; npm run sample passed and existing sample bytes are identical. Local merge commit only; no push requested.

Implementation ruling: a raw AudioProcessor cannot preallocate a new caller-owned MidiBuffer before it receives it. Instead of transferring owned storage (which fails when hosts alternate fresh buffers), the build prepares each target wrapper's own MIDI buffer outside callbacks. Custom processor callers must reserve the documented 32 KiB. MIDI event production is bounded, with note-off recovery on oversized blocks. This changes preparation in generated build-local copies of pinned JUCE, never in the upstream checkout. Whole-wrapper/DAW allocations remain a separate host validation requirement.

Final review findings and resolution: preserve Steinberg/JUCE's 2048-event output limit and reserve room for final Note Offs (an actual MidiEventList conversion test must deliver every emitted event); retain the preceding sample interval across tempo changes (literal .00095/.00195-beat boundary regressions). Replace Windows taskkill with a kill-on-close Job Object assigned atomically via STARTUPINFOEX job-list attributes; this removes both vanished-root and create-then-assign cancellation races. Windows source review is not execution evidence. POSIX process-group tests do not cover a deliberately escaping setsid descendant.

Final verification (2026-11-11): `npm test` 36 passed, 0 failed; `npm run sample` passed with byte-identical existing JSON/MIDI samples; `/workspace/native-tools/bin/cmake --build /workspace/loopforge-build --parallel 2` passed for Linux VST3 and Standalone; CTest 7/7; `node scripts/verify-native-api.cjs /workspace/loopforge-build/processor_test` passed including custom model/probe/current-state retention; `node scripts/verify-native-failures.cjs /workspace/loopforge-build/processor_test` passed HTTP 429, invalid composition, oversized response, user cancellation/recovery and stalled shutdown. Native editor snapshot/handlers and eight-idea native/Node parity checked. Independent source review findings fixed with boundary and converter regressions. Windows Job Object code received static review; the create/assign race was removed with atomic job-list attributes, but Windows compile/runtime, full DAW/driver allocation behavior and real paid provider sessions remain unverified. No release or remote push performed.

Final caller review preserved the browser contract where an empty model field selects the provider default; a probe regression reproduced the rejection before normalizing blank to absent. No implementation changes followed the final native checks.

## Continuation on 2026-11-12

T07: GitHub connector access and repository push permissions now work. Baseline Node 36/36 and CTest 7/7 repeated. Branch 902b191 pushed without rewriting history; draft PR https://github.com/Aiorro/LoopHorge/pull/2 opened to run and inspect Windows/native CI. Do not mark T07 complete without green Windows/native jobs and actual artifacts.

T08 design/execution: introduce additive `/api/v1/generate` and `/api/v1/probe` endpoints with protocolVersion=1 success/error envelopes and a Draft 2020-12 JSON schema. Preserve legacy browser/MCP behavior. Wire compositions carry explicit 4/4 timeSignature, existing three layers and bounded normalized notes; runtime also checks aggregate count and note ends (cross-field sums are outside basic JSON Schema). Reject wrong versions and malformed metadata/all ideas without replacing the native composition. Native bridge uses v1, validates HTTP length/size/framing and all returned variations, handles stable sanitized error codes, cancellation/deadline and reconnect. Use a constructor-injected bounded request deadline for a real stalled-server timeout test. Acceptance: strict/legacy HTTP tests, schema validation, actual worker socket success, malformed/unsupported version/partial response/cancel/timeout/recovery checks, full Node/native suites. Record evidence here in the implementation commit.

T07 evidence (2026-11-12): PR https://github.com/Aiorro/LoopHorge/pull/2; native workflow https://github.com/Aiorro/LoopHorge/actions/runs/43163198730 succeeded on Windows and Ubuntu for head 902b191 (PR merge build da51a3f). Windows artifact https://github.com/Aiorro/LoopHorge/actions/runs/43163198730/artifacts/13919655614, SHA256 c115519f00f60c46122eca773d67382eba5ab87d415b1e7db12f94f071bbf040; Linux artifact ID13919132528. Prototype workflow https://github.com/Aiorro/LoopHorge/actions/runs/43163198665 succeeded on Node20/22 across Windows/macOS/Linux, including actual Windows Job Object lifecycle tests. This establishes CI compilation/tests, not DAW acceptance. Later commits must receive their own CI evidence.

T10 next slice design: retain current three-layer composition and snapshot handoff. A scrollable native piano roll supports active layer, double-click add, drag move, right-edge duration resize, Alt/right-click delete, quarter-beat quantization, undo/redo with bounded per-selected-idea history. Processor edits validate and publish immutable snapshots outside audio; generation, restoration and selection clear edit history. A revision token rejects stale gestures after concurrent replacement. Extract MIDI byte export into a shared native function, preserve type1/PPQ480/separate channels, explicit 4/4 and tempo, and at least one tick per note. Keep the existing one-note minimum and 800/layer,1600/idea limits. Acceptance: real edit/undo/redo/boundary/state/export tests and UI gesture/handler snapshot smoke, Node/native suites and updated CI. No instrument/sample library or DAW automation added.

T08 completion evidence (2026-11-12): Node42/42, Linux VST3/Standalone build, CTest7/7, native API and expanded failure/reconnect scripts all passed. Python jsonschema independently validated the schema and real generation/probe/error outputs. Independent review found incomplete envelope checks, JSON trailing-data acceptance and inconsistent provider deadline codes; regression tests reproduced each and the fixes passed. A schema-integer representation regression87.0/42.0 and subnormal end-boundary scheduler regression also passed after fixes. Legacy browser/MCP routes and version1 saved-state compatibility retained. Implementation and this checklist committed together; Windows CI for the new commit remains to be observed.
