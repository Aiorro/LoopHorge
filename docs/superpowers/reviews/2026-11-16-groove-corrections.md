# Groove corrections and metre coverage — 2026-11-16

## Scope and root causes

Based on8582725 and task.md remediation queue8/9: ERR072 reset swing phase at every bar although half-time source patterns span two bars; the onset helper did not receive bar origin. ERR071 mapped both a one-numerator strong snare and its ghost to the same midpoint, then strongest-velocity canonicalization removed the ghost. ERR003 was a coverage gap, not a reproduced validator arithmetic defect.

Swing now uses barStart+base for its shared periodic phase, passed by bass, lead and drums. Existing bar-end headroom and jitter bounds remain. Unit0 stays free of random jitter while following swing phase. In1/x metre the ghost precedes the strong midpoint at sourceunit0; other ghost profiles are unchanged. Half/double-time still transform source patterns without changing tempo. New generation uses engineVersion5/source-v5/loop-v5 identity; committed saved compositions are decoded with their existing versions/events. Default straight4/4 seed42 note golden remains5970328224193018206.

## Test-first evidence

- MusicalAuditTest part7 failed the literal3/4half-time snare oracle before the fix (`/tmp/loopforge-swing-red.log`); part8 failed quiet-note/count-effect checks before the ghost fix (`/tmp/loopforge-ghost-red.log`). Part7 also specifies5/4onsets2880/7680; both metres retain absolute snare positions with swing100.
- Expanded part7 checks actual bass/lead/drum events against absolute swing position at360style/odd-metre/denominator/feel configurations, with humanization0, fills/ghosts off; counts/pitches and bar/document validity remain. Part8 checks720style/1-x/feel/swing/humanization/seed configurations: ghosts add distinct quiet snare attacks and retain each original note's start/pitch/velocity. Durations/IDs may change under existing canonicalization.
- Version expectation first failed at engineVersion5 (`/tmp/loopforge-groove-version-red.log`) then passed with metadata update and unchanged musical golden.
- CompositionTest adds0:4/4→1920:3/4→3360:5/8acceptance plus4320/3361maps rejection. Existing validator passed; no production validator change was needed or claimed.

## Final verification

- Full Linux native VST3/Standalone/all-target build passed: `/tmp/loopforge-groove-build.log`.
- CTest37/37,0failed: `/tmp/loopforge-groove-ctest.log`, including expanded musical audit, previous3440rhythm-boundary tests, generation/mood golden, processor/RT/allocation, state, export and UI tests.
- `npm test`:84/84,0failed,0skipped: `/tmp/loopforge-groove-node.log`.
- `npm run sample` passed in the isolated Qwen review worktree using the identical unchanged Node music source: `/tmp/loopforge-groove-sample.log`. User samples in the main worktree were not touched.
- Fresh standard-C++20 ASan/UBSan builds/runs passed for MusicalAuditTest and RhythmTest: `/tmp/loopforge-groove-{audit,rhythm}-san.log`. This is generator instrumentation, not whole-JUCE/SDK/DAW sanitizer certification.
- Fresh independent read-only review found no Critical/Important issues. Its independent C++20 probe passed46080one-numerator ghost comparisons across all styles/feels/denominators/swing0/50/100/humanization100/seeds0–255, preserving every original attack's start/pitch/velocity and adding notes. No additional Minor finding was raised.

## Ruling and boundaries

ERR003/071/072 are Fixed in this local regression scope. Register77unique IDs:70Fixed/4Open/1Investigating/2Blocked. Resource/peak-memory, message-thread storage responsiveness, deliberate process-session escape, browser download acceptance and external Windows/DAW/pluginval/chooser/drag checks retain their statuses. No objective stylistic-authenticity/listening certification, release or main merge is implied. Original15Txx rows and supplied task-engine payload are preserved; E02/E03/E04 remain incomplete. Sole actionable queue: [task.md](../../../task.md); next feature scope is E03 manual/Auto/mood UI and recipe persistence.
