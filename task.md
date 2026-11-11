# Development tasks

Derived from CODEX_TASK.md because task.md did not exist. Preserve AGENTS.md rules. Check items only against concrete evidence.

- [x] Inspect prototype and document prioritized risks: docs/V0.2-AUDIT.md.
- [x] Fix confirmed upstream HTTP-body/CLI-stderr disclosure and rounding overflow: commit dd4762f, regression tests.
- [x] Bound successful HTTP provider responses to 4 MiB and cancel oversized streams; sanitize transport failures: tests/core.test.js, npm test passed 18/18.
- [x] Test success-format normalization and partial fallback for OpenAI, Anthropic, OpenRouter and Ollama using mocked transports: npm test passed.
- [x] Add hostile input checks for unknown provider, malformed composition and non-finite note data: npm test passed.
- [x] Document current composition/provider contract: docs/PROVIDER-CONTRACT.md, checked against src/schema.js, src/operations.js and src/ai.js.
- [x] Write native architecture RFC before native implementation: docs/rfcs/0001-native-plugin.md. This marks documentation only, not implementation or host validation.
- [x] Verify HTTP Host/Origin/content-type boundaries, malformed JSON, path traversal rejection and offline generation: real local HTTP integration test passes.
- [x] Oversized HTTP requests return 413 and recover; 100 requests in batches of 20 pass (tests/stress.test.js).
- [x] Bound MCP stdin to 1 MiB, limit eight concurrent handlers, recover from null/invalid/oversized messages in child-process tests.
- [ ] Test provider deadlines, rate limits, cancellation and CLI process-tree termination. Windows CLI tests require a Windows runner; Linux fake-CLI failure test passes.
- [x] Independently decode MIDI for 200 generated compositions and compare notes, channels and timing.
- [x] Add injectable provider transport, AbortSignal support and HTTP disconnect cancellation; offline tests remain intact.
- [x] Compile Linux JUCE/CMake VST3 and Standalone after RFC: /workspace/loopforge-build; scheduler and processor CTest 2/2 passed. Actual VST3 module factory loaded by JUCE manifest helper.
- [x] Verify native worker -> loopback API -> eight offline ideas: processor_test plugin/tests/idea.json --api passed with npm start running.
- [x] Add Windows/Linux CI build and unsigned artifact upload configuration: .github/workflows/native.yml. Remote CI execution still unverified.
- [x] Profile 1600-note processing at blocks 32..2048; replace full event scan with binary lookup, compare against reference on 10000 positions.
- [x] Stress 500 concurrent state replacements against 3000 audio blocks; guard nonfinite host position and bound audition output.
- [x] Test native worker against HTTP 429, invalid composition, oversized response, stalled request shutdown and recovery.
- [ ] Validate scheduling in real DAWs; Linux synthetic stress tests do not establish host compatibility.
- [ ] Validate Windows compilation and native UI/audio on actual target hardware.
- [ ] Build Windows artifact and open PR. Last attempt blocked externally: GitHub API returned Forbidden and gh reported invalid authentication. Recheck after relevant access changes; never request credentials in chat.
- [ ] Validate native plugin in REAPER. Requires compiled plugin and an actual host; not yet executable.

Latest validation: npm test: 26 passed, 0 failed; npm run sample succeeded. Live provider/DAW/browser audio checks not performed. Existing untracked samples/loopforge-demo files were preserved and are not part of the code change.

Current external blockers: api.github.com, api.openai.com, api.anthropic.com and openrouter.ai are rejected by the proxy (403/Forbidden). Cloud API key variable names were checked and are missing; Ollama port 11434 is not listening. No live paid calls or credentials were used. Local CMake 4.4.4, Ninja and JUCE 8.0.8 dependencies are installed and Linux builds work; the previous missing-toolchain blocker is resolved.
