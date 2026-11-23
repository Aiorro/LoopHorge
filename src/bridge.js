'use strict';
const schema = require('../schemas/bridge-v1.schema.json');
const {generate} = require('./operations');
const {isLegacyMeter, normalizeOptions, validateIdea} = require('./schema');

class BridgeError extends Error {
  constructor(code, message, status = 422) {
    super(message);
    this.code = code;
    this.status = status;
  }
}

// Validates a bridge request against the JSON schema subset used by the
// protocol: field presence, types, enums, bounds and the fixed 4/4 time
// signature. Unknown fields are rejected outright.
function validateRequest(params) {
  if (!params || typeof params !== 'object' || Array.isArray(params)) {
    throw new BridgeError('INVALID_REQUEST', 'Expected request object');
  }
  const fields = schema.$defs.request.properties;
  for (const [name, value] of Object.entries(params)) {
    if (!Object.hasOwn(fields, name)) throw new BridgeError('INVALID_REQUEST', 'Unknown request field');
    const rule = fields[name];
    let valid;
    if (name === 'timeSignature') {
      valid = isLegacyMeter(value);
    } else {
      valid = (rule.type === 'string' ? typeof value === 'string' : Number.isInteger(value)) &&
        (!rule.enum || rule.enum.includes(value)) &&
        (rule.minimum === undefined || value >= rule.minimum) &&
        (rule.maximum === undefined || value <= rule.maximum) &&
        (rule.maxLength === undefined || [...value].length <= rule.maxLength) &&
        (!rule.pattern || new RegExp(rule.pattern).test(value));
    }
    if (!valid) throw new BridgeError('INVALID_REQUEST', `Invalid ${name}`);
  }
  return params;
}

async function generateV1(params, {probe = false, signal} = {}) {
  validateRequest(params);
  if (signal?.aborted) throw new BridgeError('GENERATION_CANCELLED', 'Generation cancelled', 499);
  const result = await generate(probe ? {...params, count: 1} : params, {signal});
  const opts = normalizeOptions(params);
  const ideas = result.ideas.map(raw => {
    const idea = validateIdea(raw, opts);
    idea.metadata.timeSignature = {numerator: 4, denominator: 4};
    return idea;
  });
  if (probe) {
    return {protocolVersion: 1, ok: true, provider: result.provider, model: result.model,
      message: 'Provider generated one validated idea successfully'};
  }
  return {protocolVersion: 1, ...result, ideas};
}

function mapUpstreamHttpStatus(status) {
  if (status === 401 || status === 403) return new BridgeError('AUTHORIZATION_REQUIRED', 'Check provider authorization, API key and model access', 400);
  if (status === 429) return new BridgeError('RATE_LIMITED', 'Provider rate limit or quota reached; retry later', 429);
  if (status === 404) return new BridgeError('MODEL_UNAVAILABLE', 'Check provider model name and access', 422);
  return new BridgeError('PROVIDER_UNAVAILABLE', 'Provider rejected the request', 502);
}

// Maps lower-level errors (operations, ai, agent-cli, HTTP body reading) onto
// stable protocol error codes. Matching is by sanitized message text, so any
// message change in the layers below must be mirrored here.
function failure(error) {
  if (error instanceof BridgeError) return error;
  if (error.status === 413) return new BridgeError('REQUEST_TOO_LARGE', 'Request exceeded 512 KiB limit', 413);
  const message = String(error.message || '');
  if (message === 'Invalid JSON') return new BridgeError('INVALID_REQUEST', 'Invalid JSON');
  if (message === 'Generation cancelled') return new BridgeError('GENERATION_CANCELLED', 'Generation cancelled', 499);
  if (/^(OPENAI_API_KEY|ANTHROPIC_API_KEY|OPENROUTER_API_KEY) environment variable is not set$/.test(message)) {
    return new BridgeError('AUTHORIZATION_REQUIRED', message, 400);
  }
  const upstream = /^(openai|anthropic|openrouter|ollama) returned HTTP (\d{3})/.exec(message);
  if (upstream) return mapUpstreamHttpStatus(Number(upstream[2]));
  if (/(-cli timed out| request timed out)$/.test(message)) return new BridgeError('REQUEST_TIMEOUT', 'Provider request timed out', 504);
  if (/CLI not found|containment/.test(message)) return new BridgeError('PROVIDER_UNAVAILABLE', 'Check CLI installation, platform process containment and login', 503);
  if (/-cli exited with code/.test(message)) return new BridgeError('AUTHORIZATION_REQUIRED', 'Check CLI login and model access', 400);
  if (/connection failed/.test(message)) return new BridgeError('PROVIDER_UNAVAILABLE', 'Check provider connectivity and configuration', 502);
  if (/exceeded|JSON|response|notes|composition/.test(message)) return new BridgeError('INVALID_RESPONSE', 'Provider returned invalid or oversized composition data', 502);
  return new BridgeError('GENERATION_FAILED', 'Generation failed; check provider configuration', 502);
}

module.exports = {generateV1, validateRequest, failure, BridgeError};
