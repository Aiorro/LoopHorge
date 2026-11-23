'use strict';
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const {generate, configuration} = require('./operations');
const {generateV1, failure, BridgeError} = require('./bridge');
const {createMidi} = require('./core');
const {normalizeOptions, validateIdea} = require('./schema');

const HOST = '127.0.0.1';
const PORT = Number(process.env.LOOPFORGE_PORT || 4177);
const ROOT = path.resolve(__dirname, '../web');
const MAX_BODY_BYTES = 512 * 1024;
const MIME = {'.html': 'text/html; charset=utf-8', '.css': 'text/css; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8', '.svg': 'image/svg+xml'};
// The browser UI may load the engine copy directly from src/.
const STATIC_FILES = new Set([
  path.join(ROOT, 'index.html'),
  path.join(ROOT, 'style.css'),
  path.join(ROOT, 'app.js'),
  path.resolve(__dirname, 'core.js')
]);
const BRIDGE_ENDPOINTS = new Set(['/api/v1/generate', '/api/v1/probe']);
const BRIDGE_VERSION_PREFIX = '/api/v1/';

function localHosts() {
  return ['127.0.0.1:' + PORT, 'localhost:' + PORT];
}

function json(res, status, body) {
  const text = JSON.stringify(body);
  res.writeHead(status, {'Content-Type': 'application/json; charset=utf-8',
    'Cache-Control': 'no-store', 'Content-Length': Buffer.byteLength(text)});
  res.end(text);
}

function readJSON(req) {
  return new Promise((resolve, reject) => {
    const body = [];
    let size = 0, oversized = false;
    req.on('data', chunk => {
      if (oversized) return;
      size += chunk.length;
      if (size > MAX_BODY_BYTES) {
        oversized = true;
        body.length = 0;
        const error = new Error('Request too large');
        error.status = 413;
        reject(error);
        return;
      }
      body.push(chunk);
    });
    req.on('end', () => {
      if (oversized) return;
      try { resolve(JSON.parse(size ? Buffer.concat(body, size).toString('utf8') : '{}')); }
      catch { reject(new Error('Invalid JSON')); }
    });
    req.on('error', reject);
  });
}

// Abort in-flight generation as soon as the client drops the connection.
function requestAbortSignal(res) {
  const controller = new AbortController();
  res.on('close', () => controller.abort());
  return controller.signal;
}

// Bridge protocol: error shapes are structured objects with a code.
async function handleBridge(req, res, url) {
  if (!url.startsWith(BRIDGE_VERSION_PREFIX)) {
    throw new BridgeError('UNSUPPORTED_VERSION', 'Supported bridge protocol version is 1', 426);
  }
  if (!BRIDGE_ENDPOINTS.has(url)) throw new BridgeError('NOT_FOUND', 'Unknown bridge endpoint', 404);
  if (req.method !== 'POST') throw new BridgeError('METHOD_NOT_ALLOWED', 'POST required', 405);
  const args = await readJSON(req);
  const result = await generateV1(args, {probe: url === '/api/v1/probe', signal: requestAbortSignal(res)});
  if (!res.destroyed) json(res, 200, result);
}

async function handleLegacyApi(req, res, url) {
  if (req.method === 'GET' && url === '/api/status') {
    json(res, 200, {ok: true, providers: await configuration(), mcp: 'node src/mcp-server.js'});
    return true;
  }
  if (req.method === 'POST' && (url === '/api/generate' || url === '/api/probe')) {
    const args = await readJSON(req);
    const probe = url === '/api/probe';
    const result = await generate(probe ? {...args, count: 1} : args, {signal: requestAbortSignal(res)});
    json(res, 200, probe
      ? {ok: true, provider: result.provider, model: result.model,
         message: 'Provider generated one validated idea successfully'}
      : result);
    return true;
  }
  if (req.method === 'POST' && url === '/api/export') {
    const args = await readJSON(req);
    const opts = normalizeOptions(args.idea?.metadata || {});
    const normalized = validateIdea(args.idea, opts);
    const bytes = createMidi(normalized, {split: args.split !== false});
    res.writeHead(200, {'Content-Type': 'audio/midi',
      'Content-Disposition': 'attachment; filename="loopforge.mid"', 'Cache-Control': 'no-store'});
    res.end(Buffer.from(bytes));
    return true;
  }
  return false;
}

function serveStatic(req, res) {
  if (req.method !== 'GET') return json(res, 405, {error: 'Method not allowed'});
  const requested = new URL(req.url, 'http://localhost').pathname;
  const file = path.resolve(ROOT, '.' + (requested === '/' ? '/index.html' : requested));
  if (file !== ROOT && !file.startsWith(ROOT + path.sep)) return json(res, 403, {error: 'Forbidden'});
  const final = requested === '/core.js' ? path.resolve(__dirname, 'core.js') : file;
  if (!STATIC_FILES.has(final) || !fs.existsSync(final)) return json(res, 404, {error: 'Not found'});
  res.writeHead(200, {'Content-Type': MIME[path.extname(final)] || 'application/octet-stream',
    'X-Content-Type-Options': 'nosniff'});
  const stream = fs.createReadStream(final);
  stream.on('error', () => res.destroy());
  res.on('close', () => stream.destroy());
  stream.pipe(res);
}

async function listener(req, res) {
  const url = req.url;
  const versioned = /^\/api\/v\d+(?:\/|$)/.test(url);
  const fail = (status, code, message) =>
    json(res, status, versioned ? {protocolVersion: 1, error: {code, message}} : {error: message});
  try {
    const host = String(req.headers.host || '');
    if (!localHosts().includes(host)) return fail(403, 'FORBIDDEN', 'Local host required');
    if (req.method === 'POST') {
      const origin = req.headers.origin;
      if (origin && !['http://127.0.0.1:' + PORT, 'http://localhost:' + PORT].includes(origin)) {
        return fail(403, 'FORBIDDEN', 'Cross-origin requests are forbidden');
      }
      if (!String(req.headers['content-type'] || '').startsWith('application/json')) {
        return fail(415, 'UNSUPPORTED_MEDIA_TYPE', 'JSON Content-Type required');
      }
    }
    if (versioned) return await handleBridge(req, res, url);
    if (await handleLegacyApi(req, res, url)) return;
    return serveStatic(req, res);
  } catch (err) {
    if (res.headersSent || res.destroyed) return;
    if (versioned) {
      const error = failure(err);
      return fail(error.status, error.code, error.message);
    }
    json(res, err.status || (err.message?.includes('API_KEY') ? 400 : 422), {error: err.message || 'Request failed'});
  }
}

if (require.main === module) {
  if (!Number.isInteger(PORT) || PORT < 1 || PORT > 65535) {
    console.error('LOOPFORGE_PORT must be an integer between 1 and 65535');
    process.exit(1);
  }
  http.createServer(listener).listen(PORT, HOST, () => console.log(`LoopForge → http://${HOST}:${PORT}`));
}

module.exports = {listener};
