# Prototype composition/provider contract

This documents the current Node.js contract; no native implementation is implied.

`generate(params)` in `src/operations.js` returns a promise of `{ideas, sources, provider, model}`. Supported provider names: offline, openai, anthropic, openrouter, ollama, codex-cli, claude-cli. Unsupported names reject. Request options are normalized to key C..B (including sharps), minor/major/dorian/phrygian, one of the existing styles, BPM 40..240 and 4 or 8 bars in 4/4. Count is bounded to 1..8. Integer seed controls offline determinism. Description is limited to 600 characters in the provider prompt; model names use a restricted character set.

An idea has `metadata` and `layers`. Metadata contains key, mode, style, bpm, bars and unsigned seed. Layers contain bass, chords and melody arrays. Each note contains numeric pitch (0..127), start (beats from zero, less than bars*4), positive duration, and velocity (1..127). The validator rounds pitch/velocity, normalizes timing to thousandths of a beat and clips ends to the pattern. At most 800 notes per layer and 1600 in total are accepted. At least one note is required. Empty individual layers are allowed. Validated notes include their layer name.

Example provider content:

```json
{"ideas":[{"layers":{"bass":[{"pitch":36,"start":0,"duration":1,"velocity":100}],"chords":[],"melody":[]}}]}
```

`normalizeResponse` also accepts a single idea, and can extract a JSON object from surrounding text or a Markdown fence. Malformed content rejects the request. A partial valid response is supplemented with offline ideas, labelled `offline-fallback`; successful model ideas are labelled `ai` (or the CLI provider name). Missing or invalid model output does not silently become a successful AI result.

HTTP adapters use fixed endpoints and server-side credentials. Their fetch deadline is 90 seconds including body consumption. Successful response bodies are limited to 4 MiB of decoded bytes before JSON parsing; oversized streams are cancelled. Upstream failure bodies and transport exception details are not returned to clients. Cloud credentials are not required for offline generation or unit tests.

Transport tests replace global fetch with WHATWG Response objects; no live provider calls are required. The internal requestAI options accept an AbortSignal and an injectable transport function. HTTP disconnects cancel cloud HTTP generation; CLI process cancellation and automatic retry/backoff remain future work. This contract describes coercion and normalization in the prototype, not a strict JSON Schema rejecting every extra field.
