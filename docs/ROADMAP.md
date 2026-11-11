# Roadmap - LoopForge AI MIDI

## Current: prototype v0.1
- Standalone local web UI and Web Audio preview.
- Offline 8-idea composer with bass/chords/melody.
- MIDI type 1 export; local MCP tool server.
- Provider adapters exist, but live cloud and account-backed CLI requests have NOT been validated end-to-end in this environment.
- Not a compiled VST3/AU.

## v0.2: hardened composer
- Strict machine-readable composition schema and validation tests.
- Provider transport abstraction, retries/timeouts with bounded resources and precise error reporting.
- Reliable persistence/presets, undo/redo and more expressive piano-roll editing.
- Security audit of local HTTP, MCP and CLI adapters.

## v0.3: native plugin skeleton
- Cross-platform JUCE/CMake layout (Windows VST3 first).
- Reproducible CI build with signed or clearly labeled unsigned artifacts.
- Host transport/tempo synchronization, deterministic playback and MIDI output.
- Non-blocking native IPC with the existing local AI service; documented IPC protocol.

## v0.4: production workflow
- Native piano roll and DAW drag-and-drop MIDI; intelligent harmonization/regeneration of selected bars.
- Music theory constraints for genre/groove and optional reference user-provided MIDI.
- Sound engine, preset manager and per-track routing.
- DAW acceptance tests in REAPER/FL Studio/Ableton as available.

## 1.0 release criteria
- Installed native VST3 validated in supported DAWs, no realtime thread stalls.
- Provider sign-in choices clearly distinguished from API keys and MCP hosts.
- Automated tests, reproducible release build and installer/uninstaller.
- Music-generation quality evaluation with documented limitations and user controls.

For all implementation tasks, follow the single live checklist in [../task.md](../task.md) and the rules in [../AGENTS.md](../AGENTS.md). This roadmap is directional, not a separate TODO list.
