# Windows release execution blocker

Observed source:37811b1096d88d5ee3ac26198fb303e2c77aaeb6 on main.

The generic GitHub Fetch API can read push-triggered workflow runs; the narrower commit-workflow wrapper only reads PR events. Do not interpret an empty result from that narrower wrapper as absence of a main run.

Native run44075981778 (push) completed failure. Full jobs API shows native Windows check/job134013990370 and Ubuntu134013991006 both runner_id0,steps[]. No compiler or test step execution is observed. Publisher134014011005 is skipped because its prerequisites failed. The Releases collection returned[]; no Windows VST3/EXE release exists at this checkpoint.

The commit check-runs API reports one Windows annotation at https://api.github.com/repos/Aiorro/LoopHorge/check-runs/134013990370/annotations. The GitHub Fetch connector rejects that endpoint as outside its allowed repository/search endpoints (400 INVALID_ARGUMENT). This is a tool capability restriction, not evidence of the annotation's content. Prior log downloads returned404BlobNotFound. Billing, quota, runner eligibility or account restrictions must not be asserted as the cause without reading that annotation.

Required continuation: owner reads the Native run Summary/Annotations and supplies the exact failure text, or provides an approved interface capable of reading it. Correct the named Actions restriction, then build the current main with supported Windows MSVC. Successful build/test/package/publisher output and real Windows/FL tests still need verification. Retrying source edits or changing compilers does not fix an unassigned runner. No JUCE compiler guard, license condition or release prerequisite was weakened.
