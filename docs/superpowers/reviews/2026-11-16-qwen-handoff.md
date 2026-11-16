# Qwen handoff review — 2026-11-16

Reviewed remote commit `2f24731ba597a72bf44e1d179c256266683e86c0` against `e67008c`. An isolated detached worktree at `/workspace/worktrees/loopforge-qwen-review` preserves both the remote work and unfinished local MCP/Composer-export changes. No merge or product-source replacement was performed during this review.

## Verification

`node --test --test-concurrency=1`: 77 tests, 77 pass, 0 fail, 0 skipped on Linux, 16.4 seconds. Log: `/tmp/loopforge-qwen-review-tests.log`. This independently verifies the existing suite, not every lifecycle edge. Windows results in the commit remain author-reported. Native source was unchanged by that commit.

## Findings requiring integration corrections

1. **P2 / ERR069 — duplicate active IDs lose cancellation ownership.** `src/mcp-server.js:50` overwrites the active entry without rejecting an existing ID. Reproduction: send two controlled pending generate requests with ID7, then cancel7. The first signal remains un-aborted and only the second is aborted. Shutdown cannot find the first controller and awaits its still-pending promise indefinitely. Reject duplicate active IDs before launching work; preserve numeric/string distinction.
2. **P2 / ERR069 — notifications and invalid IDs launch unowned work.** `src/mcp-server.js:48–54` launches tools/call with no ID, while only string/number IDs enter active. Reproduction: send a tools/call notification and request shutdown; the signal is not aborted and exit is never reached while the job remains pending. Do not execute tool-call notifications; reject invalid request IDs before launching work. Apply the capacity limit to actual owned tool jobs, not distinct overwritten IDs.
3. **P2 / ERR073 — locator timeout can leave the locator alive.** `src/agent-cli.js:20` sends one default kill, then resolves false after another second without confirming close or escalating. A controlled Node locator ignoring SIGTERM returned false at3006ms while its PID remained alive; the review explicitly killed it afterward. Bound cleanup and verify close before reporting lifecycle completion. Async responsiveness itself is improved and the new existing tests pass.

The MCP implementation also has no bounded blocked-output/shutdown path. Preserve the independently developed local backpressure/cleanup regressions when reconciling; the remote six lifecycle tests do not cover these cases.

## Integration ruling

Retain Qwen's Node MIDI overlap fix, asynchronous readiness direction, server robustness changes and honest browser-verification limitation. Integrate overlapping MCP files deliberately; do not replace the more extensive local lifecycle tests wholesale. ERR069/073 are not considered fully closed by this review despite their remote Fixed labels. Keep existing IDs074–077 from the remote register when integrating; do not allocate them to other findings. The MIDI 200-seed reference repeats much of production event logic, so its decoder is independent but its semantic oracle is not fully independent; focused expected-event tests provide additional evidence.

The sole actionable remediation queue remains [task.md](../../../task.md). The latest user authorization to continue scoped features takes precedence over the older feature-freeze wording reintroduced in the remote handoff. Composer export work and user sample files remain preserved.
