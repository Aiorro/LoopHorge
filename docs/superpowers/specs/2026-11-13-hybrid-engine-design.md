# Hybrid Offline Composer: architecture and source audit

Date: 2026-11-13. Status: approved by the user on2026-11-13 ("погнали я подтверждаю"); no engine implementation claim.

Requirements: [verbatim supplied specification](../../../task-engine.md). Execution status lives only in [task.md](../../../task.md), E01–E06. Base: `bf93d10b6e088d34578eab3901a035a410bcc778`; isolated branch `codex/loopforge-hybrid-engine`.

## Intent and constraints

Extend the existing JUCE VST3/Standalone from short ideas to a coherent multi-track composer. The native procedural engine must interpret RU/EN musical directions, plan shared harmony and motifs, arrange sections, support scoped edits and export a complete MIDI file without Node, HTTP, internet or a model. Keep existing ideas, piano editing, undo/redo, optional provider adapters and drag/export usable throughout integration.

The engine is deterministic for a resolved config, seed and engine version. Automated musical constraints are necessary but do not establish artistic quality: reviewed listening/MIDI examples are part of acceptance. Neural generation remains visibly unavailable until an actual compatible symbolic model and native runtime pass the license/resource/control checks. No hidden network, telemetry, model download or cloud license check is permitted in offline mode.

The user's final instruction explicitly makes architecture approval a prerequisite for phased product implementation. This document provides the concrete architecture to approve. Phase execution plans will be written after approval. Numeric limits and interface choices below are engineering proposals, not previously agreed user requirements.

## Source audit at the base commit

| Existing component | Observed behavior | Reuse and required extension |
| --- | --- | --- |
| `plugin/Source/Offline.h` | Native xorshift rules; eight ideas, 4/8 bars, four modes, five styles, three layers. `description` is ignored; no arrangement/motif identity/drum tracks. | Preserve the legacy generator and its deterministic tests. Add a separate native composer, sharing verified helpers only when useful. |
| `Schedule.h`, `Wire.h` | `Schedule` contains 3200 fixed note events: at most 1600 notes. Validation fixes 4/4, 16/32 quarter beats, three layers, max800 notes/layer. Strict JSON parser is capped at4MiB/depth32. | Keep bridge-v1 validation unchanged. A long composition needs a separate model, decoder and prepared playback representation. |
| `Processor.h` | Non-RT generation thread, three atomic state slots, mutex-protected UI document, revision guards, 64 full-document undo entries. State envelope version1 stores one idea. The worker also owns optional loopback requests. | Reuse lifecycle/cancellation and atomic publication principles. Full-song storage/history and worker tasks require explicit new budgets; avoid 64 whole-song clones. |
| `Processor.h` callback | Repeats the active schedule modulo its length; binary search per sample; counted same-pitch lifetimes/retriggers. Host PPQ/BPM overrides internal timing. 2048 output events/block with384 reserved for legacy three-channel release. Oscillator audition renders only channels1–3. | Preserve legacy behavior. A song requires one-shot completion, all-channel accounting, event-driven scheduling and bounded overflow release; do not reuse the legacy release assumption for16 channels. |
| `Editing.h`, `PianoRoll.h`, `Plugin.cpp` | Deep-copy revision-guarded edits, fixed bass/chords/melody names, quarter-beat grid, width fits the whole loop; controls have labels and keyboard support. | Retain interaction behavior. Adapt the view to stable track/note IDs and a visible time window; do not render all song bars on every repaint. |
| `MidiExport.h`, `DragMidi.h`, `MidiDragSource.h` | Type1, PPQ480, tempo+4/4 conductor and three note tracks; one-tick minimum/full-pattern EOT. Drag cache limits256 files/16MiB with leases. | Share cache/gesture safety after separating encoder input from legacy ideas. Add v2 tracks/maps/CC; bound encoded size before writing. |
| `src/core.js`, `schema.js`, `operations.js`, `schemas/bridge-v1.schema.json` | Browser/Node legacy generator and export with the same short-idea contract. Offline Node path also ignores free-text direction. Optional cloud/CLI adapters are separate. | Freeze v1 behavior. Native engine must not depend on these modules. Browser retains its working legacy mode until an explicitly designed v2 transport exists. |
| Native CMake/tests and CI | C++20, pinned JUCE8.0.8; 12 Linux /11 Windows CTest targets. Native workflow builds Windows/Linux VST3/Standalone and runs actual HTTP fixtures; Node20/22 matrix and separate Playwright UI workflow exist. | Add focused native engine tests to the existing CMake/CI; retain all old tests. No new runtime dependency needed for procedural phases. |

Existing evidence is scoped: allocation interception covers prepared MIDI buffers, not arbitrary host buffers or the entire wrapper. REAPER/pluginval/OS screen-reader acceptance remains unverified. Two previously recorded minor drag lease/CLOEXEC issues remain under T13. T12–T15 remain open.

Fresh phase-1 baseline: `npm test` in this worktree passed42/42. The original native build has identical C++ source; its CTest result is recorded in task.md after execution, not claimed as a new worktree build. No native source or dependency is changed by this design.

Remote UI-source verification: native [43399121262](https://github.com/Aiorro/LoopHorge/actions/runs/43399121262) passed Windows/Linux at3493777; browser [43400295896](https://github.com/Aiorro/LoopHorge/actions/runs/43400295896) passed atbf93d10. The latter Node matrix [43400295998](https://github.com/Aiorro/LoopHorge/actions/runs/43400295998) failed one Windows22 CLI availability test (`where` check returned unavailable after2222ms, its deadline is2000ms); other five matrix jobs passed. This is an observed availability/deadline failure, not yet a proven root cause. Keep it tracked separately from engine work; do not describe the baseline CI as wholly green.

## Approaches and recommendation

1. **Expand the existing idea schema and fixed arrays.** Smaller immediate diff, but silently changes bridge-v1, conflates bars with quarter beats, and duplicates large fixed storage across every processor. Reject for full songs.
2. **Introduce a native composition-v2 core alongside legacy playback, with explicit adapters.** Recommended: preserve v1 contracts and deploy independently tested slices, then connect v2 playback/UI/export atomically. Migration and two compatibility paths add work, but keep old projects usable throughout.
3. **Put long-form generation behind Node or a mandatory model.** Could reuse the web generator, but violates native offline operation and adds service/model installation dependence. Reject as the baseline; existing providers remain opt-in.

The user approved approach2 and this architecture on2026-11-13.

## Composition-v2 and time semantics

Use native typed value objects under `plugin/Source/engine/`, namespace `composer`. JUCE stays at the application/serialization boundary; musical algorithms use C++20 standard types. Intended units: `Config.h`, `Composition.h`, `Validation.h`, `LegacyMigration.h`, `CompositionJson.h`, `Theory.h`, `Harmony.h`, `Motif.h`, `Rhythm.h`, `Styles.h`, `Prompt.h`, `Mood.h`, `Arrangement.h`, `Generation.h`, `PreparedSong.h`, `SongPlayback.h`, `MidiExport.h`, `Neural.h`. Create each unit with its owning behavior/test, not speculative empty scaffolding.

Core document fields:

- `schemaVersion=2`, stable composition ID, `engineVersion`, explicit uint32 seed, resolved config and generation provenance.
- `Tick` is a signed64-bit integer, quarter-note PPQ480 initially. `Seconds` is an explicit duration type; never interpret bars, seconds or ticks interchangeably. Legacy `start`/`duration` remain quarter beats in v1.
- Total `endTick`; ordered tempo, meter and key maps beginning at tick0; section and chord timelines; stable-ID track collection.
- Tempo points use integer microseconds per quarter; piecewise constant, no ambiguous tempo ramps in the first v2 version. Meter has numerator/denominator; allowed denominators2/4/8/16, numerators1–12. Meter changes occur on a bar boundary of the preceding meter. A partial final bar is allowed and labeled.
- Track: ID, name, musical role, JUCE channel1–16, optional GM program/bank, instrument range, notes and CC. Drums use channel10; percussion may share10 only with disjoint declared pitch-ownership sets (default drums35–59, percussion60–81), checked across the whole document. Shared channel10 uses a fixed GM percussion configuration with no emitted bank/program or CC events on that channel; drum dynamics use note velocity. Reject channel10 controller/program changes when sharing is enabled. This prevents both same-pitch lifetimes and cross-track setup/controller ordering dependence in exported MIDI. Dedicated single-owner channels retain their documented CC/preset support. Melodic tracks never silently share channels or use10. Same-channel program/bank conflicts are rejected.
- Note: stable ID, pitch0–127, velocity1–127, `startTick>=0`, `durationTicks>=1`, endpoint at/before `endTick`. CC: tick, controller0–127, value0–127, at most16 distinct non-bank controllers per channel. Bank-select CC0/32 is represented by instrument bank events, not duplicated as generic CC. Initial support excludes channel-mode CC120–127, which would invalidate note lifetime tracking. No SysEx or arbitrary scripts.
- Chord: span, root, pitch classes, inversion/voicing, tension and borrowed/modulation explanation. Key changes accompany approved controlled modulations. Sections have role, span, motif references and energy/density/instrument targets.

Maps must be ordered, nonempty at origin, free of duplicate tick entries and within the document. IDs are unique and stable across unrelated edits. Reject overflow, NaN/nonintegral numeric input and unknown incompatible versions before allocation/publication. Validate every event/interval and resource total; never truncate silently.

Seconds-to-ticks uses piecewise tempo integration. For a requested duration, choose the nearest tick whose converted duration matches within half a tick at the local tempo; the final section can be partial. Musical structure may optionally propose a rounded duration, but only with the difference displayed before generation. Meter affects bar addressing, not the definition of a quarter beat. Store requested and actual duration separately.

### Proposed initial resource envelope

| Resource | Limit and failure behavior |
| --- | --- |
| Duration | Up to30 minutes AND2,000,000 quarter beats; reject combinations exceeding either bound. No promise of unlimited songs. |
| Tracks/channels | Up to16 tracks, JUCE channels1–16; channel10 reserved for drums/percussion. |
| Music events | Up to131072 notes and65536 CC total; up to4096 points in each map;1024 sections;16384 chord spans. Reject oversize before publishing. |
| Serialized state |32MiB v2 UTF-8 bytes, bounded nesting/collections/text lengths; legacy v1 parser stays4MiB. |
| Document memory |64MiB for one validated document plus its prepared representation; enforce count-based checked-size accounting before reserve/resize and measured peak-allocation tests. |
| History/workspace |32MiB retained immutable history blocks; oldest entries evicted visibly. One active document, at most one worker candidate, and three prepared slots; total native composer budget256MiB. Budget exhaustion leaves current composition intact. |
| Jobs | One active generation and one replacement request per instance; cancellation checked at least everybar/track and before publication. Default30s deadline, adjustable up to120s; timing is a limit, not a performance promise. |
| Playback density | At most256 simultaneously held channel/pitch keys; per-block cap2048 output messages including releases/reset. The v2 validator rejects documents exceeding this active-key density for every operation, including import/export, with an actionable message. There is no export-only accepted state. Rejection leaves the current document, playback and undo unchanged. No silent dropped-note success. |

Size accounting includes vector capacity and indexes, not just logical note counts. Actual peak memory must be measured before exposing long-form defaults. Resource settings may be reduced if Windows measurements require it; increasing the hard bounds requires corresponding stress evidence.

## Compatibility and state migration

Keep `bridge-v1.schema.json`, old endpoints and `wire::schedule()` unchanged. Composition-v2 is a native state/file schema, not a claim that bridge-v2 already exists. An optional future bridge-v2 requires its own request/response schema and Node integration tests.

Keep version1 projects on the current idea path, preserving exact stored beat values, selection, editing and seeded generation. Explicitly entering Composer mode imports a legacy idea into v2: multiply quarter beats by480 and round endpoints independently with minimum one tick; preserve channels1/2/3, names, key/mode/style, seed and tempo. Show timing quantization to1/480quarter note; keep the original v1 payload as provenance. Do not silently quantize an old project just by opening it.

New plugin state envelope: `version=2`, `activeMode`, optional legacy idea payload and optional `composition` v2. Validate the whole candidate before replacing the current state; unsupported/corrupt/over-budget state preserves the current project and reports a sanitized error. Old plugin binaries cannot load v2 state; explain this on first save in Composer mode. No lossy v2→v1 auto-downgrade; explicit short-three-track export is a separate opt-in operation, not required for the baseline.

Restore increments revision/job generation, cancels pending work and rejects late results. Editor destruction only detaches its observer; processor-owned worker can finish/cancel safely. Processor destruction cancels work, joins outside RT and destroys slots/history outside callbacks. Model cancellation must be demonstrably bounded before enabling an adapter.

## Deterministic musical pipeline

`interpret(prompt, manualOverrides) -> ResolvedConfig` produces typed `CompositionConfig`, recognized spans, per-field source (`text`, `manual`, `default`), conflicts and unsupported fragments. A replaceable interpreter interface owns no network capabilities. Rules normalize RU/EN case, common inflections and musical synonyms; parsing limits4096 UTF-8 bytes. Defaults: C natural minor,100BPM,4/4, nu-metal,16bars for loop /180seconds for song, humanization20/100; seven mood axes default50/100. Missing seed is chosen once and shown/stored before generation. No fresh randomness downstream.

Manual fields have an explicit Auto-from-text toggle. Precedence is an explicitly pinned manual value > recognized text > documented default. Two incompatible explicit values in text produce a visible conflict requiring correction; unsupported requests display diagnostics rather than claiming understanding. Initial genres retain trap/boom-bap/r&b/ambient/nu-metal; normalized two-genre blends use explicit weights summing100. Unsupported styles are disclosed; there is no arbitrary language comprehension claim.

`generate(config, baseDocument?, scope, cancellation, progress) -> GenerationResult` runs only on the worker. Use versioned deterministic integer PRNG substreams derived from seed, operation, role, section and motif ID; never use implementation-defined `std::hash` or shared consumption order. Quantize probabilities/humanization to integer ticks/velocities so identical seeds do not depend on floating-point PRNG thresholds across platforms. Check golden fixtures on Windows and Linux; intentional algorithm changes bump engineVersion.

Pipeline order:

1. Resolve duration, genre blend, section structure and energy arc.
2. Plan one shared harmonic timeline: triads, sevenths/extensions, inversions, bounded voice leading, harmonic rhythm, explicit borrowing and controlled modulation. Theory includes12roots, major/natural/harmonic/melodic minor, dorian/phrygian/lydian/mixolydian, major/minor pentatonic and blues. Melodic-minor ascending/descending behavior is an explicit policy. Borrowed tones are attributed, not falsely flagged as basic-scale violations.
3. Build persistent motif IDs with contour/phrase boundaries; develop repetition, variation, call/response, displacement and cadence/resolution. Bass/lead/countermelody read the same chord context, with role-specific register/leap constraints.
4. Generate genre-specific groove grids, swing, accents, ghosts, fills, half/double-time and supported polyrhythms. Humanization stays inside phrase/section bounds and preserves required ordering/endpoints.
5. Realize drums, percussion, bass, chords, lead, countermelody, arp, pads and additional harmony/instrument tracks according to section roles. Enforce channel/range/resource limits and joint validation.

Mood axes change concrete controls: darkness affects mode/borrowed-color/register; energy density/section participation; melancholy contour/cadence/voicing; tension extensions/dissonance-resolution; aggression accents/articulation/rhythm; atmosphere sustain/spacing/pads; complexity rhythmic subdivisions, harmonic rhythm and motif transformations. Expose the derived controls in metadata/tests. Test directional differences using fixed config/seed comparisons, not an unsupported claim that every axis is mathematically monotone for every note.

## Scoped operations and undo

Supported operation enum: compose, loop, generateTrack, regenerateBars, extend, variation, harmonizeMelody, rearrange, changeMood, changeStyle. A `GenerationScope` is explicit track IDs plus half-open tick intervals and a preserve-existing policy. Generate/harmonize read surrounding harmony/motifs as context; only selected content may be written. A change to global key/tempo/meter/section structure requires whole-composition scope or an explicit selected-map edit, never an incidental side effect of track regeneration.

Notes crossing a selection boundary remain unchanged unless explicitly selected as whole notes. Do not split/truncate an unselected sustained note. With preserve-existing enabled, all existing note IDs and fields remain unchanged; add new notes only in allowed uncovered positions. Extend leaves all content/maps before the old endpoint unchanged; append harmony/sections/notes to the right. Variation copies the source document to a new result; it does not mutate the original. Harmonize preserves the input melody.

Worker captures a revision and immutable base. Apply only if revision, job ID and active mode still match. Cancellation, timeout, error, stale revision or budget rejection publishes nothing. A completed operation creates one undo transaction; undo restores notes, maps, config and section metadata. Use immutable per-track/event blocks with copy-on-write at the edit boundary and byte-bounded history. Stable unselected blocks can be compared directly and hashed in tests. Do not adopt full-song `juce::var.clone()` for each mouse move.

## Prepared playback and realtime ownership

Long-form playback has its own `PreparedSong`, never `Schedule::events`. The worker validates/compiles contiguous sorted event vectors and bounded time indexes; precomputes tempo-segment conversion and track/channel metadata. It owns three slots with states free/writing/pending/active. A slot owns its dynamic storage for its entire published lifetime. Callback exchanges integer slot indexes only; it never copies vectors, increments shared ownership, destroys a document or allocates/frees memory. Worker reclaims only a slot atomically returned free by the callback. Use acquire/release ordering and explicit invariants/tests for replacing an already pending slot.

Host transport: song origin is absolute host PPQ0 by default; an explicit stored `hostStartQuarterBeat` offset places it elsewhere. Display that origin in Composer mode; pressing Play never silently reanchors the song. Before the origin and after the song end, emit no song notes and flush previous playback. Host PPQ/BPM controls timing, independent of the composition tempo map; retain the map for export. Standalone: follow the composition tempo map. Full song stops at end and flushes; explicit loop mode repeats its chosen range. Seek/loop/stop/replacement flush before processing the next interval. At seek/start inside a song, flush note/sustain state, chase the latest bank/program and at most16 non-bank CC values per channel, then start currently spanning notes once per active channel/pitch using the latest onset velocity and restored overlap count. Released notes held only by historical pedal are deliberately not resurrected; disclose this seek policy. A worker-built interval index returns at most256 active keys, avoiding a full-song scan in the callback. The chase bundle is bounded to48 bank/program +256 CC +256 NoteOn messages, fitting the ordinary budget before384 reserved releases. Apply setup before notes; skip duplicate chase/onset at the exact target tick and test both sides of the boundary. Binary-search sorted prepared events at discontinuities; use a cursor for continuous blocks. Conversion must bound work for tiny32/64sample and exceptionally large blocks and for multiple tempo changes within a standalone block.

Use a shared16x128 counted note ledger with the current retrigger semantics: NoteOff before repeated NoteOn, final NoteOff when the last overlapping lifetime ends. Cap prepared simultaneous distinct active keys at256. Reserve384 of2048 output messages for emergency releases/controller cleanup, sufficient for256 keys plus defined sustain resets. Ordinary processing stops before consuming the reserve; on overflow flush all held keys, reset sustain and pause until a safe restart, with a UI diagnostic. CC64 ownership follows the same channel and reset rules. Prove event-budget arithmetic for stop/seek/dense blocks; legacy's three-channel384 reserve is not proof of v2 safety.

Keep the pinned wrapper's32768-byte MIDI reservation and test prepared-buffer assumptions. Do not claim allocation-free behavior for an unprepared caller-owned MidiBuffer. No AI/network/disk/mutex/wait in either callback. Initially keep the simple audition synth but make it bounded across enabled channels/voices; GM programs annotate MIDI and do not promise realistic sampled instrument audio. Drum preview capability/limits must be visible rather than silently playing a pitched sine as a drum kit.

## UI, MIDI export and optional neural boundary

Extend the existing JUCE window with Legacy Ideas / Offline AI Composer modes. Composer has loop/song/selection actions, direction text, explicit Auto/manual fields, mood controls, instruments, seed, parsed-parameter summary and diagnostics, progress/cancel. Keep keyboard editing/focus/labels, MIDI export/drag and undo. Song section/track selectors feed the same piano editing surface, with horizontal viewport/zoom and visible-range note queries. Native UI and worker communicate through processor-owned immutable snapshots/job status. Browser polishing already shipped remains intact; do not add a browser dependence to native generation.

The v2 encoder writes SMFtype1, conductor tempo/meter/key maps and one track per instrument with name/program/bank/CC/note events. PPQ480 initially. Deterministic ordering: necessary releases before retrigger at the same tick, defined setup/program/CC order, and EOT at document end on every track. MIDI1 cannot represent independently voiced identical pitches on one channel: use the same retrigger/merged-final-release policy as playback and document it. Each melodic channel has one track; shared channel10 tracks have disjoint pitch ownership and fixed GM configuration without channel10 bank/program/CC events, so note lifetime and setup/controller ordering never depend on inter-track SMF merge order. Read files back with an independent decoder and compare audible lifetimes, channels, maps, endpoint and nonzero durations. Cache receives bounded immutable encoded bytes; keep retention/lease/URI protections and explicit oversize error.

`INeuralComposer` exposes capabilities, local-model validation, bounded generate/cancel and structured unavailable/error results. The procedural generator is a separate backend, never a fake neural implementation. Phase5 research records a real symbolic model's official source, license/redistribution, tokenizer/event vocabulary, supported conditioning, context length, memory, native CPU runtime and bounded cancellation. Test actual converted weights/runtime before advertising availability. No model is bundled/downloaded by this design; missing/invalid/unsupported models leave full procedural functionality available and report why neural mode is unavailable. If an in-process runtime cannot cancel safely within the deadline, isolate that optional inference in a managed native child process or keep it disabled; never kill a thread inside JUCE.

## Integration sequence and acceptance boundaries

E01 yields this audited architecture and requirements archive, followed by explicit approval. After approval, detailed executable plans are written per dependency slice, with failing test → minimal implementation → actual verification → review → small commit. Main task.md owns all completion checkboxes.

E02 starts with typed v2 data, checked validation and explicit legacy import fixtures, then a shared harmony/motif/groove engine generating validated small native documents. E03 adds deterministic prompt interpretation, mood derivation and resolved-parameter UI without replacing legacy playback prematurely. E04 connects full arrangement, resource-bounded prepared playback, v2 state/editing/history/UI and MIDI export/drag. E05 validates the optional neural boundary/model feasibility. E06 runs broad regression, musical and independent code review, Windows/Linux CI and lifecycle/RT evidence. T12–T15 remain separately scoped; no release is authorized.

Builder check: a fresh independent C++/JUCE reviewer identified three consequential ambiguities. This revision resolves them as engineering proposals: over-density always rejects atomically (no export-only state); host origin is explicit with bounded controller/note chase; shared-channel10 tracks have disjoint pitch ownership and fixed GM configuration without bank/program/CC events. A follow-up reviewer check exposed the remaining cross-track controller-order dependency, prompting the stronger shared-channel restriction. None requires relaxing the supplied requirements. User approval was received on2026-11-13.

First product slice after approval: v2 value model/validator and explicit import of the existing `plugin/tests/idea.json` fixture. Acceptance includes exact channel/tempo/seed preservation, disclosed PPQ quantization, valid endpoint, malformed/version/resource rejection and no legacy contract changes. Implementing this first proves the compatibility foundation before musical expansion.

Verification matrix for subsequent plans:

| Risk or requirement | Required evidence |
| --- | --- |
| Offline/no-model behavior | Real native generation with optional providers/network unavailable; a forbidden-transport test seam fails on any offline network attempt. No Node child launched. |
| Parser/conflicts/defaults | RU/EN fixtures, synonyms/compound prompts, invalid duration/BPM/meter, unknown phrases, manual precedence and conflicting text diagnostics. |
| Musical coherence | Theory/chord/voice-leading/role-range invariants, explicit borrowed tones; motif identity across sections; distinct fixed-seed genre grooves; manual listening/MIDI review with limitations recorded. |
| Determinism | Cross-platform golden documents/hash for same engineVersion/config/seed, seed changes and independent role substreams. |
| Duration/maps | Non4/4 meters, changes at boundaries, piecewise tempo seconds/ticks roundtrip, partial final bar,30min near limits and overflow inputs. |
| Scope/history | Byte/equivalent-ID checks on unselected content including sustained boundary notes, preserved melody, extension prefix, one-step undo/redo and stale worker results. |
| State/export | Unmodified v1 reopen; explicit v1 import; v2 roundtrip; corrupt/over-budget state while generating; independent long-song MIDI decoder, CC/program/maps/EOT/retrigger. |
| Worker/RT | Cancellation during each pipeline stage; editor close/processor destroy/restore races;32/64samples, changing tempo, negative PPQ/seek, dense16-channel releases, overflow policy, prepared buffer allocation/free interception and peak-memory stress. |
| Regression/quality | Existing Node42/native12Linux/11Windows + HTTP/UI suites; real VST3/Standalone builds, Windows CI, independent review. Manual DAW/pluginval and live model tests are claimed only if actually run. |

## Decisions requested

Approve the recommended separate native v2 core with explicit legacy import, PPQ480, the proposed resource envelope, Auto/manual precedence and staged integration above. No model selection, weight download or release approval is requested. All implementation and verification claims remain pending until their corresponding E02–E06 evidence exists.
