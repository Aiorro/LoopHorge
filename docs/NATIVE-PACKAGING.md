# Native portable candidate

This package contains the compiled JUCE VST3 bundle and Standalone application. Offline generation runs inside these binaries: the recipient does not need Node.js, npm, a local server, an AI account or model downloads. Optional existing AI integrations have their own configuration requirements. New generation is scoped to loops of at most 16 bars; full songs, section arrangements and extensions beyond 16 bars are outside this release.

Packaging is separate from release acceptance. A ZIP proves neither a successful clean installation nor DAW compatibility. `task.md` remains the acceptance record. Composer editing/history, scoped regeneration, request persistence, drag/drop and host transport must be verified against the applicable loop workflow before the release is declared finished. This document does not mark those features complete.

## Build and package

The builder needs Node.js 20+, Git, CMake 3.22+ and an already completed Release build. Node is a build-time packaging tool only. There are no npm packaging dependencies. The script does not build, sign, install, download dependencies or publish anything.

Run from the source checkout that was used to build the binaries. On Windows PowerShell, after the documented native build/test commands have passed:

```powershell
node scripts/package-native.cjs --build-dir build/native --juce-dir build/native/_deps/juce-src --output dist/native-candidate --platform windows
```

If CMake was configured with `LOOPFORGE_JUCE_PATH`, pass that exact source checkout instead of `_deps/juce-src`. The JUCE checkout must be clean and at the exact `GIT_TAG` in `plugin/CMakeLists.txt`: currently JUCE 8.0.8, commit `d6181bde38d858c283c3b7bf699ce6340c050b5d`. A source archive without Git provenance is rejected. The packager cannot prove that a binary was built from the selected source checkout: the builder must establish this through the build log and artifact provenance.

For the current Linux build environment, use a fresh output directory:

```sh
node scripts/package-native.cjs \
  --build-dir /workspace/loopforge-engine-build \
  --juce-dir /workspace/JUCE \
  --output /workspace/artifacts/loopforge-native-candidate \
  --platform linux \
  --cmake /workspace/native-tools/bin/cmake
```

Only Windows x64 and Linux x64 Release layouts are accepted. Required sources are:

| Platform | Standalone | VST3 module inside the complete bundle |
| --- | --- | --- |
| Windows | `LoopForge_artefacts/Release/Standalone/LoopForge AI MIDI.exe` | `LoopForge_artefacts/Release/VST3/LoopForge AI MIDI.vst3/Contents/x86_64-win/LoopForge AI MIDI.vst3` |
| Linux | `LoopForge_artefacts/Release/Standalone/LoopForge AI MIDI` | `LoopForge_artefacts/Release/VST3/LoopForge AI MIDI.vst3/Contents/x86_64-linux/LoopForge AI MIDI.so` |

The packager checks x64 PE executable/DLL sections or ELF load segments, rejects text placeholders, truncated images, missing modules and symlinks, then copies every bundle file recursively. These structural checks do not execute or host-load a binary. Bundle resources, including empty resource files, are preserved. A Windows PE fixture in the automated tests is synthetic and is never presented as a distributable build.

The output directory must not exist. A failed attempt leaves prior output alone and removes only its own staging directory. Choose a new output name for each attempt; never point the output at source, build, project or sample directories.

Output contains an extracted `LoopForge-…-candidate/` directory, its ZIP and a sibling `SHA256SUMS` for the ZIP. Inside the candidate are `Standalone/`, the complete `VST3/` bundle, these instructions, `DAW-ACCEPTANCE.md`, `FL-STUDIO-TEST.md`, `RELEASE-NOTES.md`, `licenses/`, `manifest.json` and another `SHA256SUMS`. The manifest records source HEAD, whether the packaging checkout is dirty, platform/x64, JUCE revision, unsigned/signature-present status and every payload file's size/SHA256. `-dirty` means HEAD is only the base revision; it does not uniquely identify the uncommitted source snapshot. Release provenance requires a committed source revision and a matching rebuild.

The internal checksum list also hashes `manifest.json`; it excludes itself to avoid a circular hash. The external checksum covers the complete ZIP. File ordering, modes and archive times are normalized to 1980-01-01 UTC. Packaging the same bytes, Git metadata and instructions with the same CMake version/platform produces the same ZIP; this is not a claim of reproducible C++ builds or byte-identical archives across CMake versions. Do not modify or sign files after generating checksums: rebuild the package/checksums after any change.

## Verify and extract on Windows

Obtain the ZIP and its SHA256 from a trusted release channel. In PowerShell, compare the complete hash with the published value (the filename below is illustrative):

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath '.\LoopForge-…-candidate.zip'
```

Extract the complete ZIP with Windows Explorer's **Extract All** into a new directory, for example `%LOCALAPPDATA%\Programs\LoopForge\candidate`. Keep the included notices and instructions with the application. Do not run from inside the ZIP. Keep projects, exported MIDI, recipes and samples in a separate user folder so replacement/removal of the application cannot remove them.

Candidates are untrusted unless independently verified. `signing.status: unsigned` means the Windows inputs contain no embedded PE certificate table. `unverified-signature-present` records a certificate table; it is not proof of a valid signature, publisher or trust chain. The packager never signs and always sets `signing.verified: false`. To inspect the extracted executable and the inner VST3 module:

```powershell
Get-AuthenticodeSignature -LiteralPath '.\Standalone\LoopForge AI MIDI.exe'
Get-AuthenticodeSignature -LiteralPath '.\VST3\LoopForge AI MIDI.vst3\Contents\x86_64-win\LoopForge AI MIDI.vst3'
```

An unsigned build can trigger SmartScreen or antivirus review. Do not disable security controls to complete installation; verify provenance with the builder if Windows blocks it. SHA256 detects changed bytes when compared with a trusted hash; it does not authenticate a publisher.

## Run Standalone and install the VST3

For Standalone, open `Standalone\LoopForge AI MIDI.exe` from the extracted folder and select your audio device in its audio settings. Generate an offline loop and use the built-in audition synth to hear it. This does not require installing the VST3 or a Node service.

For a per-user VST3 installation, close REAPER and copy the entire outer `LoopForge AI MIDI.vst3` directory from `VST3\` into `%LOCALAPPDATA%\Programs\Common\VST3\`. Create that destination if necessary. The result must be `…\VST3\LoopForge AI MIDI.vst3\Contents\…`. Do not copy only the inner `.vst3` binary, rename the bundle or merge it into another version's bundle. This per-user copy needs no administrator rights.

In REAPER x64, open **Options → Preferences → Plug-ins → VST**. Add `%LOCALAPPDATA%\Programs\Common\VST3` to the VST plug-in paths if REAPER does not already scan it, then rescan. Select the actual user path if environment variables are not expanded by that REAPER version. Alternatively, an administrator can put the complete bundle in `C:\Program Files\Common Files\VST3`; use only one copy to avoid duplicate scans. Load **VST3i: LoopForge AI MIDI** on a track. If a scan fails, record the host version and scan error before changing anything.

Windows DLL dependencies and a clean machine installation have not been audited for the current loop candidate. No Microsoft runtime installer is included. If Windows reports a missing runtime DLL, retain its exact name and ask the builder to verify the build's runtime linkage and the official matching Microsoft x64 redistributable requirement. Do not download replacement DLLs from third-party sites. The script does not establish a minimum supported Windows build.

## Route MIDI in REAPER

1. Put LoopForge on a source track and a known MIDI instrument on a second destination track.
2. Turn off LoopForge's **Audition synth** to avoid hearing its preview alongside the destination instrument.
3. Create a send from the source track to the destination. In its routing dialog select **Audio: None** and **MIDI: All → All**, preserving generated MIDI channels. If the instrument listens on one channel only, configure that instrument or intentionally map the send's MIDI channel.
4. Enable the VST3's MIDI output if your host exposes that switch. Start REAPER transport and check that the instrument receives notes. Arm/monitor only as needed for the chosen routing/recording mode; avoid a return send that creates a MIDI feedback loop.
5. For a recorded MIDI item, use the destination track's **Record: output → Record: output (MIDI)** mode, arm it and record transport. Verify note channels, timing and note-offs. Alternatively export a permanent `.mid` and import it into REAPER. Actual OS drag/drop needs its own acceptance check.

Standalone is intended for its local audition and MIDI export. Do not assume that its generated MIDI is available as a Windows MIDI device or that it routes to another application. Use MIDI export/import, or the VST3 in the DAW, until a native external-device output path has been explicitly verified. No virtual MIDI driver is installed by this package.

Record load, routing, play/stop/seek, 32/64-sample buffers, tempo changes, save/reopen and drag/import using the included `DAW-ACCEPTANCE.md`. Keep compilation, automated tests, pluginval, host loading and clean installation as separate evidence.

## Replace or remove

Save projects and close Standalone and all DAWs first. Keep your original project/MIDI/recipe/sample files. For an update, extract to a new application folder and replace the entire installed VST3 bundle after backing up the previous bundle; rescan in REAPER. Avoid merging version folders. Retain the old application until existing projects have been reopened successfully with the new build.

For removal, delete only the installed `LoopForge AI MIDI.vst3` bundle and the extracted application/candidate directory you chose, then rescan REAPER. There is no installer, service, registry registration, automatic updater or package-managed uninstall step. Host projects may retain a missing-plugin reference after removal. The package does not locate or delete user data, exported MIDI, recipes, samples, host settings or caches.

## Linux candidate

Extract the ZIP and run `Standalone/LoopForge AI MIDI`; preserve its executable permission (`chmod +x` only if the extraction tool dropped it). For an optional per-user plugin install, copy the entire bundle into `~/.vst3/`, then rescan the host. Linux is a local verification candidate; required system libraries, including glibc/C++ runtime, audio and font/desktop libraries, are not bundled. Check actual binaries with `ldd` on the target system. An x64 Linux ZIP is not a Windows package and is not guaranteed portable between distributions.

## Dependency notices and distribution status

`licenses/LoopForge-LICENSE.txt` covers repository code under its MIT notice. It does not relicense JUCE or the VST3 SDK. The packager copies notices verbatim from the clean pinned JUCE checkout, including JUCE's top-level license, VST3 root/base/pluginterfaces/public.sdk terms, zlib and its JUCE changes, libpng, Independent JPEG Group, HarfBuzz, SheenBidi, FLAC and Ogg/Vorbis. The JPEG acknowledgment is: this software is based in part on the work of the Independent JPEG Group. The source paths in the package preserve provenance and avoid substituting terms from a newer SDK revision.

The selected open native distribution uses AGPLv3 with preserved MIT notices and the SDK GPLv3 option. Full license texts and committed LoopForge/JUCE sources accompany candidates. Tracked staged/unstaged changes are rejected; untracked user files are excluded from source archives. Actual binary/source correspondence and system/compiler-runtime dependencies remain builder verification duties. validation.redistributionLicenseCleared stays false pending target verification; this is not legal certification.

## Windows artifact evidence (read-only check, 2026-11-17)

Historical remote native run [44023446517](https://github.com/Aiorro/LoopHorge/actions/runs/44023446517) at source `667369190fa600f77a96b3ff5735efc96c144b82` concluded **failure**. The GitHub connector reports Windows job `133838046646` cancelled, Ubuntu job `133838046975` failed and no artifacts. Job step metadata was empty; the log endpoint returned `404 BlobNotFound`. The exact runner failure cause cannot be established from these results, and no permissions or credentials were changed.

The older [Windows artifact 13979442439](https://github.com/Aiorro/LoopHorge/actions/runs/43294644471/artifacts/13979442439) is reported unexpired, with SHA256 `65644ead60e8214a4532e72cba111e633246cc4fcbae497efa216cb1d9c74e54`. Its run source is `2a004b703969c8c32a7c58943810ecca71c7a137` and its artifact name references merge build `df03dd1dee9d25f7130441ff961e665bc826931d`; it predates the current Composer/16-bar work. It is historical evidence, not the new Windows candidate. A matching current Windows build, portable package, clean-install test and REAPER host acceptance remain required.

For FL Studio x64, follow [the dedicated host guide](FL-STUDIO-TEST.md). The Native plugin CI matrix uploads a separate candidate ZIP/SHA256 artifact after all tests and protocol checks succeed. Each OS runs independently; a failed job cannot provide a successful package. The Windows ZIP contains `FL-STUDIO-TEST.md` and `RELEASE-NOTES.md` alongside the binaries.

Open native distribution uses AGPLv3 with preserved MIT notices and the SDK GPLv3 option. Candidate ZIPs include full license texts and committed `Sources/LoopForge.tar` plus `Sources/JUCE.tar`. Submodules cause packaging rejection until bundled explicitly; source correspondence remains the builder responsibility. CI publishes only the complete candidate artifact, not a second bare binary artifact.
