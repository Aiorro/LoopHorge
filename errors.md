# LoopForge error register

Updated: 2026-11-15. Imported from recorded reviews/regressions in [task.md](task.md); this is not a new whole-repository audit. task.md remains the sole execution checklist and priority source. This register has stable finding IDs, statuses and evidence, not competing task checkboxes.

**Open:** unresolved finding/risk. **Investigating:** observed failure, cause unproven. **Blocked:** unavailable infrastructure/host/tools prevent verification. **Fixed:** regression passes only in the stated scope, not a release/DAW claim. Missing staged Composer features remain in task.md; missing-API TDD steps and fixture typos are not product defects.

## Active findings and acceptance risks

| ID | Severity | Kind | Status | Finding | Evidence / verification scope | Task |
| --- | --- | --- | --- | --- | --- | --- |
| ERR-003 | Low | Coverage gap | Open | Meter regression uses equal-length6/8→3/4; different-length/nonzero-origin coverage remains missing. | Foundation review; current arithmetic assessed correct. | E02/E06 |
| ERR-004 | Medium | CI failure | Investigating | Windows Node22 CLI availability returned unavailable after2222ms against a2000ms deadline. | Workflow43400295998; not reproduced on Windows here. Do not weaken tests without a proven cause. | T13/E06 |
| ERR-005 | High | Infrastructure | Blocked | GitHub jobs finish without runner assignment or executed steps. | At325d70f: Native43736828671/Prototype43736828619 have runner_id0/steps0 across8jobs; cause unknown. No compilation/test step executed. | T07/T13/E06 |
| ERR-006 | High | Acceptance gap | Blocked | Fresh Windows/real DAW/pluginval/OS drag and chooser acceptance is unverified. | Linux/component screenshots do not establish platform execution; see NATIVE-DAW-ACCEPTANCE.md. | T12–T15/E06 |
| ERR-007 | High | Resource gap | Open | Whole256MiB workspace and transient memory peak are not measured. | Retained64MiB accounting excludes other jobs, JSON/UI/history/scratch. | E04/E06 |
| ERR-008 | Medium | Failure-handling risk | Open | Severe allocation failure during commit may leave partial control state. | Explicit review boundary; no unconditional transactional OOM guarantee. | T13/E06 |
| ERR-009 | Medium | UI limitation | Open | Slow storage/large project IO can block the message thread. | Bounded synchronous chooser IO stays outside audio; responsiveness not certified. | E04/E06 |
| ERR-010 | High | Lifecycle gap | Open | Final MIDI delivery at releaseResources/reprepare/destruction is not host-verified. | Normal stop/seek/replacement tested; stopped ownership alone cannot deliver teardown releases. | T12/T13/E06 |
| ERR-011 | Medium | Process boundary | Open | POSIX cancellation does not certify deliberately session-escaping descendants. | Prior CLI review excludes setsid escape; ordinary group/tree cases have evidence. | T13 |

| ERR-059 | Medium | Musical control | Open | Blues tension0/100 can emit identical notes while numerical chord tension changes; alternate-degree stacking cycles through only3of6pitch classes. | Exact event comparison in all5styles plus real prompt-adapter reproduction; audit docs/audits/2026-11-15-musical-logic.md. | E02/E03 |
| ERR-060 | Medium | Metre coverage | Open | Rhythm profiles use total bar length without simple/compound beat grouping;6/8 and3/4 always share note patterns in the tested matrix. |320/320exact note-event pairs identical across5styles×64seeds;6/8 boom-bap hats use180tick steps instead of an eighth-note240tick division. Unusual tuplets are legal; metre-aware profiles are missing. | E02/E06 |
| ERR-061 | Medium | Groove policy gap | Open | One global swing control applies different rhythmic levels to lead and drums, splitting originally aligned attacks without an explicit layer/subdivision policy. | Humanization0/boom-bap: lead720→840 while hat stays720; independent grooves can be valid, but the shared-control policy needs definition. | E02/E03 |
| ERR-062 | Medium | Musical variety gap | Open | Native seed search mostly changes microtiming/velocity; with humanization0 it collapses to2full musical sequences per style in seeds0–255. | Exact vectors:2pitch/full sequences and1harmonic plan per style; default humanization20 gives256performance sequences but only2pitch sequences. Not a requirement that every seed be unique. | E02/E06 |

## Fixed findings and recorded regressions

Native evidence below is local Linux unless a row explicitly cites earlier platform CI. Detailed commands, historical dates and review boundaries remain in task.md.

| ID | Severity | Kind | Status | Finding / correction | Evidence / verification scope | Task |
| --- | --- | --- | --- | --- | --- | --- |
| ERR-001 | Low | Bug | Fixed | POSIX drag descriptors could survive child exec and prolong a file lease. | Atomic O_CLOEXEC on all three opens; real fork/exec child readiness/release/cleanup regression RED→GREEN on Linux2026-11-15. Fork-only children are not an exec guarantee. | T13 |
| ERR-002 | Low | Bug | Fixed | Retained completed/failed drag callbacks could keep a file lease alive. | Shared atomic lease hold clears on completion/rejected start; retained/copy/concurrent/repeated callback and source-destruction tests RED→GREEN, ASan/UBSan/TSan scoped. | T13 |
| ERR-012 | High | Integration | Fixed | Original divergent v0.2 work/task rules reconciled without replacing the checklist. | Recorded merge3e46129;15original task rows preserved. Current feature integration remains draft, not main/release completion. | T01/T02 |
| ERR-013 | High | MIDI | Fixed | First Note Off could cut a later overlapping same-pitch note; retriggers needed balanced edges. | Counted lifetimes; NoteLifetime/Prepared/Playback/ComposerProcessor tests. | T08/T13/E04 |
| ERR-014 | High | MIDI | Fixed | Stop/seek/loop/replacement could leave notes sounding, including negative PPQ. | Explicit continuity and flush regressions. Teardown remains ERR-010. | T08/T13/E04 |
| ERR-015 | High | Timing | Fixed | Tempo changes skipped/duplicated notes at the previous sample boundary. | Literal .00095/.00195beat RED→GREEN at32/64samples. | T13 |
| ERR-016 | High | MIDI output | Fixed | Exceeding the2048event converter cap silently dropped output/releases. | Reserved cleanup, bounded output and real VST3 converter counts. | T13/E04 |
| ERR-017 | High | RT safety | Fixed | Small JUCE MIDI reservations could allocate in tested callbacks. | Guarded32768byte preparation and allocation/free hooks; prepared caller buffers remain required, not whole-wrapper proof. | T13/E06 |
| ERR-018 | Medium | Installation | Fixed | Native eight-idea generation depended on localhost Node4177. | Native offline legacy generator/parity/OfflineTest; native Composer loop request/UI additionally verified2026-11-15, full Composer acceptance remains open. | T09 |
| ERR-019 | Medium | Provider UI | Fixed | Model ID/probe/auth diagnostics/cancel controls were absent. | Native controls and mocked API/failure tests; live paid credentials not claimed. | T08/T11 |
| ERR-020 | High | CLI lifecycle | Fixed | Abort/timeout/overflow/root exit left children; Windows taskkill had creation/assignment races. | POSIX groups and atomic Windows Job Objects; recorded lifecycle CI. Escape boundary is ERR-011. | T03/T11/T13 |
| ERR-021 | Medium | Provider compatibility | Fixed | Blank model was rejected instead of selecting provider default. | Blank-to-absent normalization/probe regression. | T11 |
| ERR-022 | High | Parsing | Fixed | Integer wrap/bool versions/malformed tokens/NUL-hidden trailing bytes bypassed validation. | Raw bounded tokenizer and Wire/Processor regressions. | T08/T13 |
| ERR-023 | High | Protocol | Fixed | Incomplete envelopes, trailing JSON and inconsistent deadline/error codes. | HTTP/schema/failure regressions and recorded v1 CI. | T08 |
| ERR-024 | High | MIDI export | Fixed | Positive short durations became zero ticks; full tail/meter was missing. | One-tick/full EOT/map checks in MidiExportTest. | T10/T13 |
| ERR-025 | Medium | Selection | Fixed | Variation picker stayed on an old idea after generate/restore. | Real message-loop UI and authoritative selection sync. | T10 |
| ERR-026 | Medium | Native editor | Fixed | Scrollbar clipped piano endpoint; new bass stayed outside viewport. | Attachment relayout, width/replacement recenter regressions. | T10 |
| ERR-027 | Medium | Native editing | Fixed | Resize handles prevented short-note movement; tail bounds inverted. | Narrow handles, exact duration, bounded-tail tests. | T10 |
| ERR-028 | Medium | Native editing | Fixed | Delete after replacement removed a new note; double-click add lost selection. | Replacement-aware selection and real gestures. | T10 |
| ERR-029 | Medium | Web editing | Fixed | Keyboard resize exceeded endpoint; float maxima rejected valid tails. | Millibeat bounds/relocation and browser regressions. | T10/T13 |
| ERR-030 | Medium | Focus/input | Fixed | Empty layer/pending Generate lost focus; modifiers could edit notes. | Focus fallback, guarded aria-disabled and web/native modifier regressions. | T10/T13 |
| ERR-031 | Medium | Web preview | Fixed | Old preview timeout interfered with a newer preview. | Isolated timer browser regression. | T13 |
| ERR-032 | High | Cache concurrency | Fixed | Process-owned IPC locks failed to serialize cache quotas across threads. | In-process try-lock plus IPC; repeated concurrent quota tests. | T12/T13 |
| ERR-033 | Medium | Drag paths | Fixed | Linux percent/hash/space/Unicode paths did not round-trip through file URIs. | Escaped platform URL adapter and MidiDragSourceTest. | T12 |
| ERR-034 | Medium | Time conversion | Fixed | Long piecewise-tempo round trips failed at endpoint. | Cumulative elapsed time;4080point regression. | E02 |
| ERR-035 | Medium | Music control | Fixed | Modal interchange silently ignored harmonicBars4. | Borrowing at1/2/4; impossible single-span resolution rejects. | E02 |
| ERR-036 | Medium | Music control | Fixed | Explicit ghosts ignored at complexity0–24. | Actual ghosts across styles/feels and switch-off absence. | E02 |
| ERR-037 | Medium | Input validation | Fixed | Mood derivation overwrote an invalid base before validation. | Source validation first; MoodTest. | E03 |
| ERR-038 | Medium | Prompt parser | Fixed | Numeric-over-hint precedence retained superseded conflicts. | Directed provenance/precedence tests. | E03 |
| ERR-039 | Medium | Prompt parser | Fixed | Coordinated instrument exclusions became inclusions. | RU/EN negated lists/selected-track tests. | E03 |
| ERR-040 | Medium | Prompt parser | Fixed | Blend percentages reused/attached to the wrong genre. | Weight/order regressions; unsupported synthesis still rejects. | E03 |
| ERR-041 | Medium | Prompt parser | Fixed | Spelled accidental/spaced signs lost explicit key meaning. | Accidental/spaced-sign/equivalent-order tests. | E03 |
| ERR-042 | High | Resource/CPU | Fixed | Adversarial IDs caused quadratic uniqueness validation. | Balanced ID set; maximum-note decode RED→GREEN. | E04/E06 |
| ERR-043 | High | Playback boundary | Fixed | Exact seek rounding/epsilon skipped or admitted future events. | Exact/adjacent host tick123 and tick27/tempo100001 regressions. | E04 |
| ERR-044 | High | Playback ordering | Fixed | Final-off/CC/new-on ordering and endpoint CC delivery were wrong. | Ordered compact states and endpoint/next-block tests. | E04 |
| ERR-045 | High | RT performance | Fixed | addEvent repeatedly rescanned ordered prefixes. | Guarded ordered append, byte equivalence; local608us→7.44us, not a portable deadline. | E04/E06 |
| ERR-046 | High | Build guard | Fixed | Incremental JUCE edits bypassed configure-only layout verification. | Configure dependencies and real header/implementation drift regressions. | E04/E06 |
| ERR-047 | High | Stale worker | Fixed | Old generation could replace newer edits/restores. | Revision/job/mode guard; held-response test fails without it. | E04 |
| ERR-048 | High | Cold playback | Fixed | Unconditional mode drain skipped a complete short initial note. | Empty cleanup enters immediately;64sample cold-note test. | E04 |
| ERR-049 | Low | Test portability | Fixed | Test main stack frame exceeded1MiB. | Heap test owners and actual Linux1MiB-stack CTest; not Windows execution. | E06 |
| ERR-050 | Low | Diagnostics | Fixed | Composer overflow promised only a one-block pause. | Stop/start-or-seek wording and actual persistent-pause recovery. | E04 |
| ERR-051 | Medium | Overview | Fixed | Whole-note aggregation missed final pixel column. | Ceil exclusive end; full/half-note pixel tests. | E04 |
| ERR-052 | Medium | Overview UI | Fixed | Initial scrollbar clipped content; disabled legacy controls consumed view. | Height-first sizing/compact toolbar/screenshots. | E04 |
| ERR-053 | Critical | Save/data loss | Fixed | Partial save replaced a valid JSON/MIDI file and reported success. | 8febeea staged write/flush/status/length; isolated resource limits preserve original files. | E04/T13 |
| ERR-054 | High | Overview refresh | Fixed | Same-mode1→16track replacement hid lower tracks/maps. | 8febeea; real editor1→16→1without window resize. | E04 |
| ERR-055 | Medium | Test harness | Fixed | Restarting terminal JUCE dispatch loop skipped later assertions. | Isolated reflow process; actual failure observed before fix. | E06 |

| ERR-056 | Low | Native UI | Fixed | New Composer settings label compressed the full provenance summary into unreadably narrow text; a non-ASCII separator rendered incorrectly. | Minimum-window snapshot exposed the issue; replaced with12px read-only scrollable settings text and ASCII separator. Real-editor test checks accessible committed seed/origin text; final screenshots rechecked. | E03/E04 |

| ERR-057 | Medium | Input validation | Fixed | Seed character/length filters silently changed actual pasted negative/decimal/long values before validation (e.g.-1 became1). | Independent probe committed seed1; actual insertion regression RED→GREEN after preserving full text and validating before queue; CTest34/34. | E03 |
| ERR-058 | Medium | Input validation | Fixed | Oversized direction paste could lose an unsupported suffix and generate the truncated prefix. | Independent4110byte paste lost suffix; actual ASCII/multibyte insertion regression RED→GREEN after preserving text for4096UTF8byte rejection; CTest34/34. | E03 |

## Latest verification

ERR-001/ERR-002: implementation5c5eafa; Linux VST3/Standalone/CTest32/32, Node42/42/sample equality, native protocol scripts and scoped ASan/UBSan/TSan passed. Independent gpt-6-astra/high review found no Critical/Important/Minor, reran the two focused tests and verified the55entry counts. Other Fixed rows retain their historical scope rather than inheriting a new full audit.

Native Composer controls: implementation33326e9/1859a19 and final review fixes325d70f; ERR-056/057/058 verified in the stated UI scope. Post-fix Linux build/CTest34/34, Node42/42/sample equality and scoped native ASan/UBSan/TSan plus UI ASan/UBSan passed. Independent reviewer reran34/34, reproduced ERR-057/058, then one test-first fix pass closed both; no second review. Current58entries:49Fixed/6Open/1Investigating/2Blocked. At325d70f GitHub runs43736828619/43736828671 again have8jobs with no assigned runner or steps; ERR-005/006 remain blocked.

## Updating the register

Add reproduced defects and observed failures here during the same work. Retain IDs, severity, evidence and owning task when updating statuses. Mark Fixed only after a meaningful regression passes; state platform/tool boundaries. New actionable work belongs in task.md. A local green suite does not turn an external Investigating/Blocked item into Fixed.
