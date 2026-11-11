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
- [ ] Complete oversized HTTP request integration tests.
- [ ] Bound MCP stdin and test malformed protocol requests in a child process.
- [ ] Test provider deadlines, rate limits, cancellation and CLI process-tree termination. Windows CLI tests require a Windows runner; Linux fake-CLI failure test passes.
- [ ] Add independent MIDI parse/round-trip tests.
- [ ] Implement mockable provider transport and caller cancellation; retain offline behavior.
- [ ] Implement and compile JUCE/CMake VST3 and Standalone after RFC; validate scheduler and state. Previously observed local prerequisite gap: CMake and JUCE development libraries missing; this is not proof installation is impossible. Investigate installation before declaring blocked.
- [ ] Build Windows artifact and open PR. Last attempt blocked externally: GitHub API returned Forbidden and gh reported invalid authentication. Recheck after relevant access changes; never request credentials in chat.
- [ ] Validate native plugin in REAPER. Requires compiled plugin and an actual host; not yet executable.

Latest validation: npm test: 18 passed, 0 failed; npm run sample succeeded. Live provider/DAW/browser audio checks not performed. Existing untracked samples/loopforge-demo files were preserved and are not part of the code change.
