# Musical correction review

Base7dfadef; user authorized immediate resolution of audit findings ERR059–062 without further design questions. Four regression branches each failed on the old production headers. Native algorithm4 changes new generation only; persisted source and Legacy generation remain compatible.

ERR059 fills unused pitch classes after an exhausted alternate-degree cycle. ERR060 derives denominator-based group profiles and preserves selected source rhythm/density. ERR061 applies a shared continuous time warp and bounded short-bar headroom. ERR062 constructs seeded motifs before microtiming. New default straight seed42 golden5970328224193018206 records the intentional version4 algorithm change; existing timing/identity/boundary assertions remain enabled.

Existing RhythmTest caught two implementation defects before completion: expanded contours could exceed instrument range, and short-bar swing clamping could collapse onsets. Register folding and headroom-limited warp fixed them; strict generation/rhythm/mood matrices pass.

Fresh independent gpt-6-astra/high review found no Critical and two Important regressions. A separate standard-only probe independently generated50000valid configurations across metres/scales/styles/feels, maximum humanization and varied swing/complexity/contours, with valid data and strictly ordered bass/lead onsets. No second review substituted for regression tests.

One fix pass:

- Non4/4 metre profiles discarded complexity/style selections: a new6/8 low/normal complexity and style-rhythm regression failed before snapping the selected original pulse profiles to denominator subdivisions. The test now requires different low/normal events and at least4distinct bass onset patterns across5styles. Snare/hat grouping stays metre-aware.
- Unconstrained seed perturbations overrode explicit rising/falling contours: trap seed1/2 interior direction regression failed. Deviations now apply only to Contour::seeded; explicit rising/falling source offsets retain their original direction. Seeded structural diversity is tested separately from explicit contour constraints.

Full post-fix verification is recorded in task.md. Sanitizers cover the actual standard-only music helpers, not linked host/SDK code. Independent review did not rebuild the shared JUCE targets.

Final: Ruling: Sparse anchor-only phrases can have fewer seed variants, e.g.2/4 at humanization0. The diversity threshold applies to default8bar4/4 free-contour generation, not every seed/configuration. Cost if wrong: short or explicitly constrained phrases can still repeat; no seed-injectivity guarantee or full variety acceptance.

Final: Ruling: Generic metre grouping, palette-limited Blues extensions, unchanged fixed harmonic progressions and root phrase anchors are explicit bounded music policies, not universal laws. Cost if wrong: idiomatic/style and listening quality may remain limited; E02/E06 musical acceptance stays open.

Final: Ruling: Listening quality, historical authenticity and full host/audio/resource/platform certification remain unverified; this pass changes worker-only music helpers. Cost if wrong: subjective/DAW/resource issues remain possible; existing errors and E06 acceptance remain open.

No new deferred Minor finding from this review. Previous ERR003 coverage remains separate. All reproduced Critical/Important findings in this fix scope must pass the full suite before Fixed status/publication.

Final verification: full Linux VST3/Standalone build and CTest35/35 passed after both review fixes; Node42/42/sample equality; standalone musical-audit/Rhythm/Mood ASan/UBSan passed. No unaddressed Critical/Important/new deferred Minor in this fix scope. Registry errors059–066 Fixed with scope; other resource/platform/coverage findings remain open.
