# Composer Overview and Project Files Implementation Plan

> Execute inline with superpowers:executing-plans/TDD and one fresh final review. Existing architecture approval/repeated continuation authorize this E04 UI dependency; task.md owns all progress checkboxes.

**Goal:** Let native users open/save real v2 projects and inspect their full tracks/maps/sections instead of an empty legacy piano roll.

**Architecture:** A revision-based non-RT presentation snapshot copies committed Composer JSON only when changed. ComposerOverview consumes one validated document into a fixed1024-bin-per-track pitch/density overview and exact read-only metadata; no full document remains in the painter. Processor exposes a boolean restore result while retaining the host void override. ProjectFiles bounds reads before allocation and writes captured committed bytes using existing JUCE replacement semantics. Native chooser handlers run on the message thread, never the audio callback. Existing legacy editing and export stay on their tested path.

**Tech Stack:** C++20/JUCE8.0.8 and existing ProjectJson; no new dependencies. Modern Web Guidance was installed earlier but its HTML/CSS guidance does not govern native JUCE drawing.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, native mode/track view/state compatibility/limits and honest capability display.

## Global Constraints

Project files are full version1/version2 envelopes, not a silent composition import. Opening must report Processor commit success; invalid/truncated/over32MiB file preserves the project. Save captures committed state at click and never writes malformed/default-null state. Fixed16tracks/1024overview bins; aggregate note intervals with pitch/bin difference arrays, O(notes+16*128*1024) construction and bounded paint independent of note count. Exact tempo/meter/key/section metadata remains accessible in a read-only scrollable details component. Quarter-beat timeline must not invent a fixed4/4 bar grid. Composer UI remains MIDI-only/read-only; generation/edit/audition/MIDI export/drag and recipe/history integration remain open. No audio-thread JSON/IO/locks/allocations. No claim of filesystem crash recovery, nonblocking slow-disk UI, Windows/DAW/pluginval or measured256MiB peak. Preserve original15tasks, task-engine payload and untracked samples.

## Review Focus

- A refresh must not repeatedly copy/decode32MiB JSON on the5Hz timer, and a mode return must restore the real legacy roll (Task1).
- All16tracks, channel10/16, Unicode names, large IDs, tempo/meter/key changes and late sections must remain inspectable; long notes/endpoint rounding cannot spill into a following bin (Task1).
- All-overlapping maximum notes must build through bounded difference arrays, not notes*bins painting/scanning (Task1).
- Failed file read/restore must preserve current state; file growth/truncation/oversize and unwritable save must fail without deleting unrelated content (Task1).
- Editor closure/dialog completion and host replacement during an open save chooser must use safe captures; Composer must not expose fake legacy editing/export (Task1).

### Task 1: Read-only Composer view and bounded project file actions

**Files:** create plugin/Source/{ComposerOverview.h,ProjectFiles.h}, plugin/tests/ComposerOverviewTest.cpp; modify Processor.h/Plugin.cpp/CMakeLists.txt/README/task.md.

**Interfaces:** Processor::Presentation{uint64_t revision;bool composer,hasState;string json}; presentation(uint64_t known)->optional<Presentation>, returns none if unchanged, JSON only for Composer. Processor::restoreStateInformation(const void*,int)->bool; host override delegates; restoreV2 returns bool. ComposerOverview::setDocument(const Composition&,double origin)->bool, consumes validated source into bounded model; requiredHeight()->int used by viewport. projectFiles::read(File)->ReadResult{string text,error}; write(File,string_view)->String error, validates full v1/v2 and caps32MiB before filesystem mutation. UI open/save project chooser buttons, safe pointers and captured state; active viewport selects actual overview/legacy roll.

1. Write failing tests against real Processor/file helpers and editor: exact open/save v1/v2, wrong/oversized/truncated/unwritable paths preserve current state/files; presentation revision/mode/cache behavior; both viewport modes;16tracks Unicode/maps/sections exact detail text; Composer controls guarded; seed/origin visible; maximum131072notes/65536CC overview construction and rendered screenshot. Expected: missing new headers/API.
2. Implement boolean restore, revision snapshot, bounded read/write helpers and compressed overview; generate metadata outside paint, reusing normalized role/scale labels. Add chooser handlers and viewport sizing/focus/scroll restoration. Fix tracked overflow diagnostic with explicit Composer stop/start-or-seek recovery guidance; exercise actual dense overflow to verify persistent state/message then stop recovery. Expected: focused test green, legacy tests unchanged.
3. Run actual Linux VST3/Standalone build/CTest, Node/sample equality, native API/failure and Composer held-response fixtures, sanitizer probe where meaningful. Inspect generated native screenshot at minimum and expanded window size. Record limitations and commit; task-done runs full CTest. One fresh review; fix Critical/Important RED→GREEN once, ledger minors/rulings, then update existing draft PR. No merge/release.
