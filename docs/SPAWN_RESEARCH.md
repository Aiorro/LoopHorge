# SPAWN 2.0 workflow reference

**Comparative research only.** Reconstructed from the supplied excerpts. The release contract and task.md define LoopForge scope and acceptance. SPAWN observations are attributed to the vendor or the supplied review where indicated.

## Workflow comparison

| Area | SPAWN 2.0 | Attribution | LoopForge implications |
| --- | --- | --- | --- |
| Product format | VST3/AU instrument loaded in DAW; SPAWN itself has no standalone application | Vendor FAQ | LoopForge can differentiate with Standalone |
| Generation | Cloud-hosted Lemonaide MIDI generation; internet required for new AI patterns | Vendor FAQ/product | Keep offline native composer default; models optional |
| Choice | Eight candidate MIDI ideas per generation; generate again for another batch | Vendor product | Preserve candidate bank and non-destructive selection |
| Control | Genre, mood, key (major/minor scales only in review); five advertised generative models | Vendor product / review | Expose stable, validated controls, richer offline scales and versioned recipes |
| Prompt/tags | Instrument, style, mood tags; independent review reports tags mostly guide sound design rather than arbitrary MIDI instructions | Independent review | Separate musical intent from sound-preset tags |
| Layers | Bass, chords, melody with individual instruments | Vendor product | Shared harmonic timeline and layer-aware edits |
| Length | Four- and eight-bar pattern choices described in hands-on review | Independent review / FAQ | LoopForge's approved 4/8/16 bars is its own scope |
| Sound & footprint | More than 100 factory sounds including live instruments/synths; browser/tag filtering; 8 GB disk requirement | Vendor product | Sound is separate from MIDI composition; preserve lightweight portable footprint |
| External instruments | Can host user VST3/AU instruments internally, chosen per layer; rescan workflow | Vendor FAQ/product | Potential later advanced feature, NOT current release |
| Editing | Per-note pitch/start/length editing in internal piano roll; playhead repositioning is DAW-driven (no click-to-seek in piano roll) | Vendor FAQ/review | Keep direct structured MIDI note editing; define explicit VST3 vs Standalone playhead semantics |
| Experiment Pad | Real-time global pitch control and effect morphing | Vendor product/review | Optional UX concept after loop-release acceptance |
| FX | Effects per layer and automation; review lists filter, distortion, chorus, flanger, delay and reverb | Vendor product / review | Keep DSP and note identity independent |

The vendor describes the overall workflow on the product page. The supplied review observations have no review URL in the excerpts. [SPAWN](https://saucewareaudio.com/products/spawn)

## Existing code and data to preserve

- JUCE lifecycle and audio-thread boundaries — [Processor.h](../plugin/Source/Processor.h), [Plugin.cpp](../plugin/Source/Plugin.cpp).
- Offline generation — [Generation.h](../plugin/Source/engine/Generation.h), [Harmony.h](../plugin/Source/engine/Harmony.h), [Rhythm.h](../plugin/Source/engine/Rhythm.h), [Direction.h](../plugin/Source/engine/Direction.h), [Mood.h](../plugin/Source/engine/Mood.h).
- Selective editing and piano roll — [LoopEditing.h](../plugin/Source/engine/LoopEditing.h), [ComposerEditView.h](../plugin/Source/ComposerEditView.h), [PianoRoll.h](../plugin/Source/PianoRoll.h).
- Playback, publication and audition — [Playback.h](../plugin/Source/engine/Playback.h), [Prepared.h](../plugin/Source/engine/Prepared.h), [Publication.h](../plugin/Source/engine/Publication.h), [ComposerPlayer.h](../plugin/Source/ComposerPlayer.h), [PreviewSynth.h](../plugin/Source/PreviewSynth.h).
- MIDI export and drag lifetime — [Midi.h](../plugin/Source/engine/Midi.h), [DragMidi.h](../plugin/Source/DragMidi.h), [MidiDragSource.h](../plugin/Source/MidiDragSource.h), [ComposerMidiExport.h](../plugin/Source/ComposerMidiExport.h).
- Versioned persistence and migration — [CompositionJson.h](../plugin/Source/engine/CompositionJson.h), [ProjectJson.h](../plugin/Source/engine/ProjectJson.h), [LegacyMigration.h](../plugin/Source/engine/LegacyMigration.h), [RecipeJson.h](../plugin/Source/engine/RecipeJson.h).
- Native regressions — [AllocationGuard.h](../plugin/tests/AllocationGuard.h), [RealtimeTest.cpp](../plugin/tests/RealtimeTest.cpp), [RestoreAllocationTest.cpp](../plugin/tests/RestoreAllocationTest.cpp), [NoteLifetimeTest.cpp](../plugin/tests/NoteLifetimeTest.cpp), [LoopEditingTest.cpp](../plugin/tests/LoopEditingTest.cpp), [DragMidiTest.cpp](../plugin/tests/DragMidiTest.cpp), [MidiDragSourceTest.cpp](../plugin/tests/MidiDragSourceTest.cpp), [Vst3ConversionCheck.cpp](../plugin/tests/Vst3ConversionCheck.cpp).
- Contract and DAW procedures — [2026-11-17-loop-release-contract.md](superpowers/specs/2026-11-17-loop-release-contract.md), [FL-STUDIO-TEST.md](FL-STUDIO-TEST.md), [NATIVE-DAW-ACCEPTANCE.md](NATIVE-DAW-ACCEPTANCE.md), [VALIDATION.md](VALIDATION.md).
- Backlog, engineering rules and licensing — [task.md](../task.md), [task-engine.md](../task-engine.md), [AGENTS.md](../AGENTS.md), [errors.md](../errors.md), [NATIVE-DISTRIBUTION.md](../licenses/NATIVE-DISTRIBUTION.md).

## Architecture observations

- The non-real-time generation worker may take significant time; it must support cancellation, stale-epoch rejection and keeping the old project untouched on failure.
- The audio callback must not do inference, plugin scans, filesystem access, network traffic, unbounded heap allocation/deallocation (including dropping the last `shared_ptr` to a retired snapshot on the audio thread — reclaim off-RT via atomic swap + deferred GC queue) or potentially blocking synchronization.
- Keep note ownership deterministic on stop, seek, loop, mute, bypass, state restore and plugin close. Explicitly handle overlapping identical pitches.

LoopForge already returns retired prepared slots to the worker for reclamation. The deferred-reclamation principle applies without changing that ownership scheme.

## Acceptance scenarios from the supplied reference

The following scenarios describe tests to perform. Results belong in the existing DAW acceptance reports.

**Scenario D: host transport correctness**
- In an actual Windows REAPER and FL Studio host, test play/stop/seek/loop/bypass and tempo changes at 32/64 sample buffers **and under FL Studio's variable block sizes**.
- Verify arbitration when the DAW track feeds incoming MIDI into LoopForge (no competing internal pattern playback and no double-triggered notes).
- No stuck notes, duplicate retriggers, missing note-offs or audio-thread blocking. Capture host name/version and repro evidence.

**Scenario E: drag/export semantics**
- Export type-1 MIDI with conductor tempo/meter, distinct source tracks/channels, correct total length and stable timing.
- Verify Windows external drag-and-drop temp `.mid` files use collision-safe naming and remain readable until host async import finishes (no premature delete/overwrite across repeated drags).
- Import in real DAW and check pitch/onset/gate/velocity and part separation against canonical project.
