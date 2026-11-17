# Loop Release Finalization

Execute the supplied17Nov loop-only finalization contract inline with TDD and one independent integration review per completed vertical result. task.md is the sole progress queue. Original task-engine.md remains byte-preserved historical scope; latest user instruction supersedes song/arrangement requirements.

## Result1: truthful new-generation length boundary

Config/generateLoop/generateDrums and resolved prompt generation accept4/8/16only. Existing composition/project validation, playback/export and long-document limits stay unchanged. Native legacy and Node/web/API/MCP retain4/8only and reject unsupported length instead of silently normalizing. Recipe files remain exact draft data; loading an old direction is not generation and must never clip it; generation validation enforces the cap after text/manual precedence.

Tests first: directConfig32invalid, text32/17manual duration rejects preserving committed long source; 16success; simulated saved32bar project restores/exports full endpoint; legacy/native and Node normalize/generate32rejected; old4/8 output unchanged. Update previous successful32bar generation stress cases to maximum16, leaving long-model fixtures intact. Run full build/CTest, Node and isolated sample, relevant native API scripts. Review before claiming Result1.

Preflight ruling: generation Config bounds are distinct from model validation; changing document validators to16 would break old state and is prohibited. Legacy4/8contract is intentionally retained, not expanded into v1 schema changes. Recipes are draft import/export, not a generation endpoint: reject on Compose while preserving original text/seed/pins in the file. Cost: a recipe may load a currently unsupported direction but cannot generate or silently shorten it.

Remaining results follow the finite queue in task.md: controls/workflow, Composer editing/selection regeneration, host/project recipe persistence, audition/drag/DAW verification, resource acceptance, installable package. Do not add full songs/sections/models. External Windows/DAW blockers remain explicit; never replace them with local tests.

## Result2a: committed Composer note editor

Reuse the existing piano roll with a bounded typed Composition projection, exact string IDs and quarter-beat/meter coordinates. Immutable note/range helpers validate source and candidate; guarded Processor publication installs music, MIDI/view and bounded Undo/Redo together. Keep request recipes independent. Actual UI exposes track selection/Edit Notes/overview while preserving Legacy controls and old long-document overview/export. Limit interactive projection to16meter bars/8192notes/8MiB, history32states/16MiB; reject candidate limits before publication.

Tests: pure source preservation, ID/range/bounds/ownership/density/allocation recovery; Processor edit/history/draft/stale guards; real piano roll four tracks, stable selection/quantize/Undo/save/reopen/meter/high IDs, empty-layer recovery, old-document switch and viewport origin. Expected RED on absent model/Processor/view support, then GREEN. One fresh integration review; Important findings get one RED→GREEN fix pass and full native/Node/API/sanitizer verification. Evidence/rulings: docs/superpowers/reviews/2026-11-17-composer-editing.md. R02 remains open for actual scoped musical regeneration, remaining controls and audition/loop workflow.

## Result2b: bounded native Composer audition

Use the existing JUCE audio output and audition toggle. Add one fixed64voice preview synth consuming the already-produced ordered short MIDI; it must not change MIDI bytes or allocate/lock in processBlock. Melodic voices use a basic sine, channel10uses short deterministic synthetic percussion. Honor sustain/sostenuto/hold2 releases and cleanup; no assets, new providers or General MIDI emulation. Source/mode/lifecycle changes clear preview ownership. This implements audition only; standalone currently remains one-shot, seamless loop segmentation stays R02 work.

Tests first: real Composer32/64sample audio becomes nonzero when enabled, stays finite/bounded, respects pedal release/transport stop and mute, and leaves generated MIDI byte-identical to muted playback. Dense input stays bounded; cold/tempo/stop RT allocation test includes Composer. Expected current silent Composer RED. Implement fixed synth and processor/UI bridge, then full native/Node/API tests, scoped sanitizers and one independent integration review. Execution remains inline under the user's existing finalization authorization; no additional design approval or new external dependency is needed.
