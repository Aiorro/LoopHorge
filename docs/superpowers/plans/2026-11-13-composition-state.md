# Composition-v2 JSON Persistence Implementation Plan

> Execute inline with superpowers:executing-plans/TDD and one fresh independent review. Approved architecture/continuation authorize this dependency. Task.md remains the sole progress checklist.

**Goal:** Save and restore all existing Composition fields exactly through bounded UTF-8 JSON, without changing legacy plugin state.

**Architecture:** A schema-directed streaming reader constructs typed values directly, rejects unknown/duplicate/missing fields and validates the whole candidate before returning it. A deterministic writer emits canonical JSON; all uint64 IDs use decimal strings. No generic DOM, dependencies or automatic legacy conversion.

**Tech Stack:** C++20 standard library, composition validator, existing CTest; independent Python JSON decoding for a generated artifact.

**Spec:** ../specs/2026-11-13-hybrid-engine-design.md, composition-v2 resource envelope and compatibility/state migration. This slice implements composition serialization only; plugin envelope/restore cancellation/publication require the later prepared playback integration.

## Global Constraints

32MiB serialized UTF-8 bytes; existing64MiB document/prepared and all collection/time/channel/density limits. Source IDs are uint64; encode as canonical positive decimal strings, never doubles. Numeric fields accept integer tokens only (no fractions/exponents/coercions). Text must be valid UTF-8 without NUL, preserving scalar spelling/escapes; reject invalid text instead of replacement. Field order independent on read; canonical order on write. Enums use explicit stable names, optional program/bank use null. Owned-pitch mask uses128 binary characters. Unknown versions/fields, duplicates, malformed JSON/Unicode and oversize fail with no partial result. Legacy parser remains4MiB; old state/bridge/export/UI untouched. Worker-only allocation; no file/network/process operations.

## Review Focus

- IDs above2^53 and UINT64_MAX must survive exactly; negative/overflow/leading-zero/number IDs reject (Task1).
- Duplicate escaped keys, unknown/missing fields, trailing data/commas, malformed scalars/deep nesting and wrong types must reject before candidate publication (Task1).
- Raw UTF-8 and escaped surrogate pairs roundtrip; unpaired surrogates, overlong encoding/NUL/control syntax reject without byte replacement (Task1).
- Total notes/CC across tracks, per-map/text/section/chord/track budgets and32MiB input must bound reads/allocations rather than silently truncating (Task1).
- Every existing model field, empty optional/collections, unordered notes/CC and legacy provenance retain exact values; legacy plugin state continues on its existing path (Task1).

### Task 1: Exact bounded composition JSON codec

**Files:** create plugin/Source/engine/JsonCodec.h (bounded lexical primitives), CompositionJson.h (schema read/write/API), plugin/tests/CompositionJsonTest.cpp. Modify plugin/CMakeLists.txt, plugin/README.md, task.md.

**Interfaces:** CompositionJsonResult{string json,error; explicit operator bool()const;}; CompositionReadResult{optional<Composition> composition; string error;}; encodeCompositionJson(const Composition&)->CompositionJsonResult; decodeCompositionJson(string_view)->CompositionReadResult. Failure returns empty JSON/no Composition and sanitized error; no caller document mutation. Decoder parses schemaVersion/ppq/engineVersion/seed/id/style/source/legacyJson/timingNotice/endTick plus tempos/meters/keys/sections/chords/tracks. Names match existing model except startTick/durationTicks, and section endTick. IDs are strings; role/scale/section role names are kebab-case. Track fields include id/name/role/channel/lowPitch/highPitch/program/bank/ownedPitches/notes/controls. All fields required, including explicit null optional presets and empty arrays/text.

1. Write tests from a hand-authored JSON fixture with maximal IDs, map changes, chord metadata, optional presets,128-bit ownership mask and Unicode/provenance; assert every decoded value and encoded roundtrip. Cover malformed/duplicate/unknown/missing fields, wrong number/ID syntax/types, invalid Unicode/UTF-8, order independence, invalid model output and v1 rejection. Run standard-only C++/CMake test first. Expected: missing new API before production code.
2. Implement strict bounded scalar/object/array reader and canonical writer. Reader has fixed schema traversal (no arbitrary recursive DOM), property bitmasks for exact fields/duplicates and global note/CC counters checked before insertion; never reserve from untrusted declared counts. Validate final candidate atomically. Writer validates model before emission, checks32MiB while appending and validates text scalars. Expected: directed fixtures pass with no silent repair.
3. Exercise131072notes/65536CC across tracks, escaped4MiB legacy provenance, max map/track/text limits, random malformed/truncated bytes and real generated/imported documents. Decode canonical generated artifact independently with Python json and compare exact high IDs/presets/events. Measure process peak RSS for largest decode/encode case, distinguishing live-process RSS from total256MiB whole-plugin acceptance. Run strict compiler, ASan/UBSan, full Linux VST3/Standalone build/CTest and Node/sample byte checks. Expected: all pass, original task definitions and requirements payload unchanged.
4. Document exact format/API/limits; record evidence and commit. Run task-done with full CTest. Obtain one fresh read-only review, reproduce Critical/Important findings before fixes, rerun relevant suites; record exclusions and push draft PR3. Do not claim complete E04/plugin persistence/UI integration.
