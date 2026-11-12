# T10 native note editing design

Status: proposed; user approval pending. Independent builder check: nothing important left. This is a design, not an additional task list. task.md remains the authoritative checklist.

## Intent and scope

Let a musician refine any of the eight generated ideas inside the existing JUCE editor before playback/export. Keep the existing generator, MIDI scheduling, provider controls and state format. The current roll is display-only; selection, playback and type1 export already exist.

Implement layer selection, note add/move/resize/delete, quarter-beat quantization, undo/redo and full MIDI pitch access by scrolling. Do not add velocity automation, instruments, drag-to-DAW or claim real-host acceptance; those remain outside this slice.

## Approach

Recommendation made by the implementer: extract a dedicated JUCE piano-roll component and keep validated editing/history in the non-RT processor boundary. Extending Editor's existing paint method directly has less initial plumbing but couples hit testing and gesture state to provider/file controls. A dedicated component permits actual gesture tests and keeps these concerns manageable without adding a framework or dependency. Existing task.md already specifies the snapshot handoff and editing operations; this proposal supplies their detailed semantics. The user has not yet approved this design.

## Interface

Retain provider/model/generation/play/export controls. Above the roll add Bass/Chords/Melody selection, Quantize, Undo and Redo. Display all layers in their existing colours; only the active layer is editable. Use the existing fixed editor size and a vertically scrollable pitch grid spanning MIDI0–127 with named octave rows. Initially show the active layer's pitch range. Horizontal position spans the entire4/8-bar pattern, with bar/beat/subdivision lines; resizing the roll preserves musical coordinates.

Double-click empty space adds a note at the snapped position/pitch with velocity90 and one-beat duration (clamped at the pattern end). Left-drag a note moves pitch and start; dragging its right edge resizes duration. Right-click or Alt-click deletes the hit note. A selected-note highlight distinguishes the gesture target. Quantize affects all starts and ends of the active layer. Undo/Redo buttons are enabled from actual history availability; no keyboard shortcut is required for this slice.

## Musical bounds and gesture semantics

The grid is0.25beat; round nearest with half steps towards the later grid line. Clamp starts to[0,patternEnd-0.25], durations to at least0.25beat, and ends to patternEnd. Moving a note retains duration and clamps the start accordingly. Pitch is an integer0–127. Existing sub-grid notes remain unchanged unless edited or quantized. Quantize rounds both edges and guarantees at least0.25beat, moving the start back if necessary at the final boundary.

Resolve overlapping hit targets by reverse draw order. An edit operates on one layer/index in an immutable snapshot; a revision token guards that index against replacement. Pointer movement shows a local preview only. Mouse-up publishes one validated edit and creates one undo entry; no-op gestures create none. A changed revision during a gesture cancels it. Quantize that changes nothing is also a no-op. Reject deletion of the last note in the whole idea and additions exceeding800/layer or1600/idea; show a concise status without losing the composition.

## State, history and concurrency

Processor captures a deep copy of the selected idea with a monotonic revision while holding its existing non-RT mutex. Mutations require that revision, validate the complete candidate with the existing schedule validator, and publish through the existing three-slot schedule handoff. Never mutate JUCE var arrays/objects shared with saved state, the idea browser, export or undo snapshots. An accepted edit updates both selected variation and saved idea, marks the host state changed through the JUCE-supported non-RT notification path and increments revision.

Keep at most64 undo snapshots for the currently selected idea, plus redo snapshots. A new edit clears redo; undo/redo validate and publish before changing history. Selection, successful generation and state restoration clear history and increment revision. History is not serialized; edited current state uses the existing version1 preset format. Rejected edits/publications leave history, revision and authoritative composition unchanged. Late generation is an authoritative replacement, invalidating an in-flight gesture. Closing the editor cancels its local preview.

Audio callbacks consume only bounded schedule snapshots. No edit/history/JSON/UI locking or allocation enters processBlock. Schedule replacement uses existing note-release behavior. Export captures the committed current composition and uses encodeMidi; an unfinished drag preview is not exported.

## Verification and acceptance

TDD processor coverage must demonstrate actual deep-copy isolation, each edit operation, bounds/capacity/last-note rejection, stale revision rejection, selected-idea updates,64-entry history eviction, redo invalidation and state/generation/selection history reset. Actual MIDI decoding verifies edits reach exported bytes and retain pattern endpoints. Existing transport/overlap/realtime tests must remain green.

Real JUCE message-loop tests should dispatch mouse gestures and invoke controls, check committed notes/history and cancel a stale drag across replacement. Include pitch scrolling and final-pattern boundaries. Run the full Node suite, native build/CTest, HTTP scripts, sample byte comparison and Windows/Linux CI. Request independent code review after implementation. Actual DAW and release acceptance remain T12/T14; T13's wider analysis gaps stay explicit.

## Implementation calls

The recommended component boundary,64-entry history limit, velocity90, gesture mapping and rounding policy are implementer proposals. They can change before approval. Runtime handoff failure must be reported rather than silently claiming an edit succeeded. No additional dependencies or changes to the canonical15-task structure are needed.
