'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const crypto = require('node:crypto');
const { execFileSync, spawnSync } = require('node:child_process');

const cmake = process.env.CMAKE_COMMAND || 'cmake';
const haveCmake = spawnSync(cmake, ['--version']).status === 0;
const noticeFiles = [
 'LICENSE.md',
 'modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt',
 'modules/juce_audio_processors/format_types/VST3_SDK/base/LICENSE.txt',
 'modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/LICENSE.txt',
 'modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/LICENSE.txt',
 'modules/juce_core/zip/zlib/LICENSE',
 'modules/juce_core/zip/zlib/JUCE_CHANGES.txt',
 'modules/juce_graphics/image_formats/pnglib/LICENSE',
 'modules/juce_graphics/image_formats/jpglib/README',
 'modules/juce_graphics/fonts/harfbuzz/COPYING',
 'modules/juce_graphics/unicode/sheenbidi/LICENSE',
 'modules/juce_audio_formats/codecs/flac/Flac Licence.txt',
 'modules/juce_audio_formats/codecs/oggvorbis/Ogg Vorbis Licence.txt',
 'modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/COPYING'
];
function write(file, bytes) { fs.mkdirSync(path.dirname(file), { recursive: true }); fs.writeFileSync(file, bytes); }
function git(dir, ...args) { return execFileSync('git', ['-C', dir, ...args], { encoding: 'utf8' }).trim(); }
function commit(dir) {
 git(dir, 'init', '-q'); git(dir, 'add', '.');
 git(dir, '-c', 'user.name=Packaging fixture', '-c', 'user.email=fixture@example.invalid', 'commit', '-qm', 'fixture');
 return git(dir, 'rev-parse', 'HEAD');
}
// A minimal structurally valid PE fixture; never executed or represented as a real build.
function pe(dll = false) {
 const b = Buffer.alloc(1024); b.write('MZ'); b.writeUInt32LE(128, 60); b.write('PE\0\0', 128, 'binary');
 b.writeUInt16LE(0x8664, 132); b.writeUInt16LE(1, 134); b.writeUInt16LE(240, 148);
 b.writeUInt16LE(0x22 | (dll ? 0x2000 : 0), 150); b.writeUInt16LE(0x20b, 152);
 b.writeUInt32LE(512, 392 + 16); b.writeUInt32LE(512, 392 + 20); b.writeUInt32LE(0x60000020, 392 + 36);
 b[512] = 0xc3; return b;
}
function fixture(t) {
 const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'loopforge-package-test-'));
 t.after(() => fs.rmSync(temp, { recursive: true, force: true }));
 const rootDir = path.join(temp, 'repo'), juceDir = path.join(temp, 'juce'), buildDir = path.join(temp, 'build');
 for (const file of noticeFiles) write(path.join(juceDir, file), `Fixture notice: ${file}\n`);
 const juceSha = commit(juceDir);
 write(path.join(rootDir, 'plugin/CMakeLists.txt'), `project(LoopForge VERSION 0.2.0 LANGUAGES C CXX)\nFetchContent_Declare(JUCE GIT_TAG ${juceSha})\n`);
 for (const name of ['AGPL-3.0.txt','GPL-3.0.txt','NATIVE-DISTRIBUTION.md']) write(path.join(rootDir,'licenses',name),'Fixture license '+name);
 write(path.join(rootDir, 'LICENSE'), 'Fixture repository MIT notice\n');
 write(path.join(rootDir, 'docs/NATIVE-PACKAGING.md'), 'Fixture portable instructions\n');
 write(path.join(rootDir, 'docs/FL-STUDIO-TEST.md'), 'Fixture FL Studio instructions\n');
 write(path.join(rootDir, 'docs/releases/0.2.0-alpha.4.md'), 'Fixture alpha release notes\n');
 write(path.join(rootDir, 'docs/NATIVE-DAW-ACCEPTANCE.md'), 'Fixture host acceptance procedure\n');
 for (const helper of ['Setup MIDI-LLM.cmd', 'setup-midi-llm.py'])
  write(path.join(rootDir,'scripts/midi-llm',helper),'Fixture setup helper');
 const buildSha = commit(rootDir);
 const base = path.join(buildDir, 'LoopForge_artefacts/Release');
 const app = path.join(base, 'Standalone/LoopForge AI MIDI.exe');
 const bundle = path.join(base, 'VST3/LoopForge AI MIDI.vst3');
 const module = path.join(bundle, 'Contents/x86_64-win/LoopForge AI MIDI.vst3');
 write(app, pe()); write(module, pe(true));
 write(path.join(bundle, 'Contents/Resources/moduleinfo.json'), '{"Name":"fixture"}\n');
 write(path.join(bundle, 'Contents/Resources/nested/extra.bin'), Buffer.from([0, 1, 254, 255]));
 write(path.join(bundle, 'Contents/Resources/empty.dat'), Buffer.alloc(0));
 return { rootDir, juceDir, buildDir, output: path.join(temp, 'output'), platform: 'windows', cmake, app, bundle, module, buildSha, juceSha };
}
function pack(options) { return require('../scripts/package-native.cjs').packageNative(options); }
function digest(file) { return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex'); }

test('missing standalone and missing bundle never change an existing output', t => {
 const f = fixture(t); fs.mkdirSync(f.output); write(path.join(f.output, 'prior.zip'), 'keep previous release');
 for (const source of [f.app, f.bundle]) {
  const moved = source + '.hidden'; fs.renameSync(source, moved);
  assert.throws(() => pack(f), /missing|Standalone|bundle/i);
  assert.equal(fs.readFileSync(path.join(f.output, 'prior.zip'), 'utf8'), 'keep previous release');
  assert.deepEqual(fs.readdirSync(f.output), ['prior.zip']); fs.renameSync(moved, source);
 }
});

test('text placeholders, wrong architecture and incomplete bundles are rejected without creating output', t => {
 const f = fixture(t);
 for (const bytes of [Buffer.from('placeholder .exe'), pe().subarray(0, 100), (() => { const b = pe(); b.writeUInt16LE(0x14c, 132); return b; })()]) {
  write(f.app, bytes); assert.throws(() => pack(f), /executable|PE|x64|binary/i); assert.equal(fs.existsSync(f.output), false);
 }
 write(f.app, pe()); fs.unlinkSync(f.module);
 assert.throws(() => pack(f), /missing|module|bundle/i); assert.equal(fs.existsSync(f.output), false);
});

test('staging copies every bundle file and writes independent hashes and truthful provenance', { skip: !haveCmake && 'CMake required for native ZIP packaging' }, t => {
 const f = fixture(t), result = pack(f), manifest = JSON.parse(fs.readFileSync(path.join(result.packageDir, 'manifest.json')));
 assert.equal(manifest.product, 'LoopForge AI MIDI'); assert.equal(manifest.version, '0.2.0');
 assert.equal(manifest.platform, 'windows'); assert.equal(manifest.architecture, 'x64');
 assert.equal(manifest.source.buildSha, f.buildSha); assert.equal(manifest.source.dirty, false);
 assert.equal(manifest.juce.commit, f.juceSha); assert.equal(manifest.signing.status, 'unsigned');
 assert.equal(manifest.runtime.nodeRequired, false); assert.equal(manifest.validation.hostLoaded, false);
 const nested = 'VST3/LoopForge AI MIDI.vst3/Contents/Resources/nested/extra.bin';
 assert.deepEqual(fs.readFileSync(path.join(result.packageDir, nested)), Buffer.from([0, 1, 254, 255]));
 assert.ok(manifest.files.some(f => f.path === nested));
 assert.equal(fs.statSync(path.join(result.packageDir, 'VST3/LoopForge AI MIDI.vst3/Contents/Resources/empty.dat')).size, 0);
 for (const file of manifest.files) {
  assert.equal(file.sha256, digest(path.join(result.packageDir, file.path)));
  assert.equal(file.bytes, fs.statSync(path.join(result.packageDir, file.path)).size);
 }
 for (const notice of noticeFiles) assert.equal(fs.readFileSync(path.join(result.packageDir, 'licenses/JUCE', notice), 'utf8'), `Fixture notice: ${notice}\n`);
 assert.equal(fs.readFileSync(path.join(f.output, 'SHA256SUMS'), 'utf8'), `${digest(result.archive)}  ${path.basename(result.archive)}\n`);
 const extract = path.join(path.dirname(f.output), 'extracted'); fs.mkdirSync(extract);
 execFileSync(cmake, ['-E', 'tar', 'xf', result.archive], { cwd: extract });
 assert.deepEqual(fs.readFileSync(path.join(extract, path.basename(result.packageDir), nested)), Buffer.from([0, 1, 254, 255]));
 // Cross a wall-clock second so ZIP local creation-time extras cannot hide a mismatch.
 Atomics.wait(new Int32Array(new SharedArrayBuffer(4)),0,0,1100);
 const oldMask = process.umask(0o077); let again;
 try { again = pack({ ...f, output: f.output + '-again' }); } finally { process.umask(oldMask); }
 if (process.platform !== 'win32') assert.equal(fs.statSync(path.join(again.packageDir, 'manifest.json')).mode & 0o777, 0o644);
 assert.equal(digest(again.archive), digest(result.archive));
 assert.throws(() => pack(f), /exists|overwrite/i);
});

test('changed JUCE notices and missing notices are rejected rather than silently omitted', t => {
 const f = fixture(t), notice = path.join(f.juceDir, noticeFiles[1]);
 fs.appendFileSync(notice, 'changed'); assert.throws(() => pack(f), /JUCE.*(dirty|modif)|clean.*JUCE/i);
 fs.unlinkSync(notice); assert.throws(() => pack(f), /JUCE.*(dirty|modif)|missing|notice/i);
 assert.equal(fs.existsSync(f.output), false);
});

test('bundle symlinks cannot pull unrelated files into the package', { skip: process.platform === 'win32' && 'Creating symlinks may need Windows privileges' }, t => {
 const f = fixture(t); fs.symlinkSync(f.rootDir, path.join(f.bundle, 'unrelated'));
 assert.throws(() => pack(f), /symlink|symbolic/i); assert.equal(fs.existsSync(f.output), false);
});

function elf(shared = false) {
 const b = Buffer.alloc(512); b.write('\x7fELF', 0, 'binary'); b[4] = 2; b[5] = 1; b[6] = 1;
 b.writeUInt16LE(shared ? 3 : 2, 16); b.writeUInt16LE(62, 18); b.writeUInt32LE(1, 20);
 b.writeBigUInt64LE(64n, 32); b.writeUInt16LE(64, 52); b.writeUInt16LE(56, 54); b.writeUInt16LE(1, 56);
 b.writeUInt32LE(1, 64); b.writeUInt32LE(5, 68); b.writeBigUInt64LE(512n, 96); return b;
}
function linuxFixture(t) {
 const f = fixture(t); fs.unlinkSync(f.app); fs.unlinkSync(f.module);
 f.app = path.join(path.dirname(f.app), 'LoopForge AI MIDI');
 f.module = path.join(f.bundle, 'Contents/x86_64-linux/LoopForge AI MIDI.so');
 f.platform = 'linux'; write(f.app, elf()); write(f.module, elf(true)); return f;
}
test('a Linux VST3 must be a shared ELF module, not an executable with a renamed extension', t => {
 const f = linuxFixture(t); write(f.module, elf());
 assert.throws(() => pack(f), /executable|ELF|binary/i); assert.equal(fs.existsSync(f.output), false);
});
test('Linux package retains executable permission and failed archiving cleans only its scratch files', { skip: !haveCmake && 'CMake required for native ZIP packaging' }, t => {
 const f = linuxFixture(t), keep = path.join(path.dirname(f.output), 'unrelated.mid'); write(keep, 'user fixture');
 assert.throws(() => pack({ ...f, cmake: path.join(path.dirname(f.output), 'missing-cmake') }), /ENOENT/);
 assert.equal(fs.existsSync(f.output), false);
 assert.equal(fs.readdirSync(path.dirname(f.output)).some(name => name.startsWith('.loopforge-package-')), false);
 assert.equal(fs.readFileSync(keep, 'utf8'), 'user fixture');
 const result = pack(f);
 assert.equal(JSON.parse(fs.readFileSync(path.join(result.packageDir, 'manifest.json'))).platform, 'linux');
 if (process.platform !== 'win32') assert.equal(fs.statSync(path.join(result.packageDir, 'Standalone/LoopForge AI MIDI')).mode & 0o777, 0o755);
 if (process.platform !== 'win32') {
  const extract = path.join(path.dirname(f.output), 'linux-extract'); fs.mkdirSync(extract);
  execFileSync(cmake, ['-E', 'tar', 'xf', result.archive], { cwd: extract });
  assert.equal(fs.statSync(path.join(extract, path.basename(result.packageDir), 'Standalone/LoopForge AI MIDI')).mode & 0o100, 0o100);
 }
});

test('candidate includes FL guide, release notes and corresponding source', { skip: !haveCmake }, t => {
 const r=pack(fixture(t));
 assert.match(fs.readFileSync(path.join(r.packageDir,'FL-STUDIO-TEST.md'),'utf8'), /FL Studio/);
 assert.match(fs.readFileSync(path.join(r.packageDir,'RELEASE-NOTES.md'),'utf8'), /alpha/);
 assert.ok(fs.statSync(path.join(r.packageDir,'Sources/LoopForge.tar')).size>0);
 assert.ok(fs.statSync(path.join(r.packageDir,'Sources/JUCE.tar')).size>0);
});

test('tracked source changes cannot be packaged against a different HEAD archive', t => {
 const f=fixture(t);fs.appendFileSync(path.join(f.rootDir,'LICENSE'),'uncommitted change');
 assert.throws(()=>pack(f),/tracked|commit.*source/i);assert.equal(fs.existsSync(f.output),false);
 git(f.rootDir,'add','LICENSE');assert.throws(()=>pack(f),/tracked|commit.*source/i);assert.equal(fs.existsSync(f.output),false);
});
