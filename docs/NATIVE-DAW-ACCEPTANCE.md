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

## Results to retain

For each scenario record Pass/Fail/Not run, exact steps, observed behaviour, expected behaviour, host logs/screenshots and any reproduced failure. Acceptance presently remains Not run for Windows REAPER load/routing/transport, actual OS drag/import and plugin unload. Update task.md with observed evidence and exact host versions; T12 cannot be checked from this template.
