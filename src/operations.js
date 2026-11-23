'use strict';
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const {STYLES, generateEight, createMidi} = require('./core');
const {isLegacyMeter, normalizeOptions, validateIdea, isValidModelName} = require('./schema');
const {requestAI, configuration: apiConfiguration} = require('./ai');
const {generateUsingAccountCLI, available} = require('./agent-cli');

const GENERATION_FIELDS = new Set([
  'provider', 'key', 'mode', 'style', 'bpm', 'bars', 'model', 'count',
  'seed', 'description', 'action', 'timeSignature'
]);
const MAX_COUNT = 8;
const BASENAME_PATTERN = /^[a-zA-Z0-9_-]{1,45}$/;

async function generate(params = {}, {signal} = {}) {
  if (params && typeof params === 'object' && Object.keys(params).some(name => !GENERATION_FIELDS.has(name))) {
    throw new Error('Unsupported generation fields; specify loop bars explicitly');
  }
  if (params?.style !== undefined && (typeof params.style !== 'string' || !Object.hasOwn(STYLES, params.style))) {
    throw new Error('Unsupported style. Choose one of: ' + Object.keys(STYLES).join(', '));
  }
  const opts = normalizeOptions(params);
  if (params.timeSignature !== undefined && !isLegacyMeter(params.timeSignature)) {
    throw new Error('Legacy generation supports only a 4/4 time signature');
  }
  const model = params.model === '' ? undefined : params.model;
  if (model !== undefined && !isValidModelName(model)) throw new Error('Invalid model name');
  const provider = String(params.provider || 'offline');
  const count = Math.max(1, Math.min(MAX_COUNT, Math.round(Number(params.count) || 8)));

  if (provider === 'codex-cli' || provider === 'claude-cli') {
    return await generateUsingAccountCLI(provider, opts, {count, description: params.description, signal, model});
  }
  if (provider === 'offline') {
    const patterns = generateEight(opts, Number.isInteger(params.seed) ? params.seed : undefined).slice(0, count);
    return {ideas: patterns, sources: patterns.map(() => 'offline'), provider: 'offline', model: 'rules-v1'};
  }
  return await requestAI(provider, opts, {count, model, description: params.description, signal});
}

function exportsDir() {
  return path.resolve(__dirname, '../exports');
}

function safeBasename(basename) {
  if (typeof basename === 'string' && BASENAME_PATTERN.test(basename)) return basename;
  return `loopforge-${Date.now()}-${crypto.randomBytes(3).toString('hex')}`;
}

// Use a staging file in the target directory and rename after a complete write.
function atomicWrite(directory, basename, bytes) {
  const output = path.join(directory, basename + '.mid');
  const staging = path.join(directory, `.${basename}-${crypto.randomBytes(12).toString('hex')}.tmp`);
  let fd;
  let stagingOwned = false;
  try {
    fd = fs.openSync(staging, 'wx', 0o600);
    stagingOwned = true;
    fs.writeFileSync(fd, bytes);
    if (fs.fstatSync(fd).size !== bytes.length) throw new Error('Incomplete MIDI write');
    fs.fsyncSync(fd);
    const closing = fd;
    fd = undefined;
    fs.closeSync(closing);
    fs.renameSync(staging, output);
    stagingOwned = false;
  } finally {
    if (fd !== undefined) try { fs.closeSync(fd); } catch {}
    if (stagingOwned) try { fs.unlinkSync(staging); } catch {}
  }
  return output;
}

function exportIdea(raw, basename) {
  const opts = normalizeOptions(raw?.metadata || {});
  const idea = validateIdea(raw, opts);
  const filename = safeBasename(basename);
  const directory = exportsDir();
  fs.mkdirSync(directory, {recursive: true});
  const bytes = Buffer.from(createMidi(idea));
  const output = atomicWrite(directory, filename, bytes);
  return {path: output, filename: filename + '.mid', bytes: bytes.length};
}

async function configuration() {
  const [codex, claude] = await Promise.all([available('codex-cli'), available('claude-cli')]);
  return {...apiConfiguration(),
    'codex-cli': {ready: codex, defaultModel: 'ChatGPT signed-in CLI'},
    'claude-cli': {ready: claude, defaultModel: 'Claude signed-in CLI'}};
}

module.exports = {generate, exportIdea, configuration, exportsDir};
