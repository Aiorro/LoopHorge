# ERR070 CLI root-exit verification

Base42525cb. Continue assigned remediation order under feature freeze. Six real fake-CLI cases first failed with timeout: ordinary resistant child inherits stdout/stderr/both, main root exits0/7 after flushing65536bytes plus UTF8 prefix. Existing stdio-ignore test did not catch this.

Minimal change observes POSIX root exit and terminates remaining ordinary process-group descendants. Finalization remains at close, keeping stream data/size limits active until buffered output drains and preserving root result/status. Existing timeout/abort/output-limit single completion and Windows Job Object helper ownership remain unchanged.

GREEN: focused CLI14/14, Node61/61/sample byte equality, unchanged-native baseline CTest36/36. The original isolated audit probe under500ms deadline returns completed in39ms and reports descendantAlive=false (local timing only, not a universal deadline guarantee).

Fresh independent gpt-6-astra/high review: no Critical/Important/Minor. Independently ran focused14/14 on Linux/Node24.22.1, inspected result draining/cleanup/error/abort/limits/callers and Windows helper source; whitespace check passed. No second review needed.

Final: Ruling: POSIX groups cover ordinary descendants; deliberately detached/setsid children remain ERR011. Cost if wrong: escaped processes can outlive a request; this fix does not certify malicious containment.

Final: Ruling: Windows helper source is unchanged; actual Windows and signed-in CLI execution were not run. Cost if wrong: platform/provider-specific exit or timing behavior may differ; ERR004/006 and live acceptance remain open.

Final: Ruling: MCP cancellation/session shutdown and synchronous readiness latency are separate queued defects ERR069/073. Cost if wrong: those existing request/resource/latency failures remain possible until their own verified corrections; feature freeze stays active.

No deferred Minor from this review. No schema/native algorithm/production dependency changes. Next assigned ERR069.
