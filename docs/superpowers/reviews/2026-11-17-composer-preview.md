# Composer bounded preview — 2026-11-17

Result2b afterc08208c. Composer now consumes its generated short MIDI read-only to render a basic native stereo preview. Melodic voices use sine oscillators; channel10uses deterministic decaying synthetic percussion. Fixed storage holds64voices; free slots are used before round-robin stealing when full. This is not GM program/bank/pan emulation. Full MIDI remains complete and byte-identical with SOUND on/off.

Events apply before their corresponding audio sample. Sustain/sostenuto/hold2, note releases and channel cleanup are bounded. Mute continues advancing synthesis; source/mode/lifecycle/stop paths release ownership. The actual SOUND control is visible/enabled in Composer, reflects its runtime state after reopening and does not add recipe fields. Existing Legacy synthesis remains in its previous path; the shared SOUND toggle now reflects its actual state in both modes. Standalone still plays one pass with the composition tempo map; seamless looping remains R02.

## RED→GREEN and independent review

Actual32/64sample Composer audio was silent before implementation (`/tmp/composer-audition-red.log`). New audio/MIDI-equality tests passed after the bridge. Four defects were caught before preview publication:

- ERR084: muted percussion envelopes froze and restarted on unmute. A600block muted drum test failed; synthesis now advances while only the output is gated.
- ERR085: short notes stole sustained C4 despite63free slots. Independent probe and permanent100short-hats regression failed; bounded free-slot search now precedes full-pool stealing.
- ERR086: same-pitch retrigger discarded sostenuto ownership. NoteOn→CC66down→NoteOff→NoteOn→NoteOff failed; existing mapped latch is retained until pedal release.
- ERR087: reopening a muted editor displayed SOUND on and required two clicks to hear playback. Actual-editor reopen assertion failed; initialization/timer now synchronize from the runtime flag.

Fresh read-only gpt-6.1-sol/high review returned0Critical/2Important/1Minor. Root reproduced both Important findings before fixing. SOUND mismatch was regraded Important: a false enabled control makes working preview appear broken; this is a functional state mismatch, not label polish. Its actual-editor regression failed before correction. One TDD fix pass covers all review findings; no second review certifies the fixes. The two isolated ownership probes are `/tmp/preview-{free-slot,repeat}-red.log`; permanent regressions live in ComposerAuditionTest. Three old full-suite assertions explicitly expecting Composer silence were updated to the intended nonzero preview behavior; MIDI assertions remain.

## Verification

- Linux VST3/Standalone/all targets built: `/tmp/composer-audition-final-build.log`.
- CTest43/43,0failed: `/tmp/composer-audition-ctest-final.log`.
- Node93/93,0failed,0skipped: `/tmp/composer-audition-node.log`.
- Actual test main and actual Plugin.cpp ASan/UBSan: `/tmp/composer-audition-san.log`; vendor modules are not fully instrumented.
- Preview stress ASan/UBSan:700000fixed-seed malformed/valid MIDI steps at rates1/8000/44100/48000/768000/NaN/0remain finite/bounded and reset silent: `/tmp/composer-preview-stress-san.log`. This pure-synth numeric check is not audio-device/rate acceptance.
- Callback allocation traps pass cold/dense/tempo/stop Composer32/64/2048buffers with fresh32768byte MIDI storage; prior Legacy32..192000coverage remains: `/tmp/composer-preview-realtime.log`. No callback locks or heap-owning preview structures were introduced. These checks do not prove worst-case device deadline/whole-process memory acceptance.
- Native API/failure/cancel/recovery and stale-worker fixtures: `/tmp/composer-audition-{api,failures,worker}.log`.
- Composer audition test checks nonzero early/sustained sound, exact release silence, stop/restart/mute, dense channels1/10/16, finite output, unchanged MIDI bytes, three pedals/retrigger, free-slot ownership and actual-editor SOUND/reopen.

## Rulings and limits

Use the existing native output with64fixed preview voices and no assets/services. Reason: bounded CPU/storage within R02 instead of introducing a sampler. Cost: dense previews may omit voices and timbre differs from GM/external instruments; complete MIDI is unchanged.

Regrade SOUND mismatch as Important and fix the shared toggle. Reason: an enabled-looking control with silent runtime is broken user feedback. Cost: Legacy also shows actual runtime mute state; no persisted project/recipe format changes.

Reviewer set aside actual listening/native window/device/Windows/DAW/pluginval, worst-case CPU/aggregate resources, runtime sample-rate extremes, full GM/pan and seamless looping. Root added the numeric rate-stress check above; the other applicable acceptance remains in task.md. Reason: deterministic component tests cannot certify listening or hosts. Cost: the packaged end-to-end release scenario remains incomplete. No new models/providers/platforms were added.

Prior deferred minors remain: Mood/Auto return label can describe overview while returning to the piano roll; unconfirmed Compose/host-restore fallback-seed capture race. This review introduced no new deferred minor after the regrade/fix. errors.md has87unique IDs:80Fixed/4Open/1Investigating/2Blocked. R02still needs scoped musical regeneration/variants, remaining controls and seamless standalone looping. R04/R05/R06 and original T12–T15 remain open; no public release/main merge/Windows acceptance is claimed.
