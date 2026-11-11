# RFC 0001: JUCE native host and Node composition bridge

Status: implemented Linux prototype, including native offline generation; Windows and DAW validation pending. JUCE 8.0.8 is pinned at d6181bde38d858c283c3b7bf699ce6340c050b5d.

## Decision

Retain the tested Node generator and provider adapters. Add an isolated C++20/JUCE project under plugin/ with CMake targets for VST3 and Standalone, initially Windows x64. Pin JUCE to an exact revision and provide an explicit local JUCE source override for offline builds. Preserve the existing Node/browser workflow.

Use a small native processor/editor and an asynchronous bridge for optional AI. Following the 2026-11-11 installation review, the deterministic offline rules also run natively so generation does not require service installation or lifecycle management. Eight seeded ideas were compared with normalized Node output; the style/mode matrix is exercised in native tests. The browser/Node engine remains available.

## Boundaries

A background worker calls the explicitly configured loopback composition service, with finite deadlines, response limits and cancellation. The plugin must not silently start processes or install dependencies. Credentials remain outside serialized plugin state and outside the audio thread. The worker validates the composition contract and prepares a fixed-capacity, sorted schedule (maximum 1600 notes, 3200 on/off events). Malformed data is rejected without replacing the current composition.

Transfer schedules with a bounded single-producer/single-consumer queue of preallocated slots. Audio owns the active slot; the producer must never mutate or reclaim it until ownership is returned. If the queue fills, reject/defer the update on the worker. Do not use mutexes, shared_ptr destruction, allocation, JSON parsing, file I/O, networking or subprocesses in processBlock.

## Playback

Use host PPQ position and BPM when available. Emit MIDI with sample offsets within the current block and separate channels for bass/chords/melody. Define loop boundaries as half-open intervals. Note-offs precede note-ons at equal times. Maintain a fixed active-note table; flush active notes on stop, seek, loop discontinuity, replacement and release. Test variable block sizes, tempo changes, missing playhead data and overlapping equal-pitch notes before enabling host playback. Standalone needs an explicit internal transport and audition instrument; it cannot assume a DAW playhead.

## State and validation

Store a versioned composition, selected idea and non-secret controls. Reject unsupported versions and corrupt or oversized state while retaining the last valid state. Serialization must take a safe snapshot outside real-time processing. Never store API keys, CLI tokens or provider response bodies.

Acceptance: CMake configure/build on Windows; actual VST3 and Standalone outputs; deterministic scheduler tests including seeks and note-offs; bounded worker handoff tests; state round-trip/corruption tests; then manual REAPER MIDI/tempo/transport/state verification. CI uploads unsigned artifacts with commit IDs. Compilation alone is not host compatibility. Native editor expansion follows these checks.

## Implemented callback boundaries (2026-11-11)

Count overlapping same-channel/pitch lifetimes, retrigger with a balanced off/on pair, and release only after the final logical end. Preserve the preceding sample interval at tempo boundaries to avoid skipped or repeated events. Target wrappers prepare 32 KiB of MIDI storage outside processing via generated copies of the pinned JUCE preparation code. Custom callers must meet the same storage contract. Cap output to the VST3 converter's 2048-event limit, reserving final releases for all 384 possible native-layer voices. Oversized dense blocks pause generation after releasing active notes and report the event; ordinary blocks resume at host time. Validate allocation behavior inside Processor and actual MidiEventList event delivery separately from full DAW/driver realtime behavior.
