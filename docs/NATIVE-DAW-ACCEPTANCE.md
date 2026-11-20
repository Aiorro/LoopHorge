# Native DAW acceptance record

Status: not executed. This is a reproducible procedure/report template, not a second task checklist; task.md owns T12 acceptance. No REAPER, Wine or pluginval executable was found in the current Linux environment. Compilation, injected native dispatch tests and CTest do not establish host acceptance.

## Record the environment

Record Windows edition/build, REAPER exact version/architecture, audio driver/device, sample rate and buffer size, plugin source SHA, artifact name/SHA256, VST3 search path and whether the artifact is unsigned. Retain the complete .vst3 bundle. Do not label an installer/release verified from a CI artifact alone.

## Load, route and transport

Scan and load the actual VST3 in REAPER; record scan errors and editor open/close/reopen behaviour. Generate offline with Node stopped, inspect all8ideas and edit bass/chords/melody. Turn off Audition synth and route MIDI output to a known instrument on another track; record channel configuration and whether duplicate synth playback was avoided.

Run at32 and64samples, then the normal device setting. Record play/stop, seek forward/back, negative/pre-roll PPQ if supported, loop boundary and live tempo changes90→180→90. Listen/check for held or missing notes, repeated boundary notes, channel confusion and audible glitches. Exercise overlapping identical pitches and retriggered onsets; compare with the expected final-release behaviour from plugin/tests/NoteLifetimeTest.cpp. Save/reopen the project and verify committed edited notes persist and stale generation does not overwrite restored state.

## Drag and file import

Select an idea, begin DRAG MIDI and drop on the arrangement. Record actual OS gesture acceptance separately from REAPER's import result. Cancel once; then repeat successful import. Test normal, short/final-boundary notes and trailing rests in4/8-bar patterns. Verify PPQ480, tempo/meter, bass/chords/melody track separation and full pattern length. Record REAPER's import-dialog choices, since a host may merge tracks by choice.

While a gesture is captured, replace/select/edit the idea; the exported file should still contain the captured committed snapshot. Test closing the editor around drag completion and delayed import; verify the file remains readable. Save/export a permanent .mid and compare musical content with the drag file. For a host that permanently references a source path, use the permanent export; cache retention is not project asset storage.

## Current Windows acceptance checklist

Repeat the following in Windows REAPER and FL Studio. Record the exact host, driver, sample rate, audio buffer settings and artifact SHA256.

1. On a clean Windows account, extract the complete ZIP. Start Standalone with networking disabled and without Node or a model. Compose a 4, 8 and 16 bar loop using Rule Composer. Verify audition, region variation with Undo, project reopen, MIDI export and removal of the extracted installation.
2. Scan and load the VST3. Start and stop playback, seek across held notes, loop, change tempo, bypass during held notes and re-enable the plugin. Test at 32 and 64 samples where the driver permits. In FL Studio, also exercise variable block lengths and record the actual host behaviour.
3. During internal pattern playback, route incoming MIDI with the same channel and pitch into LoopForge. Confirm that the generated pattern owns the output. Count Note On and Note Off with an external MIDI monitor and check audio silence on bypass. Record if the host stops invoking the plugin or discards its MIDI while bypassed.
4. In the idea bank, repeat external MIDI drags while earlier imports are still pending. Confirm unique temporary paths and readable files after delayed import or editor close. Compare the imported notes, track channels, tempo, meter and endpoint with a permanent Type-1 export.
5. Run the 16-bar workflow with generation, preview, edits, Undo history and export. Record peak memory and responsiveness using the agreed workspace measurement, alongside a clean-launch baseline. Keep R05 open until the aggregate 256 MiB budget and the slow-storage case have evidence.

## Results to retain

For each scenario record Pass/Fail/Not run, exact steps, observed behaviour, expected behaviour, host logs/screenshots and any reproduced failure. Acceptance presently remains Not run for Windows REAPER load/routing/transport, actual OS drag/import and plugin unload. Update task.md with observed evidence and exact host versions; T12 cannot be checked from this template.

## Available source-verified artifact

Source2a004b7 passed native Windows11/11 and Linux12/12 tests in https://github.com/Aiorro/LoopHorge/actions/runs/43294644471 . The unsigned Windows bundle is https://github.com/Aiorro/LoopHorge/actions/runs/43294644471/artifacts/13979442439, GitHub-reported SHA25665644ead60e8214a4532e72cba111e633246cc4fcbae497efa216cb1d9c74e54. This is the candidate for the procedure above, not evidence that it was installed or host-loaded.
