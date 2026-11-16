# ERR070 CLI root-exit lifecycle
Continue assigned queue, no feature work. Fix root-exit/inherited-pipe completion while preserving bounded stream draining, output limits, abort/deadline and existing Windows Job Object ownership. On POSIX observe root exit, kill remaining ordinary process-group descendants, then finish at close after pipe data drains. Windows helper remains owner of its Job Object tree. No claim of deliberately detached/setsid descendant containment.

1. Real fake-root tests for inherited stdout/stderr/both, success/nonzero exit, resistant descendant cleanup and complete buffered UTF8 output; fail before implementation.
2. Minimal root-exit cleanup separate from result finalization. Keep data/size guard until close; timeout/abort paths still single completion.
3. Full Node/native baseline/sample and fresh independent review; update ERR070/task/PR on actual evidence. Next ERR069 MCP cancellation remains separate.
