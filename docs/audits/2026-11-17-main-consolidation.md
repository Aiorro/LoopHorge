# Main consolidation audit — 2026-11-17

What this repo does: LoopForge generates offline MIDI loops, offers optional AI/CLI bridges, and has browser plus JUCE VST3/Standalone interfaces. Assumed load: one desktop music session, multiple plugin instances, audio blocks32/64samples and bounded concurrent local API/MCP requests; no multi-user hosted service.

## Must fix before a test binary release

1. Current Windows/FL execution is absent (ERR005/006, task R04/R06). Native44042462736 fails both jobs without usable step/log metadata or artifacts. Linux binaries cannot substitute for a Windows EXE/VST3. Restore actual Windows CI, then record EXE launch, FL scan/clock/routing/FLP reload and archive SHA. Skipping this leaves no Windows deliverable; integrating source to main does not close the release gate.

2. The packaged workflow still lacks scoped regeneration, remaining controls and seamless standalone looping (task R02). Concrete case: standalone reaches the endpoint and does not repeat. Keep the alpha limitation visible and complete the finite loop scenario before calling the product finalized. No features were falsely marked complete in this integration.

## Should fix / verify before release

3. Aggregate workspace peak memory is unmeasured (ERR007). Preparation, JSON, history, view and export can coexist although individual structures are bounded. Measure full16bar generate/edit/history/save/export and reject work before an actual aggregate budget is exceeded. Component bounds do not certify256MiB.

4. File IO remains synchronous on the message thread (ERR009, ProjectFiles.h). A delayed filesystem blocks UI during project/MIDI/recipe chooser operations. Measure slow storage and move IO to a bounded worker if required; existing staged writes preserve prior files but do not prove UI responsiveness.

5. POSIX cancellation only owns process groups (ERR011, src/agent-cli.js). A deliberately setsid-escaping descendant survives group kill; ordinary children are tested. Keep this containment boundary explicit; choose stronger supervision if hostile CLI descendants are in scope. No paid provider/host authentication was exercised.

6. Browser export revocation remains acceptance work (ERR077, web/app.js). Download URL disposal is delayed, but actual browser download completion remains unverified. Test downloaded MIDI bytes in supported browsers before marking the finding closed.

## Fixed during this audit

- ERR089: tracked staged/unstaged edits could be omitted from Sources/LoopForge.tar because packaging always archives HEAD. A new regression failed before the guard; packaging now rejects tracked changes before output creation. Untracked user samples are allowed and excluded from archives. This enforces committed source identity; it cannot authenticate arbitrary externally supplied binary inputs.
- Packaged instructions still said no open licensing route had been chosen, contradicting the user's AGPLv3/GPLv3 decision and current source delivery. Updated to selected terms while retaining target dependency/source-correspondence checks. Original MIT notices remain preserved.

## Nice to have

7. Mood panel return says SHOW OVERVIEW while returning to the prior piano roll (Plugin.cpp). Destination works but the label is misleading. Prior review minor remains deferred.

8. Concurrent host restore between seed parsing and input sync may pair refreshed direction with an earlier fallback seed (Plugin.cpp composeLoop). This is a prior unconfirmed review minor, not a newly reproduced defect. Add a controlled interleaving test before claiming universal concurrency safety.

## Repository and verification evidence

Remote main28a4f91 is an ancestor of the integration tree; no main-only commits need conflict resolution. Both native v0.2 and Qwen histories are retained. Old Qwen uncommitted agent-cli.js and its readiness cleanup test are byte-identical to current tracked files; no unique code remains stranded there. Distinct user MIDI samples are retained unchanged with hashes in samples/README.md. No force push, history rewrite, branch deletion or worktree deletion is required.

Reviewed root contracts, task/errors, manifests/workflows, server/bridge/provider/CLI/MCP entry flows, checked file writes, Processor publication/RT ownership and package provenance. Independent fresh read-only review found the tracked-source and licensing-doc issues above; no additional confirmed high/medium issue in its reviewed AI/CLI/server/MCP paths. One regression-driven correction pass, no re-review.

Fresh Linux all-target build and CTest43/43 pass (/tmp/main-audit-build.log, /tmp/main-audit-native.log). Full Node95/95,0failed,0skipped after the source guard is recorded in task.md and /tmp/main-audit-node-final.log. New tests cover both staged and unstaged rejection. Original15Txx rows and engine specification remain preserved. Samples checked for complete SMF chunk boundaries; no output regeneration overwrote user files.

Verdict: suitable to consolidate the tested alpha source into main under the user's explicit authorization; not ready to claim a Windows release or FL compatibility. Remaining acceptance stays in task.md/errors.md.

Not checked: actual Windows/DAW/pluginval/OS chooser/drag/audio devices, live paid providers, whole-process memory/slow storage, power-loss recovery, universal concurrent/allocation schedules, legal certification, every theoretical input to all engine algorithms. Historical reports were not rewritten as current proof. No generic lean refactor was justified by this integration audit.
