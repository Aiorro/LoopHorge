# Portable Composer Recipes

Continue approved E03 architecture inline with TDD and one final independent review; task.md owns progress.

Deliver separate bounded recipe JSON rather than alter strict project-v2. Version1/kind/engineVersion5, direction4096UTF8bytes, uint32seed, seven mood values0–100 including retained Auto values, seven pin bits. Unsupported pins, fields, duplicate/missing keys, invalid UTF8/types/ranges and unsupported versions reject. Draft directions may be saved even if the prompt cannot generate yet. Reproducibility uses the same engine5 backend; no legacy algorithm emulation.

New RecipeJson.h codec and RecipeTest cover roundtrip, deterministic regeneration, invalid limits. ComposerMoodControls::apply restores values and Auto state. Plugin recipe actions capture save bytes before shared guarded chooser; checked staging preserves old file on failure. Load reads<=32KiB before allocation, decodes fully then updates next-request controls only. No auto compose/current-note mutation/project schema change. Tests include missing codec/apply RED, mapping, file cap and buttons. Full build/CTest/Node plus codec sanitizers and fresh review before push. Automatic recipe persistence with host/project and other manual fields remain E03.
