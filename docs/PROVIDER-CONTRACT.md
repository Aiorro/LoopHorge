# Prototype composition/provider contract

This documents the Node.js provider boundary used by the browser, MCP and optional native AI bridge. The native offline engine runs independently.

`generate(params, {signal})` in `src/operations.js` returns a promise of `{ideas, sources, provider, model}`. Supported provider names: offline, openai, anthropic, openrouter, ollama, codex-cli, claude-cli. Unsupported names reject. Request options are normalized to key C..B (including sharps), minor/major/dorian/phrygian, one of the existing styles, BPM 40..240 and 4 or 8 bars in 4/4. Count is bounded to 1..8. Integer seed controls offline determinism. Description is limited to 600 characters in the provider prompt; model names use a restricted character set.

An idea has `metadata` and `layers`. Metadata contains key, mode, style, bpm, bars and unsigned seed. Layers contain bass, chords and melody arrays. Each note contains numeric pitch (0..127), start (beats from zero, less than bars*4), positive duration, and velocity (1..127). The validator rounds pitch/velocity, normalizes timing to thousandths of a beat and clips ends to the pattern. At most 800 notes per layer and 1600 in total are accepted. At least one note is required. Empty individual layers are allowed. Validated notes include their layer name.

Example provider content:

```json
{"ideas":[{"layers":{"bass":[{"pitch":36,"start":0,"duration":1,"velocity":100}],"chords":[],"melody":[]}}]}
```

`normalizeResponse` also accepts a single idea, and can extract a JSON object from surrounding text or a Markdown fence. Malformed content rejects the request. A partial valid response is supplemented with offline ideas, labelled `offline-fallback`; successful model ideas are labelled `ai` (or the CLI provider name). Missing or invalid model output does not silently become a successful AI result.

HTTP adapters use fixed endpoints and server-side credentials. Their fetch deadline is 90 seconds including body consumption. Successful response bodies are limited to 4 MiB of decoded bytes before JSON parsing; oversized streams are cancelled. Upstream failure bodies and transport exception details are not returned to clients. Cloud credentials are not required for offline generation or unit tests.

Transport tests replace global fetch with WHATWG Response objects; no live provider calls are required. The internal requestAI options accept an AbortSignal and an injectable transport function. HTTP disconnects cancel HTTP and CLI generation. CLI adapters accept the validated model ID, bound execution to 120 seconds and stdout to 5 MB, and terminate normal descendants on cancellation, timeout, overflow and root exit. POSIX uses a process group; Windows 10+ uses an atomic job-list assignment and kill-on-close Job Object launched with built-in PowerShell/.NET. Windows runtime validation remains pending; deliberate POSIX setsid escapes are outside this guarantee. Automatic retry/backoff is not implemented. This contract describes coercion and normalization in the prototype, not a strict JSON Schema rejecting every extra field.

`POST /api/probe` uses the same loopback/Origin/JSON boundaries as generation, forces count=1 and returns `{ok, provider, model, message}` after one validated generation. It does not return or install the idea. UI initiation is explicit because this may incur provider charges. Errors remain `{error}` with sanitized HTTP status guidance (authorization/model access, quota/rate limits, unavailable model or connection). Model IDs are limited to 100 ASCII letters/digits/underscore, dash, dot, slash and colon; Blank IDs use the provider default; invalid explicit IDs reject instead of selecting a different model silently.
