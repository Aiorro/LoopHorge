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

The current application controls, audio scheduler, saved-state format and MIDI export still use the legacy idea path described above. This developer API does not make v2 drum preview, full-song editing/playback or v2 export available in the UI. See [task.md](../task.md) for integration status and [the architecture](../docs/superpowers/specs/2026-11-13-hybrid-engine-design.md) for ownership/resource contracts. `LegacyMigration.h` provides explicit, loss-disclosing legacy import; existing projects are never converted automatically.

### Direction and mood API

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

Defaults are C natural minor, nu-metal,100BPM,4/4,16bars, humanization20 and seven mood axes50; explicit song mode defaults to180seconds. A request's `duration` is authoritative: the `recipe` is a native loop template whose bars are assigned at execution. Common English/Russian genre, key/scale, mood, instrument, duration and performance words are supported, including Cyrillic case/ё normalization, sharps/flats, decimal minutes and checked seed/tempo/meter values. This is a bounded dictionary/grammar, not arbitrary language comprehension. Unknown fragments produce visible warnings. Positive instrument mentions form the requested track set; negative-only mentions remove roles from the default bass/chords/lead set. Piano maps to the chords role with GM program0.

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
