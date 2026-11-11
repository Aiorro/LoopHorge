# Codex kickoff - LoopForge AI MIDI

You are continuing development of an **existing** clean-room music generation project. Do not replace the prototype with placeholders or mock-only implementations. Read `AGENTS.md`, `README.md`, `docs/ARCHITECTURE.md`, `docs/CRAFT_RESEARCH.md`, and `docs/ROADMAP.md` first.

## Immediate task: harden v0.1 into v0.2 (before JUCE work)

1. Run baseline tests (`npm test`) and describe current behavior and key risks based on source inspection.
2. Audit OpenAI/Anthropic/OpenRouter/Ollama adapters and Codex/Claude CLI invocation, especially official CLI output behavior, timeout, rate limit/error handling, cross-platform launch safety and untrusted input bounds.
3. Fix confirmed issues with test coverage. Make sure no secrets leak to browser, logs, commits or MCP tool output.
4. Extend tests for hostile/partial LLM JSON, malformed notes, path traversal requests, invalid provider selection, local service boundaries and model process failures.
5. Define a stable provider and composition data contract in docs with schemas and examples.
6. Write an RFC for JUCE VST3 architecture, audio-thread safety and host synchronization. Prototype the native build only after the RFC; never claim that a VST3 exists before it compiles.
7. Summarize your changes with tested commands, open risks, and a verifiable next task.

## Acceptance criteria
- Existing offline generate, browser editor, MIDI type 1 export and MCP tool functionality remain intact.
- `npm test` passes and coverage expands for any newly discovered bug.
- No keys needed for offline functionality and tests.
- Failures return actionable, sanitized error messages.
- No direct network or agent CLI invocation from the audio thread in future architecture.
- New functionality is documented without exaggerated claims.

## Not in scope without approval
- Renaming/replacing entire product; deleting offline engine; training on proprietary/copyrighted music; emulating undocumented provider login; shipping a fake VST3; publishing a release without validated build artifacts.