# Preserved demo samples

Imported unchanged during main consolidation on2026-11-17, at the user's request to retain all final repository work. These are historical demonstrations, not a claim that the current generator emits identical bytes.

- loopforge-demo.json: bb3d185ab0ac36fdd5f73289b455616c08418c59096fcf0a2a701c70bc6f20b5; identical in all three worktrees.
- loopforge-demo.mid:11acd50c43d09c3ef42fdd30a794bda3997e440c71ef3a8f282c027c02868316; original/active worktrees,991bytes.
- variants/loopforge-demo-qwen.mid:a9b5c7ba76c8d377d2d289818e8b134429734b73d4e2021c9691190e9ca6237c; Qwen worktree variant,992bytes.

Both MIDI files have valid SMF container lengths. No existing sample was overwritten or regenerated. Build outputs/credentials are not sample assets.

## Current generator output

Run `npm run sample` to write the current MIDI and JSON under `samples/generated/`.
The generated MIDI matches [the Qwen variant](variants/loopforge-demo-qwen.mid) byte for byte. The generated JSON matches [the preserved JSON](loopforge-demo.json).
