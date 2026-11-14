# LoopForge native prototype

JUCE 8.0.8, C++20; VST3 instrument with MIDI output and Standalone. This is an early native prototype, not a production release or a validated replacement for any commercial product. Review JUCE's AGPL/commercial licensing requirements before distribution.

## Build

Windows: install Visual Studio 2022 with Desktop development with C++ and CMake 3.22+. Linux: install a C++20 compiler, CMake, ALSA, FreeType/fontconfig and X11 development libraries (see native.yml).

```sh
cmake -S plugin -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --config Release --parallel 2
ctest --test-dir build/native -C Release --output-on-failure
```

CMake fetches an exact JUCE commit. An existing JUCE checkout can be selected using `-DLOOPFORGE_JUCE_PATH=/absolute/path/to/JUCE`. Outputs are under build/native/LoopForge_artefacts/Release/{VST3,Standalone}. Copy the complete VST3 bundle into the host's VST3 search directory. CI is configured to upload unsigned Windows and Linux artifacts; a configuration is not evidence of a successful remote run.

## Compose

Choose **offline** to generate eight native ideas without Node, credentials or network access. Select key, scale, style, bars and tempo, then click Generate. This uses the same rule-based composition approach as the browser engine; prompts are used by AI providers.

For optional AI providers, run `npm start` from the repository root and keep the loopback Node service on port 4177. Choose a provider, optionally enter a concrete model ID (blank uses its default), enter a prompt and click Generate. **Test (1 idea)** performs a real generation to check connectivity, credentials and model access without replacing the current composition. This can consume cloud/CLI quota or incur charges; it is never run automatically. **Cancel request** closes the worker socket and propagates cancellation to the server and CLI process tree. HTTP authorization, model and quota errors appear in the status line without upstream response bodies. Select one of eight ideas, use the host transport, or Play in Standalone. Export MIDI writes separate bass/chords/melody tracks. Piano roll edits the active bass/chords/melody layer; other layers remain visible. See the editing walkthrough below. Audition synth is a simple sine preview, and can be disabled when routing MIDI to another instrument.

API keys belong only in the Node server process environment (`OPENAI_API_KEY`, `ANTHROPIC_API_KEY`, `OPENROUTER_API_KEY`). Ollama requires a running local service. CLI bridges require locally installed, authenticated official CLIs. A subscription is not an API key. All composition requests run on a worker thread, never in processBlock.

MCP clients can launch `node /absolute/path/src/mcp-server.js` over stdio to generate/export ideas using the same server-side engine. The native plugin uses the HTTP API, not an MCP model connection; the MCP client must explicitly call tools. MIDI files exported by MCP can be used in a DAW. Direct native loading of MCP-generated ideas is not implemented.

## Composer-v2 developer API

`Source/engine/Generation.h` contains the native C++ loop core for the approved Hybrid Composer architecture. It returns validated composition-v2 data with a shared chord/key timeline and bass/chords/lead tracks, plus optional General MIDI drums. It has no Node, network, model or JUCE dependency. Call generation on a composition worker, never in `processBlock()`:

```cpp
composer::Config config;
config.seed = 42;
config.drums = true;
config.feel = composer::Feel::half;
auto result = composer::generateLoop(config);
if (result.composition) {
    // Consume the validated candidate outside the audio callback.
} // Otherwise result.error describes the rejected configuration or resource limit.
```

The core supports twelve roots, eleven scales, bounded chord inversions/voice leading, shared harmony, motif call/response, five genre grooves and explicit borrowing/modulation. `harmonicBars` selects automatic, one-, two- or four-bar harmony independently of time feel. A modal-interchange request needs at least two harmonic spans so borrowing can resolve; incompatible settings return an error.

`Feel::half` stretches the source rhythm over two bars; `normal` retains it; `doubleTime` repeats it twice per bar. BPM, duration and harmonic timeline stay unchanged. Swing follows the original pulse phase and scales with feel. Bass, lead and drums share this timing transform. Neutral normal-mode musical output remains unchanged; the mood-capable algorithm is identified as engineVersion3/source-v3. Its canonical field/seed identity encoding is bounded and independent of platform byte order.

`drums` defaults to false for existing three-track callers. Enabling it appends channel10 without changing melodic notes, IDs or seed streams. The fixed GM kit owns pitches35–59; no bank/program/CC events are emitted. `ghostNotes` enables quiet snare pickups independently of complexity; `drumFills` enables transitions on every fourth bar. Both default to true and apply only when drums are requested. Same-pitch/onset collisions merge and note gates end at or before the next same-pitch attack. All output remains within the chosen meter/bar and document limits.

The current application controls, audio scheduler, saved-state format and MIDI export still use the legacy idea path described above. These developer APIs do not make v2 drum preview, full-song editing/playback or v2 export available in the UI. See [task.md](../task.md) for integration status and [the architecture](../docs/superpowers/specs/2026-11-13-hybrid-engine-design.md) for ownership/resource contracts. `LegacyMigration.h` provides explicit, loss-disclosing legacy import; existing projects are never converted automatically.

`Source/engine/Midi.h` exports a validated composition-v2 on a worker without JUCE, file I/O or network access:

```cpp
auto midi = composer::encodeCompositionMidi(*generated.composition);
if (!midi) { /* show midi.error; no partial bytes are returned */ }
else { /* write midi.bytes to the chosen file outside the audio thread */ }
```

Output is standard MIDI type1/PPQ480, one conductor plus all source tracks with their original channels, names, bank/program, CC and tick endpoints. Tempo/meter/key changes and section-start markers are exported. Every track retains the full duration including trailing silence. Notes sharing a pitch use counted lifetimes: a later onset retriggers, an earlier end cannot silence the later note. Simultaneous unisons share one onset (last source velocity) and remain held until all their intervals end. Source data is immutable; unsorted note/CC collections are stably ordered for output. Touched sustain/sostenuto/hold2 controllers64/66/69 are reset at the endpoint.

SMF has only major/minor key-signature metadata. Every key point therefore also carries exact `LoopForge key: <tonic> <scale>` text. Diatonic modes use their relative-major accidental set; harmonic/melodic minor use the tonic minor signature; pentatonic/blues use the tonic major/minor base. Text preserves the exact mode; importer display/interpretation varies. Chord analysis/provenance/IDs are not a MIDI project-state format. Gaps exceeding the SMF four-byte delta limit use zero-payload sequencer-specific meta padding. Encoding rejects invalid/resource-exceeding documents and is capped at32MiB. This API is not called by the current UI, playback or drag path; v2 state and application integration remain pending.

### Direction and mood API

`Source/engine/CompositionJson.h` provides exact document persistence on a worker:

```cpp
auto saved = composer::encodeCompositionJson(composition);
if (!saved) { /* show saved.error; saved.json is empty */ }
auto loaded = composer::decodeCompositionJson(saved.json);
if (!loaded.composition) { /* show loaded.error; keep the current project */ }
// Publish the validated candidate only through the future worker/revision handoff.
```

The canonical UTF-8 schema stores every current Composition field, including IDs, source/legacy provenance, maps, chord analysis, section roles, original note/CC order, track ranges, ownership and optional presets. Numeric track/event/section/chord IDs are positive canonical decimal strings through UINT64_MAX; the composition ID remains an opaque string. Numeric fields require integer JSON tokens (fractions/exponents/strings/bools are rejected). Role/scale/section role enums use explicit kebab-case names; program/bank are explicit null when absent, and ownedPitches is a128-character binary mask (pitch127 first). All fields are required; arbitrary object field order is accepted. Unknown versions/fields, duplicate names including escaped aliases, missing fields, invalid Unicode/UTF-8/NUL, bad types, invalid model data and collection/byte overflows fail without a partial document. No lossy normalization or automatic v1 conversion occurs.

Encoded/input JSON is capped at32MiB, decoded legacyJson at4MiB, metadata text at1024bytes and the composition ID/track/section names at128bytes; existing model budgets apply. Schema-directed parsing constructs bounded typed arrays directly without a generic DOM or input-controlled recursion. Large valid documents whose escaped representation exceeds32MiB cannot be saved in this format and return an explicit error. Reading/writing allocates and belongs outside processBlock. This is the composition-file codec, not yet the plugin's version2 state envelope: existing version1 presets, bridge-v1, restore/job cancellation, playback, history and UI still use the established path. Old plugin versions cannot read composition-v2 files. The complete project envelope and validated atomic publication remain E04 integration work.

Shared model validation uses a balanced ID tree so attacker-selected numeric IDs cannot turn uniqueness checks into quadratic hash-collision work. Error strings are sanitized, and allocation failures are caught on a best-effort basis; these allocating APIs are not noexcept and cannot guarantee recovery under sustained global allocator exhaustion. The codec performs no file writes and does not provide filesystem atomicity/crash recovery.

`Source/engine/Prepared.h` compiles a validated document on a worker. `prepareComposition(composition, optionalCancellationFlag)` returns a move-only `PreparedSong` or a sanitized error. The worker owns all vectors and must retain the owning, immutable value for every query/render; do not use moved-from handles or replace/free an active value. Actual vector capacities plus source storage must fit64MiB. Preparation checks cancellation between bounded stages; sorting/validation are bounded worker work, not an interruptible audio job.

Prepared data includes note-state/CC lanes, stable time-sorted event references and tempo segments. `chase(Tick)` returns at most256 active keys with exact32bit overlap counts/last-onset velocity, up to256 latest CC values and16 presets. Its state includes events at the target; `firstAfter(target)` skips those events to prevent duplicate exact-target onsets. Querying before0/at-or-after end returns no chased notes/controllers. Fixed lane indexes avoid a whole-song note scan. Controller snapshots retain the relative chronological order of their selected latest values; they do not replay the complete controller/pedal history. Notes released before a seek and held only by past pedal activity are deliberately not resurrected.

`Source/engine/Playback.h` renders bounded MIDI packets using that immutable source:

```cpp
auto prepared = composer::prepareComposition(composition); // worker only
composer::Playback playback; // persistent, fixed-size render state
composer::PlaybackBlock block;
block.clock = composer::Clock::compositionSeconds; // or absolute hostQuarterBeats
// packetSink(MidiPacket, int sampleOffset) must be noexcept and preallocated.
if (prepared.song) {
    auto status = playback.render(*prepared.song, block, packetSink);
    block.position = status.nextPosition;
} else { /* report prepared.error; keep the active source */ }
```

The caller supplies host PPQ or standalone seconds for each block. Host BPM controls host timing independently of the source tempo map; hostStartQuarterBeat is an explicit absolute origin, default0. Standalone uses precomputed tempo-map conversions. Each onset is emitted on the first sample at/after its tick. Constant-step runs use an integer-frame anchor to avoid accumulated block rounding; step changes/discontinuities reanchor to the supplied position. Chase and sample offsets compare forward event times rather than adding an epsilon that could admit a future event. Continuous blocks use a cursor; seek/start/source/clock/origin changes flush/chase. A final old lifetime releases before same-tick CC/new onset; overlapping retriggers retain the existing counted semantics. Endpoint CC events run under the normal budgets before cleanup, including when their first sample lies in the next block. Stop/end/overflow releases counted notes and resets CC64/66/69 on touched channels. Full songs stop once at the endpoint; transport jumps act as seeks, with explicit loop-range controls still pending. Zero-sample blocks leave render state untouched; invalid clocks flush and report an error.

Each block is capped at2048 MIDI messages, reserving384 for cleanup; worst cleanup is256NoteOff+48pedal resets. The ordinary1664 budget also limits processed event references, including invisible overlap-count changes. Maximum chase is48bank/program+256CC+256NoteOn messages. Overflow releases all active state and stays paused across continuous playing blocks until stop/start, seek or a new source; the caller must surface status. Fixed-sink Linux tests intercept new/delete during cold/steady/seek/stop/dense/overflow rendering and indexed queries. This is not a whole-JUCE-wrapper allocation claim: actual MidiBuffer capacity, slot publication/reclamation, audition voices and transport/UI/state integration still require their integration tests. These APIs are not yet called by the current Processor; its legacy rendering remains unchanged.

`Source/engine/Publication.h` adds `PreparedMailbox`: exactly three free/writing/pending/active slots for one serialized producer and one non-reentrant consumer. `publish(std::move(song), tag)` consumes the candidate only on `PublishStatus::published`; busy/budget rejection preserves candidate ownership and current active/pending data. Accepted pending publications replace older pending ones. The retained-payload budget includes the new candidate until a superseded value is retired; reserve for transient active/pending/writing values. Its default/hard ceiling is64MiB and may be reduced. `retainedBytes()` is producer-only accounting for engaged PreparedSong storage/capacities; fixed mailbox/control storage and the complete256MiB workspace need separate owner accounting.

The producer uses `clear()` to post a tombstone and `reclaim()` to destroy returned free-slot payloads. Consumer `beginBlock()` exchanges only an index and reads the immutable source/tag; a view is valid only until its next acquisition. Tags are opaque caller metadata, not stale-job/revision protection. `resetStopped()` and destruction require both sides to have stopped. All optional/vector destruction is producer/control work; never call producer/reset/reclaim operations from the render thread.

`Source/ComposerPlayer.h` is the JUCE callback component owning that mailbox and Playback. Producer methods forward publication/clear/reclamation. `process(block, midi)` consumes once per nonzero block, clears input MIDI as the legacy processor does, renders raw two/three-byte packets, and reports playback status/tag/source presence. Clearing the source flushes via `Playback::stop()` without dereferencing retired song data. Zero-size blocks clear MIDI but leave publication/transport untouched. Stop/replacement/clear messages are included in the same2048 output bound.

```cpp
composer::ComposerPlayer player; // owner outlives worker and callbacks
juce::MidiBuffer midi;
midi.ensureSize(32768); // preparation only, outside callbacks
// Worker: prepare a song, check it, then publish; handle busy/budget without losing it.
// Callback: reuse a prepared buffer; do not ensureSize/reclaim/reset here.
auto rendered = player.process(block, midi);
```

The caller/wrapper must reserve32768bytes for every supplied buffer beforehand. The adapter does not inspect or repair arbitrary unprepared buffer capacity in a callback. At most2048 packets of<=3bytes need<=18432 JUCE event bytes under pinned8.0.8. Linux tests intercept malloc/calloc/realloc/free plus throwing/nothrow new/delete around actual callback processing, including fresh prepared buffers, concurrent producer reclamation,256note/256CC setup,304release and dense overflow. VST3 conversion counts preserve every packet, including presets/controllers. ASan/UBSan/TSan instrument the component/header code; linked JUCE and the separate SDK conversion check remain uninstrumented. These components are still separate from current Processor activation, v2 project-envelope restoration, UI, audition and full workspace/DAW acceptance; the application continues to use its working legacy path.

`PreparedMidi.h` appends validated, ordered short channel packets directly to pinned JUCE raw storage. Standard addEvent repeatedly searches all earlier events even for already sorted input; a local256-key seek measurement exceeded600microseconds before this change. The ordered sink has linear append work, rejects invalid lengths/status/data/order/count and reports `outputRejected`; valid Playback output never triggers that flag. CMake compares normalized SHA256 of both MidiBuffer.h/.cpp before exposing the `loopforge_prepared_midi` interface target/macro. Both files are registered as configure dependencies, so an ordinary incremental build also reruns verification after edits. Consumers must link that target; JUCE source/layout changes intentionally fail configuration until reviewed. Actual encoded bytes are checked against standard JUCE for all16channels, two/three-byte packets and the2048event limit; LF/CRLF compatibility, explicit source-drift rejection and configure→mutate→build rejection for each file have CTest coverage. JUCE checkout files are not modified.

For a reproducible local callback profile, build the optional `composer_player_bench` target and run its binary. It checks the560/864-message cold/seek workload before printing median/p95/max for32samples at48kHz. Timing thresholds are not asserted in shared CI, and local timing does not certify a driver, wrapper or full plugin deadline.

`PromptCompose.h` adds a local RU/EN direction interpreter and supported-loop adapter. Sources are UTF-8; compile consumers with `/utf-8` on MSVC, as the prompt test target does. The caller supplies a seed chosen once; parsing itself never chooses fresh randomness.

```cpp
composer::LocalPromptInterpreter interpreter;
composer::ManualOverrides manual;
manual.values.recipe.bpm = 110;
manual.pinned.set(size_t(composer::Field::bpm));
auto direction = interpreter.interpret(
    "dark trap in D minor, 8 bars, 100 BPM, piano and drums", 42, manual);
auto result = composer::generateResolvedLoop(direction);
```

This resolves BPM110 from the manual pin and retains the other text settings. `origins` identifies default/text/manual values; `recognized` contains original byte spans and normalized values; `diagnostics` retains conflicts, unsupported negation and unknown fragments. Pinned fields override text for that field only. Explicit numeric values override adjective hints; incompatible values of equal specificity require correction or a pin. Structural UTF-8/input errors and unrelated field errors remain errors. Input is capped at4096UTF-8 bytes, with strict Unicode scalar validation and no NUL/control data.

Defaults are C natural minor, nu-metal,100BPM,4/4,16bars, humanization20 and seven mood axes50; explicit song mode defaults to180seconds. A request's `duration` is authoritative: the `recipe` is a native loop template whose bars are assigned at execution. Common English/Russian genre, key/scale, mood, instrument, duration and performance words are supported, including Cyrillic case/ё normalization, sharps/flats, decimal minutes and checked seed/tempo/meter values. This is a bounded dictionary/grammar, not arbitrary language comprehension. Unknown fragments produce visible warnings. A separated minus before a numeric value is an explicit ambiguity error, so negative tempo/duration cannot become positive data; write signed numbers without a space. Positive instrument mentions form the requested track set; negative-only mentions remove roles from the default bass/chords/lead set. Instrument negation continues through a coordinated list until a sentence boundary or an explicit positive connector such as with/but (с/но); per-role recognition records show include/exclude decisions. Piano maps to the chords role with GM program0.

`generateMoodLoop(recipe, mood)` derives bounded musical controls from darkness, energy, melancholy, tension, aggression, atmosphere and complexity. They affect register, note spacing/density, velocity, articulation, motif contour and chord voicings/tension; mood.complexity is the complexity source for this entry point. The plain `generateLoop` API retains direct low-level control. Neutral mood preserves the note golden; integer/threshold mappings are deliberate and do not promise a change for every single slider increment or every musical context.

`ResolvedConfig::valid()` means the request is internally consistent, not that every requested capability is implemented. The adapter currently executes exact4/8/16/32-bar loops with selected bass/chords/lead/drums, mood and supported presets. Seconds/song requests, section arrangements, additional roles and genuine two-genre mixtures remain exact in the parsed request but return a capability error with no partial composition. Genre weights are validated and missing weights are reported as inferred defaults/complements. These limits prevent silently turning a three-minute arrangement into a short loop; full arrangement, v2 UI/state/playback/export remain tracked in task.md.

## Behavior and limits

A built-in three-layer demo plays without Node. Generated compositions contain at most 1600 notes. A three-slot handoff keeps worker data writes outside the active audio schedule. Playhead PPQ, tempo and play state are used when provided by the host. Active notes are flushed on stops, seeks, loop wraps and schedule changes. Same-channel/pitch notes retrigger with a balanced off/on pair and sustain until the last overlapping note ends; MIDI 1.0 has no independent voice IDs for these overlaps. Seeking into a sustained note does not retrigger it. Native state persists the selected generated composition; provider controls/history are not yet persisted. The default demo is reconstructed when no saved composition exists.

Tests and host validation must be reported separately. Do not claim Windows, REAPER, FL Studio or Ableton compatibility from a Linux compilation. Stress/recovery commands and measured results are in ../docs/VALIDATION.md. Pending: real DAW drag/import acceptance, polished audio envelopes, target-host profiling, installers and real DAW acceptance.

## Callback MIDI memory contract

The pinned JUCE 8.0.8 VST3 wrapper normally reserves only 2048 bytes. `cmake/MidiCapacity.cmake` creates build-local copies of its VST3 preparation code and AudioProcessorPlayer with a 32 KiB MIDI reserve outside processing. It leaves the original JUCE checkout unchanged and fails configuration if the expected upstream preparation anchors change. Both VST3 and Standalone builds must retain this integration. Direct/custom callers of `Processor::processBlock` must also reserve 32768 bytes in each supplied MidiBuffer before processing; the raw JUCE API cannot allocate an arbitrary empty caller-owned buffer outside its callback by itself.

Generated callback MIDI is bounded to 2048 three-byte messages, matching the pinned VST3 converter limit, with reserved capacity for up to 384 active-note releases (each uses 9 bytes in pinned JUCE). An extremely large/dense block that hits the budget releases active notes and pauses generation for the rest of that block; subsequent blocks resume at the host position. A status warning records this condition. Typical 32/64-sample blocks are far below this budget. This intentionally bounds memory even for host blocks much larger than the advertised size.

The Linux allocation test intercepts malloc/calloc/realloc and C++ new during the processor callback, including the cold callback, dense repeats, tempo changes, stops and blocks 32..192000. It tests the prepared-buffer contract, not the entire host, wrapper input conversion, driver or hardware. Those still need target-host profiling. CLI cleanup uses POSIX process-group SIGKILL and Windows 10+ Job Objects with atomic process assignment (PowerShell/.NET launcher); The Windows launcher uses STARTUPINFOEX job-list assignment so cancellation cannot strand a suspended process between creation and ownership. It fails before launch when containment is unsupported. Windows compilation/execution remain unverified by this Linux run. POSIX descendants that deliberately create a new session can escape a process group; these tests cover normal CLI descendants, not an adversarial executable.

## Local service compatibility

The native AI worker now uses `/api/v1/generate` and `/api/v1/probe` with an explicit protocolVersion1 response. Run the updated Node server from the same checkout; an old service is rejected without replacing the selected composition. Browser/MCP legacy routes and previously saved plugin states remain supported. See ../schemas/bridge-v1.schema.json and ../docs/PROVIDER-CONTRACT.md for types, explicit4/4 metadata, bounded errors and migration details. Every returned variation is validated before publication, including those not initially selected.

Native export preserves the complete4/8-bar pattern, including trailing rests, in all four type1 tracks. Notes have at least one PPQ480 tick even when a positive duration rounds below one tick; tempo and4/4 metadata are explicit. The picker follows actual playback/export selection after generation and restoration. Restoring a preset cancels a pending generation so its late result cannot overwrite the restored composition. Native MIDI edits and bounded undo/redo are covered by T10; DAW acceptance remains separate in task.md.


## Native editing walkthrough

1. Choose offline and Generate8Ideas, then select an idea. Choose bass, chords or melody above the roll; scroll vertically for MIDI0–127. Notes in the active layer are bright; other layers remain visible in their track colours. Octave labels use C4=MIDI60.
2. Double-click empty grid space to add a one-beat, velocity90 note. New notes and edited durations snap to0.25beat and stay inside the4/8-bar pattern. Drag a note to move it; drag its right edge (up to5pixels, at most one-third of a short note) to change its duration. A moving note retains its duration. Dragging previews locally; release commits one change.
3. Right-click or Alt-click a note to delete it. Deleting the final note of the entire idea, exceeding800notes/layer or1600notes/idea, or editing a replaced composition is rejected without changing the current idea.
4. Quantize1/4 snaps starts and ends of every note in the active layer, retaining a minimum0.25beat. Undo/Redo retain64steps for the selected idea. New edits clear redo; selecting an idea, generating or restoring state clears history. History is not stored in presets.
5. Play or use host transport to hear the committed edit. Export MIDI captures committed notes, not an unfinished drag preview, and retains separate tracks and full pattern endpoints. Reloading the saved plugin state retains the edited composition. A replacement during a drag cancels the gesture.

These are reproducible manual steps for the native UI; automated message-loop gesture tests and decoded-MIDI checks are not a substitute for actual REAPER/DAW acceptance under T12.


## Drag MIDI to another application

Generate or restore a valid idea, then drag DRAG MIDI at least6pixels into a target that accepts MIDI files. A click alone creates no file. The gesture copies the committed idea captured when you press; later edits or generation do not change that captured export. Normal EXPORT MIDI still opens a save dialog. A reported start does not confirm that the target accepted/imported the MIDI.

Drag files remain in the user's LoopForge/DragMIDI application-data cache for at least24hours. Active native operations retain a read lease even if the editor closes. Stale recognized files can be pruned during a later drag; the cache is limited to256entries/16MiB. If it is busy/full/unwritable or native drag is unavailable, use EXPORT MIDI. If a host permanently references the source path, save a permanent export instead of relying on the drag cache.

The headless tests verify the real file and gesture contract using a native-dispatch adapter; they do not claim OS drag interoperability or REAPER acceptance. Follow ../docs/NATIVE-DAW-ACCEPTANCE.md and record actual host versions/results before closing T12.

## Keyboard and window layout

Fields have visible labels and accessible titles. Resize the window from1000x740 up to1600x1100 to expand the piano viewport. Tab into the roll; brackets select notes, Home/End choose first/last, arrows move, Shift+Left/Right resize and Delete removes. Add/Delete controls work without a double-click or Alt gesture. Ctrl/Alt/Command navigation is not a musical edit; a replacement cancels an old keyboard selection. See ../docs/UI-REFINEMENT.md for verification scope.
