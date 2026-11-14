# Project Envelope v2 Implementation Plan

> Execute inline with superpowers:executing-plans/TDD and one fresh review. Existing architecture approval and repeated continuation authorize this dependency slice. Task.md owns all progress checkboxes.

**Goal:** Provide a bounded, lossless project-v2 codec before activating Composer in Processor.

**Architecture:** ProjectState contains activeMode, optional original version1 state JSON, optional typed Composition and explicit host origin. Schema-directed parsing reuses CompositionJson; legacy JSON is a bounded string containing the complete original v1 envelope, validated with the unchanged wire::document/schedule contract. Codec returns a separate candidate, never mutates a live project. Processor/UI activation, worker revision policy and history remain the next integrated slice.

**Tech Stack:** C++20, existing JUCE core and native codecs; no new dependencies.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, Compatibility and state migration / host transport.

## Global Constraints

Version2 only; exact required envelope keys version, activeMode, hostStartQuarterBeat, legacyStateJson, composition. Modes legacy/composer; nullable payloads, selected mode requires its payload. Origin is a finite double JSON number with round-trip precision, independent of document ticks. Full encoded envelope<=32MiB; decoded legacy JSON<=4MiB and original bytes retained; composed validation accountedBytes plus retained legacy string capacity<=64MiB. No automatic v1 conversion or downgrade. Unknown/duplicate/missing envelope keys, invalid nested compositions and invalid inactive payloads reject with a bounded sanitized error and no candidate. Old runtime remains unchanged until coherent state/publication activation. Preserve task-engine payload, all original15 tasks and untracked samples.

## Review Focus

- Inactive malformed documents must reject, not become latent corrupt mode switches (Task1).
- Legacy beat decimals/exponents, unknown extensions, whitespace and selection metadata must survive byte-for-byte (Task1).
- Fractional/negative origins must retain exact double value; invalid numeric syntax/nonfinite/overflow/trailing input rejects (Task1).
- Whole-envelope size can exceed its component budgets due to escaped legacy text; writer and reader must enforce total32MiB (Task1).
- Failure returns no candidate and never partial serialized output or mutates its input; deep untrusted envelope nesting must reject without recursive traversal (Task1).

### Task 1: Validated project-v2 codec

**Files:** create plugin/Source/engine/ProjectJson.h, plugin/tests/ProjectJsonTest.cpp; modify plugin/CMakeLists.txt, plugin/README.md, task.md.

**Interfaces:** ProjectMode{legacy,composer}; ProjectState{ProjectMode activeMode=legacy; double hostStartQuarterBeat=0; optional<string> legacyStateJson; optional<Composition> composition}; ProjectReadResult{optional<ProjectState> project;string error}; encodeProjectJson(const ProjectState&)->CompositionJsonResult; decodeProjectJson(string_view)->ProjectReadResult. Returned project is a candidate only, not a restore controller. No new public JSON primitives required.

1. Write failing project_json CTest against real fixtures: composer-only, legacy-only and both modes; exact bytes of full old envelope including unknown extension/decimal/exponent/selection; fractional/negative origins; UINT64 IDs; byte-for-byte deterministic re-encoding. Reject absent selected payload, invalid inactive payload, invalid legacy version/schedule, invalid composition, unknown/missing/duplicate escaped envelope keys, deep nesting/trailing data and malformed numbers. Expected: build fails only for missing ProjectJson.h.
2. Implement schema-directed codec. Use bounded Reader::string for legacy and unchanged wire validators; preserve source string, not reserialized JUCE DOM. Parse strict JSON float grammar with from_chars, emit max_digits10; validate both payloads on read/write and full32MiB output. Tests additionally cover4MiB legacy and32MiB outer boundary, UTF8/escape expansion, invalid in-memory enum/nonfinite origin and failed write atomicity. Expected: focused CTest passes.
3. Build all native targets/CTest, run npm test/sample and compare preserved samples; ASan/UBSan test executable with linked JUCE scope disclosed. Inspect diff and invariants, record evidence and commit. Expected: full native/Node green, requirements unchanged. Fresh reviewer checks entire slice and connected interfaces; fix Important/Critical RED→GREEN once; record rulings and retain E02–E06 open. No claiming runtime activation or Windows/DAW validation.
