# Prepared Publication and JUCE MIDI Adapter Implementation Plan

> Execute inline with superpowers:executing-plans/TDD and one fresh review. Approved architecture and continuation authorize this dependency. Task.md remains the sole progress checklist.

**Goal:** Transfer prepared songs safely from one worker to one render thread and deliver bounded packets to actual prepared JUCE MIDI buffers.

**Architecture:** A three-slot SPSC mailbox publishes indexes with acquire/release atomics; only the worker moves/destroys payloads. A JUCE ComposerPlayer consumes once per nonzero block, renders into a preallocated MidiBuffer and flushes on clear. This is the tested callback component; current Processor/UI activation waits for v2 project-envelope/restore integration.

**Tech Stack:** C++20, existing PreparedSong/Playback and JUCE8.0.8; no new dependencies/processes/network.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, prepared playback/realtime ownership, shared16channel cleanup and pinned32768-byte MIDI reservation.

## Global Constraints

Exactly three slots, free/writing/pending/active states; one serialized worker and one non-reentrant consumer. Consumer exchanges only indexes and immutable pointers, never moves/destroys/refcounts dynamic data. Latest pending publication wins; replacing pending retires its slot on worker. Active view remains valid until next consumer acquisition. Failed publish preserves incoming ownership and active/pending state; busy is retryable outside callback. Clear is a pending tombstone; consumer releases old active slot without destroying it. Reclaim/reset/destruction only on worker/control thread, with reset/destruction after producer and callback quiescence. Retained prepared storage capped at64MiB (configurable downward only); this is not total256MiB pipeline certification. JUCE caller must reserve32768bytes outside callback; callback clears/reuses buffer and emits<=2048short MIDI packets (<=18432encoded bytes). Preserve existing Processor/state/bridge/UI.

## Review Focus

- Producer/consumer exchanges racing pending replacement or clear must never free active data, double-own a slot, or publish stale/mixed metadata (Task1).
- Retired vectors must be destroyed only by producer; consumer cold/replace/clear/query paths must show no new/delete (Task1).
- Insufficient retained budget and transient busy must not move the rejected candidate or change active/pending state; stopped reset needs quiescence (Task1).
- JUCE32/64sample cold/dense/seek/replacement/clear/tempo/overflow callbacks with32768byte buffers must allocate/free nothing and preserve MIDI ordering, channel/preset/CC and converter counts (Task2).
- Zero-size blocks, empty/cleared source, stop/end/overflow and repeated pending updates must flush once and stay within2048messages, including all-channel releases (Task2).

### Task 1: Three-slot immutable publication

**Files:** create plugin/Source/engine/Publication.h, plugin/tests/PublicationTest.cpp; modify CMakeLists.txt/task.md.

**Interfaces:** PreparedMailbox(size_t retainedBudget=64MiB); PublishStatus{published,busy,budgetExceeded}; publish(PreparedSong&&,uint64_t tag)->PublishStatus (worker); clear() (worker tombstone); reclaim() (worker free-slot destruction); retainedBytes() (worker-only); beginBlock()->PublishedView{const PreparedSong* song,uint64_t tag,bool changed} (consumer); resetStopped() (control thread after both sides quiescent). Mailbox noncopyable/nonmovable. Tags are caller metadata, not a revision/cancellation policy.

1. Write failing tests for initial empty/latest-before-consume/publish-while-active/clear/clear-replacement, stable active queries, budget rejection preserving source, reclaim and quiescent reset. Track consumer new/delete and actual retained bytes. Expected: missing Publication.h.
2. Implement release-published pending index/acquire exchange, worker CAS on free slots, consumer-only active index and free return. Worker reclaims old payloads; never dereference a view past next acquisition. Bound candidate+retained storage before moving. Expected: directed tests pass and no consumer allocations/frees.
3. Run20000producer/consumer iterations with coherent tagged source fields and interspersed clears, preserving last publication. Strict compiler/ASan/UBSan and attempt TSan (report actual environment limitation if unsupported); full native/Node/sample checks. Expected: no wrong metadata/lifetime/hooks; no fabricated race-detector pass. Record evidence and commit.

### Task 2: Prepared JUCE callback component

**Files:** create plugin/Source/ComposerPlayer.h, plugin/tests/ComposerPlayerTest.cpp; modify Playback.h/CMakeLists.txt/README/task.md.

**Interfaces:** Playback::stop(Sink)->PlaybackResult flushes/reset fixed runtime state without dereferencing a source. ComposerPlayer owns mailbox+Playback. Worker publish/clear/reclaim delegate; resetStopped after quiescence. process(const PlaybackBlock&,juce::MidiBuffer&)->ComposerPlayerResult{PlaybackResult playback,uint64_t tag,bool hasSong}. Consume only on nonzero blocks; clear input MIDI as legacy does; raw two/three-byte sink preserves sample/insertion order and no MidiMessage heap construction. Caller buffer must be prepared, not checked/repaired in callback.

1. Write failing callback tests for pending source replacement (old offs before new ons), clear/empty/zero blocks, program/bank/controller/order/channel16, maximum256note/256CC chase and304release,32/64samples/tempo, overflow pause, repeated candidates and worker reclamation. Verify existing VST3 converter delivers all packets. Expected: missing ComposerPlayer.h/stop API.
2. Implement bounded bridge using only consumer index/pointer and noexcept sink; no preparation/ensureSize/resetStopped/reclaim in process. Stop flushes without retaining/dereferencing retired data. Expected: actual MidiBuffer output matches traces/counts and<=18432bytes.
3. Linux malloc/calloc/realloc/free wrappers plus C++new/delete hooks track callback allocations and deallocations with fresh preallocated host buffers and producer activity; cross-platform ordinary hooks remain. Run native VST3/Standalone/CTest, Node/sample, ASan/UBSan and fresh review; fix Critical/Important RED→GREEN in one pass, then push PR3. Explicitly retain source/callback quiescence, prepared-buffer and future Processor/state/UI/DAW/full-workspace limits.
