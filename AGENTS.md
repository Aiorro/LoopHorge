# Codex development instructions - LoopForge AI MIDI

## Mission
Build an original, production-grade AI-assisted MIDI composition tool. The repository name is `LoopHorge`, while the product name remains **LoopForge AI MIDI**. Do not rename the GitHub repository without explicit user approval.

Start by preserving and improving the current Node.js v0.1 prototype. The eventual goal is a real JUCE-based VST3 instrument/MIDI effect that works in professional DAWs. This repository is **not** yet a compiled VST3.

## Mandatory working method
1. **Inspect** the existing code, tests, docs and package scripts before changes; document uncertainty instead of inventing capabilities.
2. **Specify** the behavior, interfaces, acceptance checks, security implications and migration path for each vertical slice.
3. **Implement** the smallest useful improvement; prefer test-driven work and keep the offline generator available.
4. **Verify** via `npm test`, `npm run sample` where relevant, manual browser/API checks for UI changes, and real DAW validation for native plugin milestones.
5. **Report** what changed, test evidence, what was not tested, and exactly what still needs to be built. Do not claim a feature works without evidence.

## Product constraints
- First-class operation without cloud credentials: deterministic offline composition and MIDI export must always work.
- Planned AI options: OpenAI API, Anthropic API, OpenRouter, local Ollama, and optional signed-in Codex/Claude CLI bridges when permitted by official provider capabilities. Account subscription != general API credential. Never ask for or store account passwords, cookies, access tokens or billing credentials.
- MCP: the current implementation is a local **stdio tool server**, not a remote OAuth service. Do not claim that connecting an MCP client automatically gives the plugin access to that client's model; the client must explicitly call MCP tools or configure an approved provider adapter.
- AI responses are untrusted: parse, normalize, validate and bound MIDI note data, durations, velocity, counts and output size before using it.
- Bind HTTP services to loopback by default. Do not expose API keys in web assets, logs, repository or MCP replies.
- Never execute untrusted AI text as a shell command, script or plugin. Confirm consent before adding code which installs packages or downloads models automatically.
- Real-time audio callbacks must never block on networking, file I/O, process launches, synchronization, or AI inference; use worker/background threads with bounded handoff.
- Keep music theory constraints configurable (key, scale, BPM, bars). Preserve independent bass, chords, melody export and DAW interoperability.
- Avoid copyrighted instrument presets, proprietary SPAWN assets, source code, branding, reverse-engineered private APIs or trained weights. Follow clean-room behavior-first design.

## Architecture boundaries
- `src/core.js`: deterministic offline generation and MIDI encoding.
- `src/schema.js`: schema normalization and note validation.
- `src/ai.js`: HTTP provider adapters and generation prompt.
- `src/agent-cli.js`: optional authenticated CLI adapter.
- `src/operations.js`: application operations and export.
- `src/mcp-server.js`: MCP stdio transport and tool dispatch.
- `src/server.js`: local HTTP server.
- `web/`: current standalone UI and piano roll.
- `integrations/reaper/`: REAPER import support.
- Future `plugin/`: JUCE VST3/AU host integration only after an RFC and reproducible build instructions.

## Engineering requirements
- Support Node.js >=20. Prefer built-in Node APIs and minimal dependencies for the prototype.
- Update/add focused automated tests with every behavioral change. Run `npm test` before committing.
- Do not break MIDI type 1 semantics, track separation, PPQ timing, velocity handling or lossless round-trip of valid note events.
- Keep application files UTF-8, avoid generating test output in the repository except intentionally tracked fixtures.
- In release notes distinguish prototype from production features and mark live cloud/DAW tests accurately.
- Use English for code, docs and commits. User-facing discussion can be in Russian.

## Priority work after migration
1. Audit current AI adapter error handling, timeouts, API response formats, CLI sandboxing and MIDI data validation. Add adversarial tests.
2. Introduce a documented provider interface, mockable transports, and reproducible API/MCP integration tests without secrets.
3. Create an RFC and minimal JUCE VST3 project with CMake, isolated from Node, building on Windows in GitHub Actions.
4. Implement MIDI scheduling in DAW host time and a secure non-real-time IPC bridge. Then native piano-roll UI and preset management.
5. Only after tests in actual hosts: release candidate artifacts and user guide.

See `docs/ARCHITECTURE.md`, `docs/ROADMAP.md`, and `CODEX_TASK.md` for more detail.