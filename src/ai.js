'use strict';
const {normalizeResponse, isValidModelName} = require('./schema');
const {generateEight} = require('./core');

const ENDPOINTS = {
  openai: {url: 'https://api.openai.com/v1/chat/completions', key: 'OPENAI_API_KEY', model: 'gpt-5.2'},
  anthropic: {url: 'https://api.anthropic.com/v1/messages', key: 'ANTHROPIC_API_KEY', model: 'claude-sonnet-4-5'},
  openrouter: {url: 'https://openrouter.ai/api/v1/chat/completions', key: 'OPENROUTER_API_KEY', model: 'openai/gpt-4.1-mini'},
  ollama: {url: 'http://127.0.0.1:11434/v1/chat/completions', key: null, model: 'qwen2.5:7b'}
};

const REQUEST_TIMEOUT_MS = 90000;
const MAX_RESPONSE_BYTES = 4 * 1024 * 1024;
const MAX_DESCRIPTION_LENGTH = 600;
const ANTHROPIC_API_VERSION = '2023-06-01';
const SYSTEM_PROMPT = 'You are a composer. Answer with valid JSON only.';
const DEFAULT_DIRECTION = 'Produce a distinctive, musical motif.';

// Provider readiness report for UI/status endpoints; never exposes keys.
function configuration() {
  return Object.fromEntries(Object.entries(ENDPOINTS).map(([name, info]) =>
    [name, {ready: !!(info.key && process.env[info.key]), local: name === 'ollama', defaultModel: info.model}]));
}

function prompt(opts, count, description) {
  const total = opts.bars * 4;
  return [
    'Compose original, coherent instrumental MIDI ideas for a music production plugin. Return ONLY valid JSON, no markdown.',
    `Requested: ${JSON.stringify(opts)}. Number of ideas: ${count}. User direction: ${String(description || DEFAULT_DIRECTION).slice(0, MAX_DESCRIPTION_LENGTH)}`,
    'Schema: {"ideas":[{"layers":{"bass":[{"pitch":36,"start":0,"duration":1,"velocity":100}],"chords":[{"pitch":48,"start":0,"duration":3,"velocity":85}],"melody":[{"pitch":72,"start":0.5,"duration":0.5,"velocity":100}]}}]}.',
    `Times are beat offsets, not seconds. There are ${total} beats. Output full ${count} ideas. Each should have meaningful chord progressions, bass aligned with chord roots and melodic motifs, with distinct rhythmic contours. Use MIDI pitches 0..127; starts >=0 and <${total}; duration>0; velocity 1..127. Use 4/4. Include all 3 layer arrays for every idea.`,
    'Keep note count modest (roughly 10-32 bass, 12-32 chord, 15-40 melody notes per idea); ensure key/scale agreement and voice-leading. No copyrighted melody quotations.'
  ].join('\n');
}

// Builds provider-specific headers and request body. Anthropic uses its own
// envelope; every other endpoint follows the OpenAI chat-completions shape.
function buildRequest(provider, actualModel, instruction, apiKey) {
  const headers = {'Content-Type': 'application/json'};
  if (provider === 'anthropic') {
    headers['x-api-key'] = apiKey;
    headers['anthropic-version'] = ANTHROPIC_API_VERSION;
    return {headers, body: {model: actualModel, max_tokens: 20000, temperature: 0.75,
      messages: [{role: 'user', content: instruction}]}};
  }
  if (apiKey) headers['Authorization'] = `Bearer ${apiKey}`;
  return {headers, body: {model: actualModel, temperature: 0.75,
    messages: [{role: 'system', content: SYSTEM_PROMPT}, {role: 'user', content: instruction}]}};
}

function httpStatusHint(status) {
  if (status === 401 || status === 403) return 'check authorization, API key and model access';
  if (status === 429) return 'rate limit or quota reached; retry later';
  if (status === 404) return 'check the model name';
  return 'check provider availability';
}

// Reads the response with a hard byte ceiling so a runaway provider cannot
// exhaust memory. Cancellation and timeout errors are re-labelled consistently.
async function readResponseBody(response, requestSignal, signal, provider) {
  const reader = response.body?.getReader();
  if (!reader) throw new Error('Provider sent empty response');
  const chunks = [];
  let size = 0;
  try {
    while (true) {
      const {done, value} = await reader.read();
      if (done) break;
      size += value.byteLength;
      if (size > MAX_RESPONSE_BYTES) {
        await reader.cancel();
        throw new Error('Provider response exceeded 4 MiB safety limit');
      }
      chunks.push(value);
    }
  } catch (error) {
    if (signal?.aborted) throw new Error('Generation cancelled');
    if (requestSignal.reason?.name === 'TimeoutError' || error?.name === 'TimeoutError') {
      throw new Error(`${provider} request timed out`);
    }
    if (error.message === 'Provider response exceeded 4 MiB safety limit') throw error;
    throw new Error('Provider response could not be read; check connectivity or timeout');
  } finally {
    reader.releaseLock();
  }
  return Buffer.concat(chunks).toString('utf8');
}

function extractProviderText(provider, json) {
  if (provider === 'anthropic') {
    if (!Array.isArray(json.content)) return undefined;
    return json.content.filter(c => c?.type === 'text' && typeof c.text === 'string').map(c => c.text).join('\n');
  }
  return json.choices?.[0]?.message?.content;
}

// When the model returned fewer ideas than requested, top up from the offline
// engine so callers always receive the full count.
function completeIdeas(content, opts, count, sourceLabel) {
  const ideas = normalizeResponse(content, opts, count);
  const sources = ideas.map(() => sourceLabel);
  if (ideas.length < count) {
    const missing = generateEight(opts).slice(0, count - ideas.length);
    ideas.push(...missing);
    sources.push(...missing.map(() => 'offline-fallback'));
  }
  return {ideas, sources};
}

async function requestAI(provider, opts, {count = 8, description = '', model, signal, transport = fetch} = {}) {
  if (signal?.aborted) throw new Error('Generation cancelled');
  if (!Object.hasOwn(ENDPOINTS, provider)) throw new Error('Unsupported provider');
  const config = ENDPOINTS[provider];
  const apiKey = config.key ? process.env[config.key] : '';
  if (config.key && !apiKey) throw new Error(`${config.key} environment variable is not set`);
  // An unusable model name silently falls back to the provider default.
  const actualModel = isValidModelName(model) ? model : config.model;
  const instruction = prompt(opts, count, description);
  const {headers, body} = buildRequest(provider, actualModel, instruction, apiKey);

  const requestSignal = signal
    ? AbortSignal.any([signal, AbortSignal.timeout(REQUEST_TIMEOUT_MS)])
    : AbortSignal.timeout(REQUEST_TIMEOUT_MS);
  let response;
  try {
    response = await transport(config.url, {method: 'POST', headers, body: JSON.stringify(body), signal: requestSignal});
  } catch (error) {
    if (signal?.aborted) throw new Error('Generation cancelled');
    if (requestSignal.reason?.name === 'TimeoutError' || error?.name === 'TimeoutError') {
      throw new Error(`${provider} request timed out`);
    }
    throw new Error(`${provider} connection failed; check connectivity and provider configuration`);
  }
  if (!response.ok) {
    try { await response.body?.cancel(); } catch {}
    throw new Error(`${provider} returned HTTP ${response.status}; ${httpStatusHint(response.status)}`);
  }

  const raw = await readResponseBody(response, requestSignal, signal, provider);
  let json;
  try { json = JSON.parse(raw); } catch { throw new Error('Provider sent invalid JSON response'); }
  if (!json || typeof json !== 'object' || Array.isArray(json)) throw new Error('Provider sent invalid JSON response');
  const content = extractProviderText(provider, json);
  if (typeof content !== 'string' || !content.trim()) throw new Error('Provider response contained no text');

  return {...completeIdeas(content, opts, count, 'ai'), model: actualModel, provider};
}

module.exports = {requestAI, configuration, ENDPOINTS, prompt, completeIdeas};
