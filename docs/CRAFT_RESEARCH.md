# Craft Anything workflow: Study → Build → Check → Refine

This dossier applies the **public-data, clean-room** investigative workflow described by `storytold/craft-anything`.
We did **not** run Craft Anything's proprietary-target binary inspection or call its executable, because the tool is not installed in this environment and no licensed SPAWN build was supplied. Public descriptions establish behaviors, not private architecture.

## STUDY / evidence

- https://saucewareaudio.com/products/spawn : 8 ideas, genre/mood/key workflow, MIDI note editing and independent bass/chord/melody export, sound/plugin replacement.
- https://sauceware-audio.myshopify.com/pages/spawn-help : cloud AI generation, offline regular instrument mode, plugin scan, editable MIDI, no source-code disclosures.
- https://github.com/storytold/craft-anything : use visual behavior and public documentation in clean-room mode; compare observed results; require approval for inspecting proprietary files.
- https://developers.openai.com/api/docs/guides/api-keys : API key security.
- https://developers.openai.com/api/docs/guides/tools-connectors-mcp : OpenAI models can call MCP tools with explicit authentication.
- https://help.openai.com/en/articles/20001542-using-your-chatgpt-plan-in-other-apps-and-sites : participating apps can request official plan sharing, not arbitrary app access.

## Known / unknown

Known: cloud MIDI generation; selected controls and three functional layers; editable MIDI; swap instruments; export to DAW.
Unknown: model architecture, training data composition, inference parameters, tokenization, audio render engine internals, exact 2.0 UX implementation, internal effect algorithms, automation and preset data schemas.

We deliberately do not claim access to unreleased model weights, source code, binaries, or internal API endpoints.

## BUILD / independent system

Offline rule-based music layer in `src/core.js`; AI adapters and a strict MIDI validator in `src/ai.js` / `src/schema.js`; local HTTP and MCP bridges in `src/server.js` / `src/mcp-server.js`; UI and native multitrack MIDI export.

## CHECK / test ledger

`npm test`: validates reproducibility, MIDI track metadata, musical note boundaries, failure paths, MCP request handling, AI adapter payloads with mocks, export to filesystem.
`python -mido validation` (optional): verifies standard MIDI file is parsable and tempo/track count matches expectations.
Browser UI: inspect local screenshot, verify no JavaScript runtime errors, button-level smoke.

## REFINE / target gaps

Build DAW-native JUCE VST3, sound engine, plugin hosting, resizable embedded piano roll, waveform samples, musical constraints, AU support, model-provider UI for sign-in where officially supported.
