# T12 MIDI drag/drop design

Status: proposed; user approval pending. task.md is the sole task/progress checklist. This small design covers the drag source; real DAW acceptance is a separate part of T12 and stays unchecked until actually observed.

## Intent and approach

Drag the committed current idea into an application that accepts MIDI files, preserving the existing type1 encoder and Save dialog. Recommend a separate DRAG MIDI control next to EXPORT MIDI: it makes the gesture discoverable and keeps normal export behaviour predictable. A combined click-to-save/drag-to-copy control saves space but hides the second action and complicates accidental-drag handling. The separate control is the implementer's recommendation, not yet a user-approved choice. Use JUCE8.0.8's native performExternalDragDropOfFiles, not platform-specific virtual-file/COM implementations or an external service.

## Interaction and snapshot

Split the existing230px export area into two controls without changing editor size. DRAG MIDI is enabled only for a valid current composition. On mouse-down capture a deep copy of the committed selected idea; after at least6logical pixels of left-button movement, encode that captured idea once and start one native OS drag for that gesture. Click without movement performs no export or file write. A later edit, selection or generation does not silently change a drag already captured. No preview-only piano-roll state is exported. Files are copy-only (canMoveFiles=false). An invalid snapshot, file-write failure, full cache or failure to start native drag produces a clear status and leaves normal EXPORT MIDI available. 'Drag started' does not claim target acceptance; the OS completion signal does not establish a successful DAW import.

## File lifetime and resource bounds

Use a dedicated user application-data cache under LoopForge/DragMIDI, with random UUID.mid names and user-private directory permissions where supported. Cache contains only encoded MIDI, never prompts or credentials. Keep files for at least24hours rather than deleting when the native gesture ends: a target may read asynchronously. Creating the cache and writing the bounded file occur outside processBlock. No network, subprocess, installation or background service is added.

At preparation time prune only recognized regular non-symlink cache files older than24hours; never recurse through arbitrary paths or touch user-selected export destinations. Cap cache at256files/16MiB; if fresh retained files reach the bound, reject new drag preparation and suggest ordinary export. Never remove a file belonging to an active drag. A host that references source files permanently must use a normal saved export instead; the drag contract is a copy/import operation. Native completion handling must not capture a raw Editor pointer; a file must remain available even if the editor closes. Prefer no completion callback when it is not needed. Native JUCE/process-unload behaviour remains part of real-host acceptance.

## Verification and limits

TDD must cover decoding the actual cached MIDI (type1, PPQ480, tempo/meter, three tracks plus tempo track and pattern endpoints), immutable capture across edits, short/final-boundary notes, click/no-drag, movement threshold, one dispatch per gesture, copy-only dispatch, invalid composition, write/start failure, cache retention/cap/age cleanup and source/editor destruction. A narrow native-drag adapter may be substituted in headless tests to inspect the files and flags without starting a blocking OS drag; this proves the source contract, not OS/DAW interoperability. Run full Node/native suites and both platform CI builds; inspect the actual UI.

Record actual Windows REAPER version, VST3 load, host play/stop/seek/loop/tempo, routed MIDI and drag import/track separation before closing T12. No REAPER, Wine or pluginval executable was found in this environment. Therefore this design authorizes implementing/testing the source, not claiming that Windows DAW acceptance passed. No DAW download/install or release publication is included.
