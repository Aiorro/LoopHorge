# FL Studio x64 — alpha test

Target: Windows x64, FL Studio x64, LoopForge VST3. FL Studio acceptance has NOT been performed. Record exact FL Studio/Windows versions, audio driver, sample rate, buffer size, source commit and package SHA256 with every report. Start with a disposable FL project and keep existing projects backed up.

## Standalone Windows EXE

The same Windows ZIP includes `Standalone/LoopForge AI MIDI.exe`. Extract the complete package and launch that EXE directly. It is a native JUCE application, not Electron/a web launcher, and offline Composer does not need Node. Open its audio settings, select an available Windows audio device/driver, rate and buffer, generate a loop, enable **Sound** and press **Play / Stop**. Record the exact device/driver in the test report. Standalone currently plays one pass; MIDI export/import is the supported transfer into FL Studio. No virtual MIDI cable/output-device integration is claimed. Keep both the EXE and the package notices/sources; no background service is installed.

The Native plugin workflow builds the Windows x64 EXE and VST3 and runs automated tests. Standalone playback with a real Windows audio device still needs a manual test.

## Install and scan

1. Extract the entire Windows candidate ZIP; verify its SHA256 against SHA256SUMS. Linux packages cannot load in Windows FL Studio.
2. Close FL Studio. Copy the complete outer `VST3/LoopForge AI MIDI.vst3` bundle to `C:\Program Files\Common Files\VST3` (administrator copy). Keep only one installed version. Do not copy just the inner module.
3. Open **Options → Manage plugins**; check the search paths, enable plugin verification and run **Find installed plugins**. Load LoopForge AI MIDI as a generator from the Channel Rack. Record scan errors instead of bypassing verification.
4. No Node server/account/model is required for offline Composer. Optional cloud/CLI bridges are outside this first test.

## First sound and MIDI

For the offline acceptance run, choose **Rule Composer** in the engine selector. In the piano roll of a supported native loop, choose a track and use **Regenerate Bars**. Compare the edited span with the prior version and verify Undo restores the original notes. Record a separate result for bypass and for sending MIDI into the generator while its internal pattern plays.

- Enter `4 bars D minor 100 bpm bass chords lead drums`, seed42, press COMPOSE LOOP. Enable SOUND. Start FL's transport: plugin playback follows host transport/tempo; the plugin PLAY button is for standalone playback when no host clock is supplied.
- Basic SOUND is a sine/synthetic-percussion preview, not a GM instrument. For external instrument routing, mute SOUND. In the LoopForge wrapper settings, assign a MIDI **Output port** (for example10); give the receiving VST instrument the same wrapper MIDI **Input port**. Ports are FL routing numbers; track MIDI channels remain separate. Instrument/channel filtering depends on the receiving plugin. Verify this in your installed FL version before assuming live routing works.
- The first test path, independent of live wrapper routing, is **EXPORT MIDI**. Import the `.mid` into a fresh FL project using the MIDI import dialog and retain separate channels/tracks. Assign instruments to the imported channels. Check bass/chords/lead/drums and note timing. Do not import over valuable piano-roll data.
- Composer OS MIDI drag is not implemented. Use export/import for this alpha. Existing Legacy drag behavior still needs host verification.

## Test record

Use PASS/FAIL/NOT TESTED plus steps and observed result for each case; none are pre-approved here.

| Case | Procedure | Expected |
| --- | --- | --- |
| Scan/load | Verified scan; add/remove plugin ten times | No scan failure, crash or hang |
| Offline | No local server; generate4/8/16bars; try17bars | Valid lengths work;17rejects without losing notes |
| Audio | SOUND on/off, repeat notes, stop/start | Audible preview when on; no stuck note after stop; MIDI unaffected |
| Clock | FL transport,100→140BPM, seek, host loop crossing | Notes follow host clock; no stuck notes; record boundary glitches |
| Buffers | Repeat at32/64/256samples,44.1/48kHz with driver support | No crash, stuck notes or repeatable underrun; record CPU/driver |
| Edit | Select each track; add/move/resize/delete, quantize, Undo/Redo | Other tracks preserved; selected note remains correct |
| Save | Save FLP with edited notes and next-request mood/seed; close FL; reopen | Notes and valid request controls restored |
| Instances | Two independent instances with different seeds; remove during generation | Independent state; no crash/hang |
| MIDI | Export/import multi-track MIDI; compare note count/timing and drums | Tracks/channels preserved; no shortened endpoint |
| Recovery | Cancel generation; invalid direction; reopen UI muted | Prior music retained; SOUND matches actual state |

Standalone Composer currently plays one pass. Seamless standalone looping, scoped musical regeneration and remaining explicit controls are incomplete. Host loop compatibility is a test target, not certified.64voice preview can steal when full; complete exported MIDI remains intact.

## Report and remove

Report: build SHA / ZIP SHA256 / FL version / Windows version / driver / rate / buffer / case / exact reproduction / expected / actual / frequency. Attach a minimal non-sensitive FLP, project JSON/MIDI and screenshot or scan log when useful. Never include provider keys or personal projects. Report crashes with available host log/Windows event details.

To remove, close FL Studio and delete only the installed LoopForge VST3 bundle and extracted application folder. Rescan plugins. Keep user FLP/JSON/MIDI/recipes/samples. No background service is installed.

## Reproduce the Windows candidate when CI is unavailable

Open **Visual Studio 2022 → Developer PowerShell for VS2022** (x64), with the Desktop development with C++ workload, Git, Node20+ and CMake3.22+ already installed. The script does not install them or change execution policy. Run from a clean committed main checkout:

```powershell
.\scripts\build-windows-test.ps1 -JucePath 'C:\src\JUCE'
```

JUCE must be the clean pinned8.0.8 revision. Alternatively, explicitly permit CMake to download the pinned JUCE source:

```powershell
.\scripts\build-windows-test.ps1 -FetchPinnedJuce
```

This runs Node tests, MSVC x64 build, CTest and native protocol checks, then creates a fresh Windows ZIP containing both `Standalone/LoopForge AI MIDI.exe` and the complete VST3 bundle, source archives/licenses, guides and SHA256SUMS. Existing output is never deleted; choose `-OutputDirectory` for another attempt. If Windows blocks script execution, follow your organization's normal script policy; do not disable security controls. This script still requires an actual Windows execution result before claiming success.

Linux MinGW is not a supported fallback for the pinned JUCE8.0.8: juce_TargetPlatform.h explicitly errors with "MinGW is not supported. Please use an alternative compiler." A2026-11-17cross attempt configured but failed at that guard; no EXE was produced or relabeled.
