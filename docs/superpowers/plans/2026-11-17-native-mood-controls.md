# Native Mood Controls Implementation Plan

Execute inline/TDD within user-approved E03 architecture and repeated authorization to continue features. task.md alone tracks actionable progress.

Goal: expose seven next-request mood pins with Auto in the native Composer. Architecture: a compact settings panel captures ManualOverrides by value into the existing guarded worker; existing interpreter applies manual > text > defaults. Tech: JUCE/C++20, no dependencies or callback changes. Spec: ../specs/2026-11-13-hybrid-engine-design.md and task-engine.md.

One vertical slice: add ComposerMoodControls.h; extend Processor requestComposer with optional ManualOverrides, copied under request ownership and moved to worker-local state. Summary labels manual origins. Plugin settings toggle replaces the overview visually while open, without destroying either view. Seven0–100sliders initially Auto; unchecking Auto enables pin; Auto ignores retained value. Editing during generation affects next request only. Settings are editor-local and not a persisted recipe (remaining E03 scope).

Tests first: missing request API/panel RED; real worker result must equal direct interpreter/generator with captured pins even after caller mutation. All Auto restores text/default behavior. Each of seven pins maps to correct Field and value; controls enabled state, geometry and real editor action tested. Invalid manual values reject without replacing current project. Verify focused then full native build/CTest and npm test; one fresh review before commit/push. Preserve original15tasks and user samples.
