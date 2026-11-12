# Native editing implementation plan

> Use superpowers:executing-plans inline; progress lives only in task.md (AGENTS.md overrides duplicate checklists).

Goal: implement approved T10 native note editing.
Architecture: dedicated JUCE roll, validated deep-copy edits on the non-RT processor boundary, existing bounded schedule handoff.
Tech: C++20/JUCE8.0.8, no dependencies.
Spec: docs/superpowers/specs/2026-11-12-native-editing-design.md

## Global constraints

Grid0.25beat, velocity90,64 undo entries; pitch0–127;800 notes/layer,1600/idea; no processBlock allocations/locks; preset version1 and type1 MIDI remain compatible.

## Review focus

Stale drag after generation must cancel; aliasing must never modify a saved snapshot; no-op must preserve redo; final-pattern short notes must remain valid; rejected edits must not change history.

## Task1: validated editing boundary

Files: Source/Editing.h, Source/Processor.h, tests/EditingTest.cpp, CMakeLists.txt.
Interfaces: EditSnapshot {juce::var idea; uint64_t revision;}; snapshot(), edit(revision,layer,index,pitch,start,duration,remove=false), quantize(revision,layer), undoEdit(), redoEdit(), canUndo(), canRedo(). index=-1 adds velocity90; remove deletes. Returns bool for accepted changed state only.
First write tests asserting deep-copy isolation, add/move/resize/delete, snap/clamp, last-note/count rejection, stale revision rejection,64-entry eviction, no-op redo retention, state/choose/generation reset and MIDI endpoint validity. Observe failure before implementing. Implement copy/validation/history without touching audio scheduling. Build editing_test then run CTest and Node tests before commit; include task.md evidence.

## Task2: real native gestures

Files: Source/PianoRoll.h, Source/Plugin.cpp, tests/UiTest.cpp.
Consumes Task1 interfaces. Roll supports scroll/pitch mapping, active-layer selection, reverse hit testing, preview during drag and revision-guarded commit. Add layer/quantize/undo/redo controls and retain provider/play/export controls. Tests must drive actual mouse methods and controls: add, move, resize, delete, quantize, undo/redo, scroll endpoints and stale gesture cancellation. Observe failure first, then implement. Run native UI and whole CTest/Node suites; record evidence in task.md before commit.

## Task3: verify/review

Build actual Linux VST3/Standalone, CTest, native HTTP success/failure scripts, Node42/42, sample comparison; inspect screenshot. Fresh whole-change code review per requesting-code-review, TDD fixes for important findings. Update plugin README manual instructions and task.md, push existing authorized branch and observe Windows/Linux CI. T10 is checked only after implementation/tests/manual procedure evidence exists; host/release gaps remain separate.
