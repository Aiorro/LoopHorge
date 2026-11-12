# Native prototype validation

## Reproduce

```sh
npm test
npm run sample
cmake -S plugin -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --config Release --parallel 4
ctest --test-dir build/native -C Release --output-on-failure
node scripts/verify-native-api.cjs build/native/processor_test
node scripts/verify-native-failures.cjs build/native/processor_test
```

On Windows use build/native/Release/processor_test.exe in the final two commands. Port 4177 must be free: the scripts start and stop only their own servers. Windows execution is covered by the commit-specific CI evidence below. CI runs these checks and uploads unsigned artifacts.

Optional scheduler memory/undefined-behavior check on GCC/Clang:

```sh
c++ -std=c++20 -fsanitize=address,undefined -fno-omit-frame-pointer -g plugin/tests/SchedulerTest.cpp -o /tmp/scheduler-sanitized
/tmp/scheduler-sanitized
```

## Executed coverage

- Node test suite: core regression tests plus malformed MCP values/oversized messages, a 100-request MCP burst with bounded concurrent handlers, HTTP 413/recovery, 100 HTTP generation requests in batches of 20, 1000 seeded compositions, independent MIDI decoding for 200 compositions, cancellation/429 handling, malformed provider envelopes and Unicode prompts split across network chunks.
- Native scheduler: capacity, note bounds, equal-time note-off ordering; optimized event lookup compared with a reference scan at 10000 positions. AddressSanitizer and UndefinedBehaviorSanitizer passed for this scheduler test. This is not whole-plugin sanitizer coverage.
- Native processor: MIDI output, stop note-offs, invalid host PPQ, valid/invalid state restoration. Worker success, failure and recovery through actual loopback sockets. Failed API responses retain the previous composition. Stalled-server shutdown completes within the test's two-second bound.
- Native stress: 1600 notes at 48 kHz, block sizes 32, 64, 128, 512 and 2048; 300 blocks per size; 500 concurrent state replacements while processing 3000 audio blocks. Finite samples and MIDI offsets are checked. A dense maximum-velocity audition chord must remain within the output range.

## Performance observations

On this Linux cloud machine, the original 1600-note scan averaged about 0.35 ms per 64-sample block. Binary event lookup and cached oscillator increments reduced the average to about 0.02–0.03 ms. One comparison run measured p99 of 0.061 ms against a 1.333 ms block duration. The old algorithm had an observed 6 ms outlier; OS scheduling and these synthetic runs cannot guarantee hard real-time deadlines on other machines.

Dense full-range maximum-velocity audition exposed a peak of 2.6233 before limiting. A soft output limiter was added; MIDI note values remain unchanged. The audition oscillator still has no production envelope or instrument-quality sound design.

## Resource and connection inventory

The repository contains the CMake targets, pinned JUCE commit, native source, test fixture, browser assets, stdio MCP server, HTTP AI adapters, REAPER import script, CI and build instructions. Node requires no third-party packages. JUCE is downloaded from the official repository at d6181bde38d858c283c3b7bf699ce6340c050b5d (8.0.8), or supplied with LOOPFORGE_JUCE_PATH. Compiler, CMake and Linux development libraries are prerequisites, not vendored application assets. Build outputs are intentionally outside tracked source.

The current machine has CMake 4.4.4 and Ninja under /workspace/native-tools/bin, JUCE at /workspace/JUCE, development headers/libraries extracted under /workspace/native-sysroot, and built artifacts at /workspace/loopforge-build. Shared-library inspection found no missing runtime dependency in the Linux VST3. Normal machines should use the documented OS package installation, rather than copying this temporary sysroot.

Git read/push access and the connected GitHub API now work; PR2 and actual CI results are recorded below. Earlier unauthenticated probes to api.openai.com, api.anthropic.com and openrouter.ai failed at the proxy CONNECT stage with 403; this is a network policy blocker, not a provider authentication result. Corresponding API-key variable names were checked without reading values and are missing. Local Ollama port 11434 is not listening. Live-provider network access would require api.openai.com, api.anthropic.com and openrouter.ai. Credentials belong in environment settings, never chat or repository files.

## Outstanding acceptance

A production-ready claim remains blocked by actual DAW transport/audio/MIDI behavior, live AI providers, whole-plugin analysis and long-duration leak/profiling tests. Windows builds and CLI process-tree/deadline behavior have commit-specific CI evidence below. Native editing, undo and drag-to-DAW are not implemented. These checks cover a defined stress matrix, not every host/device/plugin combination. See task.md for remaining work.

## Review remediation (2026-11-11)

The main branch task-management contract and its 15 task IDs were merged into the native branch without dropping native/security work (merge 3e46129). Untracked demo sample bytes were preserved.

New reproducible coverage: `note_lifetime_test` (overlap/retrigger, tempo changes at 32/64, stop/seek/loop/state replacement); `offline_test` (eight native variations, deterministic seed, scale/bounds, every supported style/mode); `realtime_test` (Linux allocator interception, bounded oversized-block output and actual JUCE MidiEventList conversion without discarded events); `ui_test` (native provider/model/probe/cancel controls and offline handlers). Run through CTest. `ui_test /absolute/path.png` also produces an editor snapshot. Native HTTP scripts now use a mocked Ollama transport behind the real Node listener, since offline generation intentionally bypasses the service; no paid provider is called.

Node CLI tests launch real fake executables and resistant descendant processes, check model arguments, abort/timeout/output overflow and HTTP disconnect cancellation, and validate Claude's envelope. Provider probing generates one idea on explicit request, preserves the native current composition, and reports sanitized authorization/model/rate-limit failures. Streaming-response cancellation is distinct from transport failure.

Allocation evidence applies to Processor callbacks with 32 KiB of caller-owned MIDI storage. Build-local JUCE preparation changes provide that storage in the shipped target wrappers; the direct allocation test does not itself load the VST3 wrapper or certify a DAW/driver. See plugin/README.md for the output budget and custom-caller contract. No hard real-time guarantee follows from these synthetic tests.

Independent review regressions reproduced before fixes: .00095-beat onset repeated after BPM90→180; JUCE VST3 discarded releases from a 1600-note dense repeat fixture. Fixes retain the preceding sample interval and cap emitted events to 2048 with release reserve. Native and Node offline output parity was independently compared for eight seeded D-minor ambient ideas after normalization.

## GitHub CI and bridge v1 (2026-11-12)

PR2: https://github.com/Aiorro/LoopHorge/pull/2. Native workflow43163198730 passed on Windows and Ubuntu for head902b191 / PR merge build da51a3f. It built real VST3/Standalone targets, ran CTest and native HTTP success/failure scripts, then uploaded unsigned artifacts. Windows artifact13919655614 has GitHub-reported SHA256 c115519f00f60c46122eca773d67382eba5ab87d415b1e7db12f94f071bbf040. Prototype workflow43163198665 passed Node20/22 on Windows/macOS/Linux, including actual Job Object CLI lifecycle tests. This supersedes the earlier Windows-compilation/CLI-runtime uncertainty for that commit; no DAW load is implied.

V1 Node tests exercise typed requests, meter/seeded responses, legacy compatibility, error shapes, malformed/oversized input recovery, explicit cancellation, sanitized malformed upstream data and equal timeout codes for headers/body. Draft2020-12 schema validity and generated/probe/error examples were independently checked with Python jsonschema in the cloud environment (not a new project dependency). Native socket tests exercise valid custom-model/probe handoff; missing/wrong versions; missing or mistyped envelope fields; bad source count; nonboolean probe ok; invalid later variations/meter; trailing JSON data; incomplete Content-Length; a real250 ms stalled-server deadline; connection loss and reconnect; cancellation/size/error recovery. A normalized note whose start equals the pattern end, even with a subnormal duration, is rejected. Integral numeric representations such as87.0 are accepted as JSON Schema integers.

## Repeat Superpowers review and fixes (2026-11-12)

Applied official Superpowers7.0.2 `requesting-code-review` and its reviewer template to origin/main..f9d245d with a fresh, read-only reviewer. Four Important findings and one Minor finding were reproduced or confirmed in connected code, then fixed:

- Important: JUCE numeric parsing wrapped oversized pitch/protocol integers before range checks and accepted invalid JSON tokens/escapes. Wire parsing now validates token grammar/UTF-8/nesting, reads finite numbers using std::from_chars and canonicalizes them safely before JUCE conversion. Floating .0 representation is retained for valid saved-state round trips. Big scaled mantissas, uint32 seeds and small finite values have positive tests; malformed tokens, unknown escapes and million-digit literals have negative tests.
- Important: conversion to juce::String truncated at NUL before HTTP framing validation. Content-Length is now checked against original bytes; embedded NUL and invalid UTF-8 cannot hide a suffix. Byte-level socket and saved-state regressions passed.
- Important: a valid duration0.001 at beat0 exported both edges at tick0. Shared native encodeMidi quantizes once and guarantees finish>=start+1, including the final boundary of an8-bar pattern.
- Important: exported files lost trailing rests because their last event defined the endpoint. All four type1 tracks now end explicitly at bars*4*480; tempo and4/4 metadata are included. Actual MIDI bytes are decoded and checked for7680/15360-tick endpoints, PPQ480, track count and one-tick notes.
- Minor: the idea picker remained on IDEA4 after regeneration installed IDEA1. The processor now owns the selected index, and the editor synchronizes it even when the count stays8. Restoring state exposes exactly the restored idea and cancels stale generation. A real message-loop control test reproduced and fixed regeneration/restore mismatch.

Additional TDD regressions: negative-PPQ seeks could leave held notes because lastEnd<0 also represented 'no prior block'; a separate continuity flag fixes this without callback allocations. Boolean saved-state version was accepted through integer coercion; the shared strict numeric version check now rejects it. Old behavior was reproduced before both fixes.

Verified after fixes: Node42/42; Linux VST3/Standalone build; CTest9/9; expanded native socket failure/recovery/cancellation/deadline suite; scheduler with ASan/UBSan; unchanged sample JSON/MIDI bytes. Sanitizer coverage is the scheduler, not the entire plugin. Callback allocation coverage still requires the prepared-MIDI-buffer contract. Actual DAW sessions, pluginval (not installed here), real provider accounts and release installation remain separate acceptance limits. The superseded f9d245d commit passed both native Windows/Linux and the three-OS Node20/22 matrix; the new fix commit receives separate CI checks.

Environment installation: official Superpowers7.0.2 at pinned upstream22f0b463e7a5e579904dd6211a520321035526f7 is installed/enabled in the native Codex CLI; cached skill files hash-match the reviewed source. The separate Codex App Superpowers plugin is confirmed installed/ENABLED (catalog does not expose its version). Pluginval was unavailable; the dedicated static analyzers clang-tidy and cppcheck were also absent. JavaScript syntax checks passed for all16 files in src/scripts/tests, and git diff --check passed. T13 remains open rather than treating source review/scheduler sanitizers as complete whole-plugin analysis.

Final source CI (2026-11-12), head0c6a132 / PR merge48441d646f6a7142dec86fb809dc3704cc45fa34: https://github.com/Aiorro/LoopHorge/actions/runs/43172603984 succeeded for Windows and Ubuntu, including VST3/Standalone builds, CTest8/8 on Windows and9/9 on Linux, and both actual native HTTP scripts. Prototype matrix https://github.com/Aiorro/LoopHorge/actions/runs/43172603983 passed Node20/22 on Windows/macOS/Linux. Windows unsigned artifact https://github.com/Aiorro/LoopHorge/actions/runs/43172603984/artifacts/13922639075 has GitHub-reported SHA2566062e80415eb2827c03952d1dd54f252d2b85a04ccb359daaa8c39b7328c215e; Linux artifact13923022117 has SHA25691e6930e070e88410fea1fcb2a73c25811f9b042ed305aae37c66399925d3d81. These are compiled/tested artifacts, not a host-validated release. A brief cloud runtime interruption occurred after local tests; the files and installed/enabled CLI plugin were rechecked after recovery. This evidence-only follow-up also removes one trailing blank line in Processor.h; it changes no runtime behavior.
