# Architecture

```text
Desktop/browser UI ──localhost HTTP──> LoopForge server ──┬── Offline music rules
                        (no keys in browser)               └── Provider adapters
                                                           ├── OpenAI
                                                           ├── Anthropic
                                                           ├── OpenRouter
                                                           └── Ollama (local)
                             └── Signed-in agent CLI bridge (Codex / Claude Code)

Claude / Codex / ChatGPT MCP host ─stdio MCP─> MCP server ───> generate/export operations
                                                             └── Standard MIDI 1 files
                                                                              └── DAW import
```

Native VST3 will be a JUCE client to the local bridge, with host-synchronized MIDI playback running on the plugin audio thread and model inference running on a background worker. Never run HTTP requests or block on LLM output inside `processBlock`.

## AI response contract

Single idea:

```json
{"metadata":{"key":"D","mode":"minor","style":"nu-metal","bpm":105,"bars":8},"layers":{"bass":[{"pitch":38,"start":0,"duration":0.75,"velocity":110}],"chords":[{"pitch":50,"start":0,"duration":3,"velocity":87}],"melody":[{"pitch":74,"start":0.5,"duration":0.5,"velocity":94}]}}
```

Eight: `{"ideas":[idea, ...]}`. Times in beats, not seconds. Server normalizes and checks all fields, clamps overlapping end positions to the pattern length, and refuses invalid notes. It does not promise every LLM-generated melody will be original, musical, or legally free of similarity to published compositions.

## Provider isolation

Only fixed trusted provider endpoints are used; the model name is configurable. No arbitrary remote URL accepted from a browser/MCP call, preventing generic proxying and most local SSRF vectors. Secret tokens are read from environment variables only.

## MVP vs production

The current iteration is a testable music composition MVP rather than a shipping professional VST. The complete audio-host integration, sandbox policy for plugin loading, instrument presets, and potential cloud authorization are separate milestones.
