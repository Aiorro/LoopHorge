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

On Windows use build/native/Release/processor_test.exe in the final two commands. Port 4177 must be free: the scripts start and stop only their own servers. Windows execution remains unverified. CI runs these checks and uploads unsigned artifacts.

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

Git read/push access works. GitHub API access for CI inspection/PR creation still returns Forbidden. Fresh unauthenticated probes to api.openai.com, api.anthropic.com and openrouter.ai fail at the proxy CONNECT stage with 403; this is a network policy blocker, not a provider authentication result. Corresponding API-key variable names were checked without reading values and are missing. Local Ollama port 11434 is not listening. Required domain additions: api.github.com, api.openai.com, api.anthropic.com, openrouter.ai. Credentials belong in environment settings, never chat or repository files.

## Outstanding acceptance

A production-ready claim remains blocked by unverified Windows builds, actual DAW transport/audio/MIDI behavior, live AI providers, Windows CLI process-tree/deadline behavior and long-duration leak/profiling tests. Native editing, undo and drag-to-DAW are not implemented. These checks cover a defined stress matrix, not every host/device/plugin combination. See task.md for remaining work.

## Review remediation (2026-11-11)

The main branch task-management contract and its 15 task IDs were merged into the native branch without dropping native/security work (merge 3e46129). Untracked demo sample bytes were preserved.

New reproducible coverage: `note_lifetime_test` (overlap/retrigger, tempo changes at 32/64, stop/seek/loop/state replacement); `offline_test` (eight native variations, deterministic seed, scale/bounds, every supported style/mode); `realtime_test` (Linux allocator interception, bounded oversized-block output and actual JUCE MidiEventList conversion without discarded events); `ui_test` (native provider/model/probe/cancel controls and offline handlers). Run through CTest. `ui_test /absolute/path.png` also produces an editor snapshot. Native HTTP scripts now use a mocked Ollama transport behind the real Node listener, since offline generation intentionally bypasses the service; no paid provider is called.

Node CLI tests launch real fake executables and resistant descendant processes, check model arguments, abort/timeout/output overflow and HTTP disconnect cancellation, and validate Claude's envelope. Provider probing generates one idea on explicit request, preserves the native current composition, and reports sanitized authorization/model/rate-limit failures. Streaming-response cancellation is distinct from transport failure.

Allocation evidence applies to Processor callbacks with 32 KiB of caller-owned MIDI storage. Build-local JUCE preparation changes provide that storage in the shipped target wrappers; the direct allocation test does not itself load the VST3 wrapper or certify a DAW/driver. See plugin/README.md for the output budget and custom-caller contract. No hard real-time guarantee follows from these synthetic tests.

Independent review regressions reproduced before fixes: .00095-beat onset repeated after BPM90→180; JUCE VST3 discarded releases from a 1600-note dense repeat fixture. Fixes retain the preceding sample interval and cap emitted events to 2048 with release reserve. Native and Node offline output parity was independently compared for eight seeded D-minor ambient ideas after normalization.
