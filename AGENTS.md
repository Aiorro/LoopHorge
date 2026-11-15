# AGENTS.md - LoopForge AI MIDI development contract

This file is the **single source of truth for agent behavior, engineering rules, tool usage, and completion standards**.
The root-level [task.md](task.md) is the **single source of truth for actionable work, priorities, checkboxes, and progress**.
Do not create or maintain competing TODO lists in README, ROADMAP, issues, audit files, or chat messages. Issues and plans may link to `task.md`, but must not replace it.

## 1. Mission and source of truth

Develop an original, reliable AI-assisted MIDI composition product, with a standalone application and a real JUCE VST3 plugin as future native deliverables. GitHub repository: `Aiorro/LoopHorge`. Product name: **LoopForge AI MIDI**; keep the existing repository spelling unless the user approves changing it.

At the start of **every** Codex session:
1. Read `AGENTS.md`, then `task.md`; inspect the affected code, tests, and relevant documentation.
2. Run `git status --short --branch` and inspect recent commits; **preserve** uncommitted and unpushed work. Do not reset or overwrite other work.
3. Select the first **unblocked** unchecked task under the earliest incomplete milestone in `task.md`; respect dependencies.
4. Work through its subtasks; implement, test, review and only then mark its checkbox `[x]`.
5. Update `task.md` **in the same commit** as the implementation or verification that completes the item. Use brief evidence next to the completed item or in its evidence section, e.g. command, platform, commit, workflow URL.
6. Continue to the next unblocked TODO in the same session when feasible. Do not stop after producing a plan or writing a report.
7. End with a concise summary: changed files, completed task IDs, exact test results, remaining task IDs, blockers, commit/PR links if available.

A checkbox `[x]` means the acceptance conditions **actually passed**, not that code was drafted, a command was attempted, or someone previously reported success. Leave unverified work `[ ]`. On a blocker, record its cause, mitigation, next action, and verification needed in the **Blockers** section of `task.md`; continue other independent tasks.

## 2. Method: research -> design -> build -> verify -> review

Use a Craft Anything-style iterative workflow as a **method**, not as a claim that an external skill is installed: inspect current behavior; define a small testable slice; implement; run checks; review correctness, UX and security; record evidence; repeat. If a relevant installed skill exists and is available, use it; otherwise follow these written rules.

- Favor working vertical slices over speculative framework rewrites or mock-only UI.
- Write or update focused tests with behavioral fixes; document reproducible failure cases.
- Avoid redundant refactors unrelated to the active `task.md` item.
- Keep English in code, commits and repository documentation. Conversation with the user may be in Russian.
- Never fabricate successful builds, tests, releases or host validation.
- Do not leave TODO comments as a substitute for the authoritative checklist.

## 3. Tools and preferred workflow

Use only tools actually available in the current environment. Check versions/capabilities before relying on them. **A missing optional connector is not grounds to stop local development.**

| Purpose | Preferred tools | Fallback / verification |
| --- | --- | --- |
| Repository | `git`, connected GitHub tools, `gh` if authenticated | Local commits; document the exact push/PR command if remote auth is blocked |
| Node prototype | Node.js >= 20, `npm`, built-in `node --test` | `node --version`, `npm test`, `npm run sample` |
| Native builds | CMake, Ninja or MSBuild, C++20 toolchain (MSVC on Windows) | Windows GitHub Actions runner with pinned dependencies if local tools are absent |
| Audio plugin | JUCE via pinned CMake integration, VST3 support through JUCE | Compile a minimal processor/editor first; document JUCE version/license |
| Plugin tests | JUCE unit tests, CTest, pluginval when available | Headless instantiation/smoke checks; manual REAPER/DAW tests clearly marked unavailable |
| AI/MCP | Mock transports, local stdio MCP harness, provider SDK/docs | Test with fixtures; no paid credentials required for CI |
| Documentation | Context7 MCP for current library docs **if connected**; official upstream docs | Pin actual APIs/versions in docs, do not hallucinate SDK calls |
| UI verification | Browser tools when present, manual smoke testing | Reproducible steps and screenshots where possible |

Preferred execution commands for current prototype:
```sh
node --version
npm test
npm run sample
npm start
npm run mcp
```

Native target commands **after** CMake files exist (adapt to real preset names):
```sh
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release --output-on-failure
```

Do not claim these native commands work before verifying they exist. No forced dependency downloads, package managers, installer scripts or model downloads on the user's machine without approval. CI dependencies may be installed from pinned, reviewed upstream sources as part of a documented build workflow. Never commit tokens, API keys, cookies, private credentials or generated build output.

## 4. Existing code ownership and architecture

- `src/core.js`: deterministic offline music generation, MIDI encoding.
- `src/schema.js`: note/track normalization and strict bounds validation.
- `src/ai.js`: HTTP AI provider adapters, prompt/response handling.
- `src/agent-cli.js`: optional authenticated Codex/Claude CLI adapters.
- `src/operations.js`: composition/export orchestration.
- `src/mcp-server.js`: local stdio MCP tools.
- `src/server.js`: loopback HTTP service.
- `web/`: standalone browser interface and piano roll.
- `integrations/reaper/`: optional DAW import helper.
- `plugin/`: future JUCE/CMake native application and VST3; implement incrementally under `task.md`.

Preserve tested behavior and keep the standalone/offline generator usable throughout the migration. Define stable composition and IPC contracts before connecting Node and JUCE; ensure schemas support deterministic golden tests.

## 5. Safety, performance and correctness

- **Audio thread:** no file I/O, network access, CLI/process launches, waits, locks that may block, AI inference or unbounded allocations in process callbacks. Hand off via bounded non-RT queues/snapshots.
- **AI inputs:** treat model output as untrusted data. Validate structure, count, pitch (0-127), velocities, durations, starts, total bars, output size and file paths. Sanitize provider/CLI error messages. Bound timeouts and provide cancellation.
- **Authentication:** OpenAI API, Anthropic API, OpenRouter, local Ollama and optional official CLI bridges may be supported where legitimately authorized. A ChatGPT/Claude subscription is not automatically a third-party API entitlement. MCP tool access does not imply account credential sharing.
- **HTTP/MCP:** local server binds to loopback by default; MCP currently uses local stdio. Never expose tokens to web assets, logs or MCP results; do not evaluate LLM-generated scripts.
- **MIDI:** preserve MIDI type 1, tempo, PPQ, track separation, valid note-off ordering and boundary-safe note durations. Existing bass/chords/melody tracks are not disposable.
- **IP:** independent clean-room behavior-based implementation. Never copy SPAWN code, private APIs, presets, assets, music or model weights.
- **Engineering:** minimize dependencies, pin external native build versions, document licenses and maintain cross-platform-ready structure.

## 6. Branches, pull requests and blocked infrastructure

- Work on the active task branch; when needed create an appropriately named feature branch. Never force-push or rewrite unrelated history.
- Before creating PR: check push visibility and CI; open a PR when GitHub authentication permits. Do not claim PR/CI occurred if only local changes exist.
- If `gh` or GitHub API is forbidden, keep meaningful **local commits**, record missing remote auth in `task.md`, and continue implementable code work. Never paste auth tokens into prompts or files.
- If CMake, JUCE or Windows compiler is absent locally, write and validate source/build configuration as far as possible; configure pinned Windows CI when permission allows. Mark actual compilation and host-load tasks incomplete until independently verified.
- For native artifacts, distinguish **compiled**, **pluginval-validated**, **host-loaded**, and **release-ready**; these are different milestones.

## 7. Done criteria and final response

For every task `Txx`, use the acceptance/evidence described in `task.md`. Test code changes at the narrowest useful level and run `npm test` before committing; for MIDI changes also run `npm run sample`. For JUCE changes, perform CMake configure/build/CTest or explain exactly why unavailable. For releases require a real artifact and independently verified CI, never a placeholder `.vst3`.

Only edit task priorities or scope when new dependencies or reproducible evidence justify it; briefly explain the change in `task.md`. Do not silently auto-check milestones. Follow `task.md` until all possible tasks are closed or an external blocker is documented.

## Diagnostic register

The user requested root errors.md on2026-11-15. Maintain stable finding IDs, severity, status and verification evidence there. It is a diagnostic register, not a second execution checklist: task.md remains the sole source of priorities/actionable checkboxes. Add new findings and update statuses during the same work; never imply unverified platform/host acceptance from a local fix.
