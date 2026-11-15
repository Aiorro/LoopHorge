# Musical logic audit: historical scope, rules and generated output

Audit date:2026-11-15. Code reviewed at598ca278af149caf082590f00ae39196f9a8713d. This is a musical-logic audit of the native Composer and the Legacy/browser rule generators, not a claim about all music traditions or professional listening quality. Assumed use: one interactive MIDI composition request on the existing worker. No production code, existing test expectations or generation algorithm changed. task.md owns priorities; errors.md owns finding statuses.

Verdict: the Western twelve-pitch-class foundation, note ranges, basic scales, shared harmonic plan and MIDI timing are sound in the tested scope. The current generator is a constrained loop sketcher, not a complete historically/style-aware composition system. One ineffective musical control and three rhythm/variety coverage gaps prevent an unconditional claim that the musical logic is correct.

## Historical and musical context

Historical theory describes particular practices; it is not one universal legality test. Seventeenth-century basso continuo links a common bass/harmonic skeleton with an improvised upper texture. Eighteenth-/nineteenth-century common-practice harmony emphasizes tonal functions and directed closure. Species counterpoint instead teaches independence and treatment of intervals between lines. These are useful models, but the strict ban on parallel fifths in a first-species exercise is not a ban on rock power chords.

Later blues and pop/rock practices include looping schemata, modal harmony, pentatonic-root power chords and progressions such as V–IV–I that do not follow a textbook classical cadence. Modern diatonic modes share names with medieval church modes without being historically identical systems. Selecting a five-/six-note pitch palette also does not mean that every accompanying chord must be constructed by skipping one index in that palette. Blues melodic material can coexist with major/dominant harmony; a scale-only membership test cannot certify or reject an entire genre.

For this product, musical mechanics need separate layers: pitch material/key context, chord vocabulary and harmonic rhythm, melodic motives/development, metre/beat grouping/subdivision, groove and articulation, phrase/form, then instrumentation and listening. The existing shared harmonic plan is a useful foundation. A valid note list or a different output hash only establishes a narrow property, not idiomatic style, emotion or musical value.

Mood-to-register/density/velocity/gates/contour mappings are explicit compositional heuristics. Lower register need not mean darkness for every listener, falling contour need not mean melancholy, and extra chord tones do not always increase perceived tension. Neither this audit nor the existing tests supply controlled listener validation for those associations.

The source reading for this audit was Open Music Theory, an academic open textbook, at repository commita907aa015f7ec54b925ddb070d2d36aecd0dc705. Direct textbook/Smithsonian HTTP access was blocked by the environment; the original OMT chapter sources were actually read through GitHub. This is a focused Western tonal/pop-oriented review, not a comprehensive history of world music.

## What is correct in the inspected logic

- Theory.h has the expected interval tables for major, natural/harmonic minor, ascending melodic minor, four supported modes, two pentatonics and six-note minor blues. Negative scale degrees use floor-style octave correction. The descending melodic-minor helper returns natural minor; the generator uses the ascending collection, which can be a legitimate fixed modern/jazz palette rather than an automatic defect.
- Stacking alternate degrees makes normal diatonic triads/sevenths/extensions in seven-note collections. The C-major triad is C–E–G; harmonic minor supplies the raised leading tone. This does not generalize unchanged to every collection, as ERR-059 demonstrates.
- Harmony.h creates one chord/context timeline before bass and lead generation. Bass roots and phrase anchors refer to that timeline, including temporary parallel-key borrowing and explicit modulation. A borrowed note being outside the original key is not by itself a wrong note.
- voiceLead searches bounded closed-position inversions with a deterministic small-motion cost. It is a limited voicing optimizer, not a complete species/SATB counterpoint checker. Adding unconditional classical parallel-motion prohibitions would be inappropriate for the current genre set.
- PPQ480 and barTicks=480*4*numerator/denominator give the correct duration for supported signatures. Both6/8 and3/4 occupy1440ticks, while4/4 occupies1920. MIDI/DAW BPM here is quarter-note based; a notated dotted-quarter beat in6/8 is a different level of pulse. Correct bar length alone does not imply correct metrical grouping.
- Half/double feel changes event density without changing host BPM or harmonic duration. The five styles have distinct fixed rhythmic profiles; enabled GM drums use channel10. Existing stop/overlap/transport tests cover delivery mechanics separately from musical style.
- Legacy src/core.js and native Offline.h use the same four heptatonic pitch collections and planned per-bar progression. Their random note choices are simpler than the Composer motif system; no new musical correctness defect was confirmed on that Legacy path in this audit.

## Must fix

### ERR-059 — Blues tension changes analysis but not the notes

Locations: plugin/Source/engine/Theory.h:35, Harmony.h:26 and Mood.h:34. For the six-note blues collection, stepping by2 visits only three positions before cycling. Deduplication therefore reduces requested3/4/5/7-voice sets to the same three pitch classes. For C blues, requesting seven voices returns MIDI60,65,67 (C–F–G).

With all other settings fixed, tension0 and100 give exactly the same channel/pitch/velocity/onset/duration sequences in all five styles. The first chord's numerical tension changes from0 to85, but none of its performance changes. The actual prompt adapter also reproduces it: `4 bars blues tension0` and `4 bars blues tension100`, seed42, both parse successfully with the intended mood values and emit identical note sequences. This is an ineffective selected control, not a disagreement about whether a dissonance sounds good.

Smallest sound correction: define a collection-aware chord vocabulary/extension policy, or explicitly reject unsupported tension combinations instead of changing only metadata. Preserve deterministic existing seven-note fixtures and add a Blues control-effect regression. A separate melody palette and chord vocabulary is a reasonable future model, not an instruction to force all blues into classical harmony. If left unchanged, users can request a strong tension change and receive the same musical events.

## Should fix

### ERR-060 — Equal-length signatures have no different metrical grouping

Locations: plugin/Source/engine/Rhythm.h:36 and Generation.h:55. The numerator/denominator affect bar length, but profiles are fixed fractions of a bar; there is no separate beat/grouping model. In320matched pairs (five styles ×64seeds), changing6/8 to3/4 leaves every musical event identical; only metadata/identity differ.

With boom-bap, no humanization/ghosts/fills,6/8 produces hats at0,180,360,540,720,900,1080,1260ticks and snares at360,1080. The usual eighth-note division in6/8 is240ticks and its two dotted-quarter beats are720ticks apart. The emitted16-grid-derived pattern is possible as a deliberately unusual subdivision, but it is not an implementation of a compound-duple groove. A single coincident pattern could be intentional; always treating3/4 and6/8 identically is missing metrical semantics.

Smallest correction: represent simple/compound/additive grouping independently of total bar length and give supported genre/meter combinations explicit profiles; visibly reject combinations without a profile. Add an acceptance case that distinguishes3/4 from6/8, separate from duration tests. Otherwise users get correct-length files with the requested signature but no corresponding metre-aware groove. This is distinct from ERR-003's time-map test coverage gap.

### ERR-061 — Global swing uses different subdivision levels by layer

Locations: plugin/Source/engine/Rhythm.h:19 and Generation.h:78. Swing is based on each part's local unit parity and grid size. In4/4 boom-bap with humanization0, the straight lead attacks at0,720,960,1680ticks; swing100 moves them to0,840,960,1800. The drum hats remain at0,240,480,720,960,1200,1440,1680 because they occupy even units of a16-grid, whereas the lead uses an8-grid.

Thus initially coincident lead/hat attacks at720 and1680 split by120ticks with one shared swing control. A swung melody over straight drums can be intentional music; it is not universally illegal. The problem is that the API has one global control and no exposed policy for which subdivision/parts it swings. Generic per-part grid parity silently makes that decision.

Smallest correction: specify the common groove subdivision in musical time before per-part rhythm generation, or explicitly expose/disclose separate swing layers. Add a no-humanization alignment test for coincident musical positions at the selected swing level. Leaving the policy implicit makes a global swing request change coordination unpredictably; do not enforce identical timing for intentionally independent grooves without a design decision.

### ERR-062 — Seed variety collapses to two structural templates

Locations: plugin/Source/engine/Generation.h:58 and Harmony.h:13. With humanization0, fixed default musical settings and each of the five styles, seeds0–255 produce exactly two full note-event sequences and one harmonic plan per style. With default humanization20,256seeds produce256different performance sequences, but still just two pitch sequences and one harmonic plan in the tested default style. Comparisons use exact event vectors, not a probabilistic hash.

The cause is the seed's modulo-two motif-family choice plus seed-independent progression/onset profiles. Random timing/velocity accounts for most additional performance variation. Duplicate results for some seeds are perfectly acceptable; no seed-injectivity requirement is proposed. The issue is the very small musical search space for a composition-idea generator, especially when humanization is disabled.

Smallest correction: add bounded seeded choices for motif construction/development and phrase/rhythmic variation before microtiming, retaining intentional repetition and same-seed determinism. Measure structural diversity with humanization disabled as well as the full-event diversity. If left unchanged, repeated seed searches mostly revisit two sketches. This is a quality/coverage limitation, not proof that either sketch is bad music.

## Evidence and limits

Fresh full native CTest34/34 passed in11.74seconds. Node42/42 passed. These tests validate many useful bounds and contracts but miss the cases above; a green suite is not a proof of musical correctness. The independent C++20 audit probe uses exact vectors of channel/count/pitch/velocity/onset/duration and exact chord vectors, excluding composition IDs, seeds and analysis metadata from musical comparisons.

Probe observations:

```text
6/8 vs3/4 notes identical=1;6/8 snareTicks=360,1080, hats=0,180,360,540,720,900,1080,1260,
identical meter pairs=320/320
blues trap tension0/100 identicalNotes=1 chordSizes=3,3 tensions=0,85
blues boom-bap tension0/100 identicalNotes=1 chordSizes=3,3 tensions=0,85
blues r&b tension0/100 identicalNotes=1 chordSizes=3,3 tensions=0,85
blues ambient tension0/100 identicalNotes=1 chordSizes=3,3 tensions=0,85
blues nu-metal tension0/100 identicalNotes=1 chordSizes=3,3 tensions=0,85
straight lead=0,720,960,1680,; swing lead=0,840,960,1800,; swing hats=0,240,480,720,960,1200,1440,1680,
256seeds trap: pitchSequences=2 fullNoteSequences=2 harmonicPlans=1
256seeds boom-bap: pitchSequences=2 fullNoteSequences=2 harmonicPlans=1
256seeds r&b: pitchSequences=2 fullNoteSequences=2 harmonicPlans=1
256seeds ambient: pitchSequences=2 fullNoteSequences=2 harmonicPlans=1
256seeds nu-metal: pitchSequences=2 fullNoteSequences=2 harmonicPlans=1
default humanization20,256seeds: pitchSequences=2 fullNoteSequences=256
actual prompt path blues tension0/100: valid=1 parsedTensions=0,100 identicalNotes=1
C blues requested7voices actual=60,65,67,
```

No real-instrument/DAW listening panel, timbre/dynamics perception study, non-Western tuning review, full-world music history survey, full-song arrangement acceptance or new runtime/platform audit was performed. Existing MIDI/player/codec tests were rerun, not independently re-audited here. Live AI/provider output is outside this deterministic offline review. No production fix or completed E02/E03/E06 claim follows from this report; findings remain Open in errors.md and implementation priority is recorded only in task.md.

## Sources actually read

All chapter links below are pinned to the inspected OMT commit. Historical/contextual descriptions are paraphrased; no textbook implementation/assets were copied into the engine.

1. [A brief history of basso continuo keyboard-style voice-leading](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/bassoContinuo-history.md).
2. [Harmonic functions](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/harmonicFunctions.md).
3. [Composing a first-species counterpoint](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/firstSpecies.md).
4. [Scales and scale degrees](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/scales.md) and [Collections and Scales](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/scales2.md).
5. [Harmony in pop/rock music](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/popRockHarmony.md) and [Harmonic functions in minor](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/popRockHarmony-minor.md).
6. [Minor-pentatonic-based pop/rock tonality](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/popRockHarmony-EverettSystem5.md), [Blues-based pop/rock tonality](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/popRockHarmony-EverettSystem4.md) and [The blues progression](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/popRockHarmony-blues.md).
7. [Meter and time signatures](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/meter.md).
8. [Syncopation in pop/rock music](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/syncopation.md). This explains rhythmic levels; it does not claim that every part must always share one swing level.
9. [About Open Music Theory](https://github.com/openmusictheory/openmusictheory.github.io/blob/a907aa015f7ec54b925ddb070d2d36aecd0dc705/about.md), including its emphasis on active music making and contextual, critical use of theory.

## Reproduce the audit probe

From the repository root, extract the following standalone source into `/tmp/loopforge-music-recheck.cpp`, compile with `c++ -std=c++20 -O2 -I. /tmp/loopforge-music-recheck.cpp -o /tmp/loopforge-music-recheck`, then run that binary. It uses the production headers without JUCE, modifies no state and compares full musical-event vectors rather than saved JSON/IDs.

<!-- MUSIC_PROBE_BEGIN -->
```cpp
#include "plugin/Source/engine/Generation.h"
#include "plugin/Source/engine/PromptCompose.h"
#include <iostream>
#include <set>
using namespace composer;
std::vector<int64_t> notes(const Composition& c,bool pitchOnly=false){std::vector<int64_t> out;for(const auto& t:c.tracks){out.push_back(t.channel);out.push_back(int64_t(t.notes.size()));for(const auto& n:t.notes){out.push_back(n.pitch);if(!pitchOnly){out.push_back(n.velocity);out.push_back(n.start.value);out.push_back(n.duration.value);}}}return out;}

void starts(const Track& t,int pitch,int64_t end){for(const auto& n:t.notes)if(n.pitch==pitch&&n.start.value<end)std::cout<<n.start.value<<",";}
int main(){
 Config cfg;cfg.seed=42;cfg.humanization=0;cfg.drums=true;cfg.ghostNotes=false;cfg.drumFills=false;cfg.style=Style::boomBap;
 cfg.numerator=6;cfg.denominator=8;auto compound=*generateLoop(cfg).composition;cfg.numerator=3;cfg.denominator=4;auto simple=*generateLoop(cfg).composition;
 std::cout<<"6/8 vs3/4 notes identical="<<(notes(compound)==notes(simple))<<";6/8 snareTicks=";starts(compound.tracks[3],38,1440);std::cout<<" hats=";starts(compound.tracks[3],42,1440);std::cout<<"\n";
 int identicalMeters=0;for(int style=0;style<5;++style)for(int seed=0;seed<64;++seed){cfg.style=Style(style);cfg.seed=seed;cfg.numerator=6;cfg.denominator=8;auto a=*generateLoop(cfg).composition;cfg.numerator=3;cfg.denominator=4;auto b=*generateLoop(cfg).composition;identicalMeters+=notes(a)==notes(b);}std::cout<<"identical meter pairs="<<identicalMeters<<"/320\n";
 for(int style=0;style<5;++style){Config base;base.seed=42;base.humanization=0;base.scale=Scale::blues;base.style=Style(style);Mood low,high;low.tension=0;high.tension=100;auto a=*generateMoodLoop(base,low).composition;auto b=*generateMoodLoop(base,high).composition;std::cout<<"blues "<<styleName(base.style)<<" tension0/100 identicalNotes="<<(notes(a)==notes(b))<<" chordSizes="<<a.chords[0].voicing.size()<<","<<b.chords[0].voicing.size()<<" tensions="<<a.chords[0].tension<<","<<b.chords[0].tension<<"\n";}
 Config b;b.style=Style::boomBap;b.humanization=0;b.drums=true;b.ghostNotes=false;b.drumFills=false;b.swing=0;auto straight=*generateLoop(b).composition;b.swing=100;auto swing=*generateLoop(b).composition;
 std::cout<<"straight lead=";for(const auto& n:straight.tracks[2].notes)if(n.start.value<1920)std::cout<<n.start.value<<",";std::cout<<"; swing lead=";for(const auto& n:swing.tracks[2].notes)if(n.start.value<1920)std::cout<<n.start.value<<",";std::cout<<"; swing hats=";starts(swing.tracks[3],42,1920);std::cout<<"\n";
 for(int style=0;style<5;++style){std::set<std::vector<int64_t>> pitches,all;std::set<std::vector<int>> harmonies;Config q;q.style=Style(style);q.humanization=0;for(int seed=0;seed<256;++seed){q.seed=seed;auto c=*generateLoop(q).composition;pitches.insert(notes(c,true));all.insert(notes(c));std::vector<int> h;for(const auto& chord:c.chords){h.push_back(chord.root);h.insert(h.end(),chord.voicing.begin(),chord.voicing.end());}harmonies.insert(h);}std::cout<<"256seeds "<<styleName(q.style)<<": pitchSequences="<<pitches.size()<<" fullNoteSequences="<<all.size()<<" harmonicPlans="<<harmonies.size()<<"\n";}
 Config defaults;defaults.humanization=20;std::set<std::vector<int64_t>> defaultPitches,defaultNotes;for(int seed=0;seed<256;++seed){defaults.seed=seed;auto c=*generateLoop(defaults).composition;defaultPitches.insert(notes(c,true));defaultNotes.insert(notes(c));}std::cout<<"default humanization20,256seeds: pitchSequences="<<defaultPitches.size()<<" fullNoteSequences="<<defaultNotes.size()<<"\n";
 auto lowPrompt=LocalPromptInterpreter{}.interpret("4 bars blues tension0",42),highPrompt=LocalPromptInterpreter{}.interpret("4 bars blues tension100",42);
 auto lowResult=generateResolvedLoop(lowPrompt),highResult=generateResolvedLoop(highPrompt);
 std::cout<<"actual prompt path blues tension0/100: valid="<<(lowPrompt.valid()&&highPrompt.valid()&&bool(lowResult.composition)&&bool(highResult.composition))<<" parsedTensions="<<lowPrompt.config.mood.tension<<","<<highPrompt.config.mood.tension<<" identicalNotes="<<(notes(*lowResult.composition)==notes(*highResult.composition))<<"\n";

 auto pitches=chordPitches(Key{{0},0,Scale::blues},0,7,4);std::cout<<"C blues requested7voices actual=";for(int n:*pitches)std::cout<<n<<",";std::cout<<"\n";
}
```
<!-- MUSIC_PROBE_END -->
