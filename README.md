# LoopForge AI MIDI Composer

Independent music-composition prototype inspired by the **workflow category** of SPAWN 2.0. No SPAWN code, sounds, branding, trained weights, or license checks are used.

## Native prototype

A JUCE/C++20 VST3 and Standalone prototype is available in [plugin/](plugin/README.md). It includes MIDI playback, a basic audition synth, native offline generation without a service, optional API-backed generation, model selection, connection checks/cancellation, variation selection, a piano-roll display and MIDI export. Linux compilation and native/API tests were performed; Windows and DAW compatibility remain unverified. Composer mode also exports its captured project as multi-track MIDI with tempo, meter, key and controller maps through a bounded native worker. Composer note editing/quantize/Undo/Redo are available through Edit Notes; basic Composer audition is available; scoped regeneration, seamless standalone looping and Composer MIDI drag remain pending. The browser prototype below remains available.

## Working features

- 8 offline, deterministic but varied musical ideas; 4 or 8 bars; 5 styles, 4 scales, 12 root notes, BPM 40-240.
- Three MIDI layers: bass, chords, melody. Browser piano roll: move notes, add with double click, delete with Alt+click, transpose by octaves.
- Built-in Web Audio synth preview (basic oscillator timbres, not sampled instruments).
- Standard MIDI type 1 export with separate bass/chords/melody tracks and tempo metadata.
- AI provider adapters: **OpenAI, Anthropic, OpenRouter, Ollama**, plus optional account-backed **Codex CLI** and **Claude Code CLI** bridges. Text model returns structured notes; deterministic local rules validate them and can supplement missing variants. No claim that an LLM is a purpose-built music diffusion model.
- Local MCP stdio server: tools `loopforge_generate`, `loopforge_export_midi`, `loopforge_providers`.
- CLI-friendly and can be connected by Claude Desktop, Claude Code, Codex and other compatible local MCP hosts (host configuration required). ChatGPT web requires a separately hosted remote MCP integration with suitable authorization; local stdio is not directly reachable from the ChatGPT website.

## Run on Windows/macOS/Linux

Requires Node.js 20+; no `npm install` or external dependencies needed. On Windows, you can double-click **`start-windows.bat`** after installing Node.js.

```bash
npm start
```

Open **http://127.0.0.1:4177**. The local interface starts with 8 already generated demo ideas. Click **Generate 8 Ideas** to make new ones. Export the result as MIDI, then import into a DAW.

For an offline-only session: no keys, accounts or internet needed. The font falls back to system fonts without internet.

## AI connections

Set environment variables in the same terminal where you launch LoopForge. Do NOT paste API keys into chat or source files.

**PowerShell (Windows):**

```powershell
$env:OPENAI_API_KEY="YOUR_KEY"
npm start
```

**macOS/Linux:**

```bash
export OPENAI_API_KEY="YOUR_KEY"
npm start
```

Equivalent variables: `ANTHROPIC_API_KEY`, `OPENROUTER_API_KEY`. Ollama uses `http://127.0.0.1:11434/v1/chat/completions` and requires Ollama and a locally available chat model. Change the **Model** field in the UI to choose your own compatible model; provider defaults are starting suggestions, not guarantees of model access. Keys are only on your own local server process; the browser never receives them.

**Important:** a ChatGPT subscription is not a general-purpose OpenAI API credential. Certain officially supported apps allow Sign in with ChatGPT and use of a plan. This independent prototype does not implement the partner-only sign-in flow. The supported alternatives are direct API keys, local Ollama, or an authenticated AI client invoking the MCP tools.

## Use your signed-in OpenAI or Claude account (without manually sharing API keys)

Install and authenticate the official CLI on the **same computer**:

```bash
codex login
# or
claude
```

Check with `codex login status` (OpenAI). Then start LoopForge and choose **OpenAI account / Codex CLI** or **Claude account / Claude Code CLI** in the **Engine** dropdown. The server launches the installed CLI in a restricted/plan mode, sends a music composition prompt via stdin, receives structured JSON, validates the resulting note events, and imports it to the piano roll. The existing CLI retains control of your sign-in; LoopForge never reads its stored OAuth tokens.

This is a local invocation of official coding-agent CLIs, not the same as implementing a commercial **Sign in with ChatGPT** integration. Whether a model request is permitted, how allowances are counted, and which models are available depend on the provider and your plan/CLI configuration. **The CLI integration has not been live-tested with signed-in accounts** in this environment.

## MCP mode (Claude Desktop / Codex / Claude Code)

Run `npm run mcp`, or configure the host to start the process for you. Example `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "loopforge": {
      "command": "node",
      "args": ["C:/ABSOLUTE/PATH/LoopForge-Prototype/src/mcp-server.js"]
    }
  }
}
```

For Codex: `codex mcp add loopforge -- node C:/ABSOLUTE/PATH/LoopForge-Prototype/src/mcp-server.js` (check your installed Codex CLI syntax). MCP does not transfer ChatGPT or Claude API credentials to the plugin. Instead the host's own model can prepare notes and call the export tool, or ask the plugin's configured provider to generate.

Example request in your AI client: "Use loopforge_generate to create eight 8-bar D minor nu metal ideas at 105 BPM; save the third using loopforge_export_midi."

The MCP server writes only to its project's `exports/` folder. It never evaluates arbitrary scripts, runs commands, or uploads files.

## REAPER

Drag exported `.mid` into REAPER or run `integrations/reaper/LoopForge_Import_MIDI.lua` from ReaScript. Other VST-capable DAWs can import standard MIDI.

## Run tests

```bash
npm test
npm run sample
```

Output demo: `samples/loopforge-demo.mid`.

## Browser prototype limitations (v0.1)

- The browser prototype is not a DAW-loaded instrument. The new native VST3/Standalone targets and verified Linux build are described in plugin/README.md; AU is not implemented.
- No waveform rendering, drag-and-drop DAW integration or sample library.
- No full automatic DAW-session editing except separate optional import script for REAPER.
- AI provider connectivity has been unit-tested with a mocked API response, but live paid endpoints have **not** been exercised (no credentials supplied).
- The standalone browser preview uses simple oscillator timbres and is not production-quality synthesis.

Next native milestones: actual DAW validation, editable piano roll, drag-and-drop export and improved instruments.

## Security

Local server binds only to 127.0.0.1. No browser access to secret credentials. AI generation sends your prompt and musical parameters to the selected provider; offline mode sends nothing. No credentials are logged. Never expose the local service directly to the internet.

## Originality

This is a clean-room implementation using public feature descriptions as functional requirements. All code in this prototype is original; no proprietary assets or model weights are included.
## UI controls and verification

Browser/native keyboard controls, responsive UI behaviour, installed Modern Web Guidance provenance and reproducible browser tests are documented in [docs/UI-REFINEMENT.md](docs/UI-REFINEMENT.md). The browser verification tools are isolated from the dependency-free app runtime.

Native Composer now accepts local RU/EN direction text plus an unsigned32bit fallback seed. Use **COMPOSE LOOP** from Legacy, then **COMPOSE OFFLINE LOOP** for another result; **BACK TO LEGACY / VIEW COMPOSER** switches between both preserved source documents. An explicit text seed overrides the fallback. The scrollable committed-settings view explains text/default origins and diagnostics. Generation runs on the native worker without Node, network or a model; cancellation and newer edits/restores/mode changes prevent an old result from replacing the project.

This integration executes4/8/16bar loops with selected bass/chords/lead/drums and existing supported musical parameters. Unsupported song/seconds/section arrangements, blended synthesis and extra roles reject without truncating the request or changing the committed project. Composer provides multi-track MIDI export and Edit Notes for bounded loops, including quantize and Undo/Redo. Seven mood axes offer Auto/manual pins. Host/project-v3 persists these next-request controls alongside committed notes/maps/seed and the inactive source; old v1/v2 documents remain readable. SOUND enables a bounded basic sine/percussion preview without changing MIDI output. Scoped musical regeneration, seamless looping and the remaining manual controls are tracked in task.md. Existing Legacy controls affect Legacy generation only.

## Current release target

The product release is a native offline loop tool, capped at16new bars; full songs/section arrangements are outside this release. Composer generates4/8/16bars; existing Legacy/browser generation supports4/8. Old longer documents remain loadable/exportable. Project-v3 stores exposed next-request recipe controls while preserving old v1/v2 loading. Current acceptance and remaining Composer regeneration/loop/DAW/package gates are tracked only in [task.md](task.md). See [packaging](docs/NATIVE-PACKAGING.md) for building an engineering candidate; this does not claim a finished release.

## First FL Studio test candidate

See [FL Studio installation and test guide](docs/FL-STUDIO-TEST.md) and [alpha release notes](docs/releases/0.2.0-alpha.1.md). The Native plugin workflow packages tested Windows/Linux binaries with SHA256 and dependency notices. Current Windows/FL acceptance and public binary delivery remain blocked as recorded in [task.md](task.md); a Linux candidate is not a Windows release.

The combined native binary distribution uses GNU AGPLv3 with preserved original MIT notices and the SDK GPLv3 option. Full terms: [native distribution](licenses/NATIVE-DISTRIBUTION.md). Candidate packages include corresponding committed LoopForge/JUCE sources and license texts.
