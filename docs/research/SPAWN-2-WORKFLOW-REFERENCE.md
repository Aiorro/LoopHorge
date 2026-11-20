# SPAWN 2.0 workflow reference and LoopForge design implications

**Research date:** 2026-11-20  
**Product observed:** Sauceware Audio x Lemonaide SPAWN / SPAWN 2.0 (VST3/AU), not an inspection of proprietary source code.  
**Repository baseline:** Aiorro/LoopHorge, remote main README.md, plugin/README.md, AGENTS.md, task.md and the 2026-11-17 loop-release contract as read on the research date.  
**Status:** Comparative research only. This document does not authorize a scope change, implement SPAWN functionality, or close any release acceptance item.

## Executive finding

SPAWN is best understood as an **idea-to-DAW workflow**, not simply a text-to-MIDI model. It connects (1) cloud symbolic generation, (2) an eight-idea selector, (3) coherent bass/chord/melody MIDI, (4) immediately playable instrument sounds and effects, (5) direct note editing, and (6) drag/export to a DAW. The major differentiation to retain for LoopForge is **offline-first generation, user-owned/editable MIDI, deterministic project state, and an optional model boundary**.

**Important nuance:** In the independent hands-on SPAWN review, the prominent text/tag area mainly guides **sound/instrument selection**, while MIDI generation is primarily configured by BPM, key and model/style. Do not assume SPAWN implements arbitrary natural-language music arrangement instructions. This is an independent review, not a guarantee about every SPAWN version.

## 1. Evidence and limitations

Evidence levels used below:

- **Confirmed vendor:** Feature publicly stated by Sauceware or Lemonaide.
- **Observed review:** Independent published hands-on test or screenshot, subject to version drift.
- **Design inference:** Our architectural hypothesis for implementing analogous behavior; not claimed to describe SPAWN internals.
- **LoopForge baseline:** What current repository documentation reports; not equivalent to a new local build or DAW host test.

Neither the weights, architecture, training corpus, exact cloud API protocol, DSP graph, VST subhost implementation, nor internal audio-thread ownership of SPAWN are publicly documented in the sources reviewed. Do not present the conceptual diagram below as reverse-engineered proprietary implementation.

### Primary sources

1. Official product: https://saucewareaudio.com/products/spawn
2. Official FAQ and support: https://sauceware-audio.myshopify.com/pages/spawn-help
3. Partner Lemonaide product page: https://www.lemonaide.ai/spawn
4. Independent hands-on analysis (2025-06-10): https://www.audiocipher.com/post/sauceware-spawn-ai-midi-plugin
5. Official walkthrough video: https://www.youtube.com/watch?v=Ttx-icn5Qqs
6. SPAWN 2.0 announcement video: https://www.youtube.com/watch?v=vEP5sdPotSc

These are references, not bundled third-party documentation or licensed source assets.

## 2. SPAWN user-facing behavior

| Subsystem | Publicly reported operation | Evidence | LoopForge relevance |
| --- | --- | --- | --- |
| Product format | VST3/AU instrument loaded in DAW; SPAWN itself has no standalone application | Vendor FAQ | LoopForge can differentiate with Standalone |
| Generation | Cloud-hosted Lemonaide MIDI generation; internet required for new AI patterns | Vendor FAQ/product | Keep offline native composer default; models optional |
| Choice | Eight candidate MIDI ideas per generation; generate again for another batch | Vendor product | Preserve candidate bank and non-destructive selection |
| Control | Genre, mood, key; five advertised generative models | Vendor product | Expose stable, validated controls and versioned recipes |
| Prompt/tags | Instrument, style, mood tags; independent review reports tags mostly guide sound design rather than arbitrary MIDI instructions | Independent review | Separate musical intent from sound-preset tags |
| Layers | Bass, chords, melody with individual instruments | Vendor product | Shared harmonic timeline and layer-aware edits |
| Length | Four- and eight-bar pattern choices described in hands-on review | Independent review / FAQ | LoopForge's approved 4/8/16 bars is its own scope |
| Sound | More than 100 factory sounds including live instruments/synths; browser/tag filtering | Vendor product | Sound is separate from MIDI composition |
| External instruments | Can host user VST3/AU instruments internally, chosen per layer; rescan workflow | Vendor FAQ/product | Potential later advanced feature, NOT current release |
| Editing | Per-note pitch/start/length editing in internal piano roll | Vendor FAQ/review | Keep direct structured MIDI note editing |
| Experiment Pad | Real-time global pitch control and effect morphing | Vendor product/review | Optional UX concept after loop-release acceptance |
| FX | Effects per layer and automation; review describes filters/delay/reverb/etc. | Vendor product / review | Keep DSP and note identity independent |
| Mixing | Multi-output audio routing for bass/chords/melody | Vendor FAQ | Future independent audio bus design |
| Export | Drag to DAW, MIDI and audio/WAV export; split vs bundled MIDI workflows | Vendor FAQ/product/review | Validate real host drag target and type-1 MIDI import |
| Hardware MIDI | Product advertises external hardware MIDI routing | Vendor product | Not needed for frozen alpha |
| Offline | Offline *instrument/play manually* mode; not offline AI generation | Vendor FAQ | LoopForge native offline composition is a real difference |
| License/output | Vendor claims royalty-free output, one-time purchase and two activated machines | Vendor FAQ | Do not transfer vendor license guarantees to LoopForge |

### Reconstructed public workflow

~~~text
USER INPUT
  genre / mood / key / generation model / instrument tags
          |
          v
CLOUD MIDI GENERATION (Lemonaide; internal details undisclosed)
          |
          v
EIGHT CANDIDATE IDEAS
  each presented through bass + chord + melody layers
          |
          +------> MIDI PIANO ROLL (inspect, add, move, resize, remove)
          |
          +------> SOUND ENGINE (presets / per-layer FX / optional hosted VST)
          |                           |
          |                           v
          |                    IN-PLUGIN AUDITION
          |
          +------> EXPORT: bundled or split MIDI / audio
                                       |
                                       v
                                 HOST DAW WORKFLOW
~~~

This is a **behavioral decomposition**, not a claim that SPAWN uses these exact processes, services, classes or messages.

### Bundled vs split export and the DAW instrument question

- **Split MIDI** exports part data for independent DAW instrument tracks; a MIDI file contains notes/events, not instrument audio or the sound engine's state.
- **Bundled MIDI** can be kept on the SPAWN instrument track to use SPAWN's assigned sounds, subject to its instrument-mode behavior (independent review).
- Simply dragging MIDI into another DAW instrument track **does not transfer Serum/Omnisphere or SPAWN timbres**. To preserve the sound, keep the original hosted instrument setup or bounce audio.
- The independent review describes SPAWN switching to instrument mode when DAW MIDI is placed on its instrument track, avoiding simultaneous competing internal playback. This is a useful state-machine edge case to model in LoopForge tests.
- Product pages advertise audio export, but a project-state file, standard MIDI file and rendered audio export are **different** artifacts with different preservation guarantees.

## 3. Technical design lessons for LoopForge

### 3.1 Separate symbolic composition, sound realization and host routing

A robust architecture should use three independent data planes:

1. **Composition plane** (host-independent): time signature, tempo map, key/scale map, bars, tracks, immutable note/event IDs, pitch, velocity, tick start/end, CC/program maps, origin and seed/algorithm version.
2. **Sound plane** (optional): track-to-sound assignment, basic GM/synth preview or user-selectable patch, effects and automation. No sound selection should silently rewrite symbolic notes.
3. **DAW plane** (host-dependent): MIDI transport synchronization, plugin state restore, MIDI drag/drop/file export, host mixer buses, and externally hosted instruments.

The output of a text/LLM model must be parsed and validated into the same composition contract used by the deterministic offline engine. No model is allowed to control the audio callback, write arbitrary files, or bypass note/event/resource bounds.

### 3.2 Recommended LoopForge boundary diagram (PROPOSAL, not SPAWN internals)

~~~text
JUCE EDITOR / COMPOSITION CONTROLS
    |
    +--> OFFLINE RULE COMPOSER ----+
    |                             |
    +--> OPTIONAL LOCAL MODEL ----+--> NON-RT VALIDATOR --> IMMUTABLE SNAPSHOT
    |                             |                            |
    +--> OPTIONAL PROVIDER -------+                            |
                                                +--------------+-------------+
                                                |                            |
                                             PIANO ROLL                 AUDIO CALLBACK
                                             EDIT HISTORY           MIDI CLOCK / NOTE SAFETY
                                                |                            |
                                         PROJECT STATE                 BASIC AUDITION /
                                         MIDI TYPE 1                   HOST MIDI OUT
                                         DRAG OR SAVE
~~~

Implementation notes:

- The non-real-time generation worker may take significant time; it must support cancellation, stale-epoch rejection and keeping the old project untouched on failure.
- The audio callback must not do inference, plugin scans, filesystem access, network traffic, unbounded heap allocation or potentially blocking synchronization.
- Keep note ownership deterministic on stop, seek, loop, mute, bypass, state restore and plugin close. Explicitly handle overlapping identical pitches.
- All generated candidates must be immutable until accepted; editing a candidate should never accidentally mutate other candidates.
- Persist the originating seed, engine version, normalized controls, manual pin precedence and the committed note document; do not confuse regeneration recipe with full project recovery.
- Audio timbres are not serialized into MIDI. Exporting a project and exporting MIDI must remain different operations with different promises.

### 3.3 Concrete conceptual contracts (for review, NOT a replacement for existing schemas)

The current composition-v2/project-v3 serialization and MIDI encoder are authoritative; these examples are design vocabulary only.

~~~json
{
  "request": {
    "mode": "offline",
    "bars": 8,
    "bpm": 140,
    "key": { "root": "D", "scale": "minor" },
    "style": "trap",
    "roles": ["bass", "chords", "lead", "drums"],
    "seed": 42,
    "controls": {
      "mood": "resolved-controls",
      "manualPinsTakePrecedence": true
    }
  },
  "result": {
    "candidateCount": 8,
    "symbolicComposition": "validated-versioned-document",
    "soundAssignments": "independent-optional-mapping"
  }
}
~~~

This JSON is illustrative, not a claim that LoopForge or SPAWN currently accept this wire format. Do not replace existing validated contracts based on it.

### 3.4 Local neural generation vs rule-based generation

- **Current reliable fallback:** native deterministic music rules with explicit groove/harmony/variation handling.
- **Optional local neural adapter:** produce symbolic MIDI/note events, validate and normalize them, then audition through the same sound engine. A general-purpose chat LLM is not necessarily trained for useful rhythmic or multi-track music.
- **Optional remote adapter:** same interface and bounds, explicit consent and clear network/privacy/usage disclosure.
- For neural model evaluation, benchmark musical coherence and phrase continuity **per genre** rather than claiming quality from model size or parameter count.
- Never auto-download third-party weights, require unverified CUDA compatibility or promise a given license allows redistribution.
- Preserve a fully functional offline engine when any model fails, refuses, is slow or is absent.

## 4. Current LoopForge gap analysis (documented main baseline)

| Capability | LoopForge documented state | What SPAWN demonstrates | Priority relative to approved alpha |
| --- | --- | --- | --- |
| JUCE VST3 | Native VST3 + Standalone source/build exists | SPAWN VST3/AU instrument | Existing; test Windows DAW acceptance |
| Offline symbolic generation | Native rule-based generation, 4/8/16-bar Composer | SPAWN's new AI generation needs cloud | Key product differentiation, preserve |
| Candidate browsing | Eight ideas in Legacy/native flows | Eight variations per cloud generation | Existing; integrate cleanly with Composer |
| Coherent roles | Bass/chords/lead, optional drums with shared musical data | Bass/chords/melody with independent sound layers | Existing concepts; check continuity and layer safety |
| Note editing | Composer piano roll, note edits, quantize, Undo/Redo | Direct internal note editing | Existing; verify saved edits survive host restore |
| Selected-region regeneration | Reported as incomplete in task.md R02 | Vendor supports whole new idea selection; per-range AI semantics not verified | **Current release requirement**; do not assert SPAWN implements it |
| Seamless playback | Reported incomplete in task.md R02 | SPAWN auditions loop patterns | **Current release requirement** |
| MIDI file export | Type-1 multi-track export; project maps documented | MIDI export from plugin | Existing; independent DAW round trip needed |
| MIDI drag/drop into DAW | T12/R04 acceptance open in task.md | Direct drag part or bundled MIDI | **Current release requirement** |
| Audio preview | Bounded basic sine/percussion/GM-like audition documented | >100 factory sounds/effects | Sound-quality parity is NOT an alpha blocker |
| Audio/WAV rendering | Not established as current release requirement | Vendor advertises audio export | Future, only after accepted alpha |
| Hosted third-party VST3/AU | Not claimed as implemented | SPAWN can subhost instruments | Future; expensive stability/licensing feature |
| Per-layer multiout / FX morph | Not established as current release requirement | Advertised by vendor | Future; do not expand alpha |
| Real-world compatibility | Windows package published per later task.md evidence; REAPER/FL host acceptance still open | SPAWN supported DAWs list in vendor FAQ | **Independent Windows/DAW validation required** |

**Important status distinction:** Repository task.md contains both old snapshots and newer dated evidence. Later entries report a Windows alpha release and further alpha iterations. Do not extrapolate that these were successfully smoke-tested in FL Studio or that a newer candidate automatically satisfies R02/R04. The current checked status and concrete test logs in task.md/errors.md remain authoritative.

### Existing code/data to preserve

- plugin/Source/engine/Generation.h: native composition generation.
- plugin/Source/engine/Midi.h: worker-safe type-1 MIDI encoding.
- plugin/Source/engine/CompositionJson.h: strict versioned project persistence.
- plugin/README.md: build, engine and user-mode documentation.
- docs/superpowers/specs/2026-11-17-loop-release-contract.md: explicit current scope.
- task.md: **sole authoritative backlog and completion checklist**.
- AGENTS.md and errors.md: engineering rules and diagnostic findings.

File names and code owner references reflect the repository documentation read for this report. Confirm paths against the working branch before editing them.

## 5. Behavioral acceptance scenarios inspired by SPAWN

These are **test-design notes**, not new TODO checkboxes. Map any work to the existing R02/R04/R05/R06 tasks in task.md.

**Scenario A: offline first-run**
- Disconnect network, open Standalone or VST3, request a supported 4/8/16-bar loop, receive and audition it without Node, credentials or weights.
- Retry from same normalized seed/algorithm version and verify identical symbolic output, independent of sound preset.

**Scenario B: idea isolation**
- Create eight candidates, choose one, edit its bass note, undo, then switch candidates and return.
- No silent change of the other seven candidates or committed document; project save/reopen retains expected selection.

**Scenario C: selective regeneration**
- Select 2 bars of lead only and regenerate. Verify untouched bass/chords/drums and unselected lead bars preserve note IDs, positions, controls and sound assignments.
- On cancellation, invalid output, project restore or a stale completion, keep prior musical state byte-for-byte where the persistence contract permits.

**Scenario D: host transport correctness**
- In an actual Windows REAPER and FL Studio host, test play/stop/seek/loop/bypass and tempo changes at 32/64 sample buffers.
- No stuck notes, duplicate retriggers, missing note-offs or audio-thread blocking. Capture host name/version and repro evidence.

**Scenario E: drag/export semantics**
- Export type-1 MIDI with conductor tempo/meter, distinct source tracks/channels, correct total length and stable timing.
- Import in real DAW and check pitch/onset/gate/velocity and part separation against canonical project.
- Distinguish dropping notes onto an external synth from retaining the exact sounds of a hosted instrument; do not promise timbre migration.

**Scenario F: save/restore**
- Save DAW project with committed edited notes and seed/recipe; close/reload, reopen editor and audition exact note document.
- Previously saved v1/v2/longer projects must remain loadable without truncation even though NEW generation is restricted to <=16 bars.

**Scenario G: optional-model failure**
- With local model missing or remote disconnected, offline core stays available and existing project remains intact.
- Report the actual provider/backend fault without exposing secrets, environment, token paths, private prompts or upstream diagnostic bodies.

**Scenario H: bounded resource behavior**
- Exercise the maximum approved 16-bar workflow with idea browsing, piano-roll editing, Undo/Redo, preview and export.
- Measure aggregate memory versus the existing 256 MiB budget and slow storage responsiveness; do not silently change the target.

## 6. Practical prioritization and release boundary

The 2026-11-17 user contract freezes the current product at **native offline loop generation up to 16 bars**. It explicitly excludes new models, instruments, provider expansion, new roles, additional platforms, whole-song structure and unrelated feature expansion.

For the **current alpha** concentrate on the existing R02/R04/R05/R06 gates: selective regeneration, seamless loop audition, DAW drag/routing, actual Windows REAPER/FL Studio acceptance, resource measurements, secure portable delivery and project restore. This research document does **not** change that priority.

For a **possible later post-release design discussion** (not an authorized implementation queue), the most commercially relevant lessons from SPAWN are:

1. **Sound assignment independently from note generation:** sounds can change without losing the notes.
2. **Fast idea comparison with non-destructive history:** musical exploration is the primary workflow.
3. **Instrument-hosting strategy:** only consider loading third-party plugins after a separate stability, host-compatibility, lifecycle, licensing and security design review.
4. **Audio export and independent mixer routing:** explicit bus and render contracts if later requested.
5. **Optional neural symbolic model:** only after verifying legal weights, supported hardware and actual multi-genre musical quality.

Do not copy SPAWN code, private API, license bypasses, training data, preset files, GUI assets, branding or samples. Implement only publicly observable behaviors via original code and legal dependencies. The product is a comparator, not a source tree.

## 7. Product positioning

**SPAWN publicly offers:** online AI MIDI idea generation with tightly integrated sounds, editor, effects, subhosting and DAW export.  
**LoopForge's chosen identity:** a local-first symbolic MIDI composition and editing tool with versioned deterministic output, standalone availability, editable bass/chords/lead/drums and optional AI/model adapters.

The strongest practical goal is not visual imitation. It is the short, robust sequence **set direction → generate loop → compare → audition → edit/regenerate selection → save/reopen → drag/export to DAW**, verified on a shipping Windows VST3/Standalone package.

---
**Research limitations:** No proprietary binary was installed, decompiled or dynamically instrumented during this research. No SPAWN source, private API, actual network traces, or local DAW session was available. Claims about its internals would require additional authorized evidence. This file is a reference for LoopForge's engineers, not an official SPAWN manual.
