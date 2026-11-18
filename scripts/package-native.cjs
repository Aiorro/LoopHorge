#!/usr/bin/env node
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { execFileSync } = require('node:child_process');

const NOTICE_FILES = [
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
const epoch = new Date('1980-01-01T00:00:00Z');
function git(dir, ...args) { return execFileSync('git', ['-C', dir, ...args], { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] }).trim(); }
function hash(bytes) { return crypto.createHash('sha256').update(bytes).digest('hex'); }
function regular(file) {
 let stat;
 try { stat = fs.lstatSync(file); } catch { throw new Error(`Missing package source: ${file}`); }
 if (!stat.isFile() || stat.isSymbolicLink()) throw new Error(`Expected regular file, not a symlink: ${file}`);
 return stat;
}
function filesIn(dir) {
 let stat;
 try { stat = fs.lstatSync(dir); } catch { throw new Error(`Missing bundle directory: ${dir}`); }
 if (!stat.isDirectory() || stat.isSymbolicLink()) throw new Error(`Expected bundle directory, not a symlink: ${dir}`);
 return fs.readdirSync(dir).sort().flatMap(name => {
  if (/[\r\n\\:]/.test(name)) throw new Error(`Unsupported package filename: ${name}`);
  const file = path.join(dir, name), entry = fs.lstatSync(file);
  if (entry.isSymbolicLink()) throw new Error(`Bundle symlink is not allowed: ${file}`);
  if (entry.isDirectory()) return filesIn(file);
  regular(file); return [file];
 });
}
function executable(file, platform, plugin) {
 regular(file); const b = fs.readFileSync(file);
 const invalid = () => { throw new Error(`Invalid ${platform} x64 executable binary: ${file}`); };
 if (platform === 'windows') {
  if (b.length < 64 || b.toString('ascii', 0, 2) !== 'MZ') invalid();
  const pe = b.readUInt32LE(60);
  if (pe + 24 > b.length || b.toString('binary', pe, pe + 4) !== 'PE\0\0') invalid();
  const sections = b.readUInt16LE(pe + 6), optional = b.readUInt16LE(pe + 20), characteristics = b.readUInt16LE(pe + 22);
  if (b.readUInt16LE(pe + 4) !== 0x8664 || !sections || optional < 240 || pe + 24 + optional + sections * 40 > b.length
      || b.readUInt16LE(pe + 24) !== 0x20b || !(characteristics & 2) || Boolean(characteristics & 0x2000) !== plugin) invalid();
  let code = false;
  for (let n = 0; n < sections; n++) {
   const s = pe + 24 + optional + n * 40, bytes = b.readUInt32LE(s + 16), offset = b.readUInt32LE(s + 20);
   if (bytes && (offset === 0 || offset + bytes > b.length)) invalid();
   if (bytes && (b.readUInt32LE(s + 36) & 0x20000000)) code = true;
  }
  if (!code) invalid();
  // Certificate presence is not proof of a valid publisher signature.
  return b.readUInt32LE(pe + 24 + 112 + 4 * 8) !== 0;
 }
 if (b.length < 64 || b.toString('binary', 0, 4) !== '\x7fELF' || b[4] !== 2 || b[5] !== 1
     || b.readUInt16LE(18) !== 62 || !(plugin ? [3] : [2, 3]).includes(b.readUInt16LE(16))) invalid();
 const offset = Number(b.readBigUInt64LE(32)), size = b.readUInt16LE(54), count = b.readUInt16LE(56);
 if (!Number.isSafeInteger(offset) || size < 56 || !count || offset + size * count > b.length) invalid();
 let load = false;
 for (let n = 0; n < count; n++) {
  const p = offset + n * size;
  if (b.readUInt32LE(p) !== 1) continue;
  const start = b.readBigUInt64LE(p + 8), bytes = b.readBigUInt64LE(p + 32);
  if (start + bytes > BigInt(b.length)) invalid();
  if (bytes && (b.readUInt32LE(p + 4) & 1)) load = true;
 }
 if (!load) invalid();
 return false;
}
// libarchive writes local UT creation times even with --mtime. Canonicalize
// timestamp extras in both local and central records; payload bytes/CRC stay intact.
function canonicalZipTimes(bytes) {
 const end=bytes.length-22;if(end<0||bytes.readUInt32LE(end)!==0x06054b50)throw Error('Unsupported ZIP directory');
 let at=bytes.readUInt32LE(end+16);const count=bytes.readUInt16LE(end+10);
 const extras=(start,length)=>{const finish=start+length;if(finish>bytes.length)throw Error('Invalid ZIP extra');
  for(let p=start;p<finish;){if(p+4>finish)throw Error('Invalid ZIP extra');const id=bytes.readUInt16LE(p),n=bytes.readUInt16LE(p+2);p+=4;if(p+n>finish)throw Error('Invalid ZIP extra');
   if(id===0x5455&&n){const flags=bytes[p];let q=p+1;for(let bit=1;bit<=4;bit<<=1)if(flags&bit){if(q+4>p+n)break;bytes.writeUInt32LE(315532800,q);q+=4;}}
   p+=n;
  }
 };
 for(let i=0;i<count;++i){if(at+46>bytes.length||bytes.readUInt32LE(at)!==0x02014b50)throw Error('Invalid ZIP entry');const name=bytes.readUInt16LE(at+28),extra=bytes.readUInt16LE(at+30),comment=bytes.readUInt16LE(at+32),local=bytes.readUInt32LE(at+42);
  if(local+30>bytes.length||bytes.readUInt32LE(local)!==0x04034b50)throw Error('Invalid ZIP local entry');
  extras(local+30+bytes.readUInt16LE(local+26),bytes.readUInt16LE(local+28));extras(at+46+name,extra);at+=46+name+extra+comment;
 }return bytes;
}
function packageNative({ buildDir, juceDir, output, platform = process.platform === 'win32' ? 'windows' : process.platform,
 rootDir = path.resolve(__dirname, '..'), cmake = process.env.CMAKE_COMMAND || 'cmake' }) {
 if (!['windows', 'linux'].includes(platform)) throw new Error('Only windows and linux x64 candidates are supported.');
 if (!buildDir || !juceDir || !output) throw new Error('Required: --build-dir, --juce-dir and --output.');
 [buildDir, juceDir, output, rootDir] = [buildDir, juceDir, output, rootDir].map(p => path.resolve(p));
 const base = path.join(buildDir, 'LoopForge_artefacts/Release');
 const appName = 'LoopForge AI MIDI' + (platform === 'windows' ? '.exe' : '');
 const app = path.join(base, 'Standalone', appName), bundle = path.join(base, 'VST3/LoopForge AI MIDI.vst3');
 const module = path.join(bundle, 'Contents', platform === 'windows' ? 'x86_64-win' : 'x86_64-linux',
  platform === 'windows' ? 'LoopForge AI MIDI.vst3' : 'LoopForge AI MIDI.so');
 const appSigned = executable(app, platform, false), moduleSigned = executable(module, platform, true);
 const bundleFiles = filesIn(bundle);
 const cmakeSource = fs.readFileSync(path.join(rootDir, 'plugin/CMakeLists.txt'), 'utf8');
 const jucePin = cmakeSource.match(/GIT_TAG\s+([0-9a-f]{40})\b/)?.[1];
 const version = cmakeSource.match(/project\(LoopForge\s+VERSION\s+([0-9.]+)/)?.[1];
 if (!jucePin || !version || git(juceDir, 'rev-parse', 'HEAD') !== jucePin) throw new Error('JUCE source must match the pinned CMake GIT_TAG.');
 if (git(juceDir, 'status', '--porcelain')) throw new Error('JUCE source is dirty or modified; use a clean pinned checkout.');
 const copies = [[app, `Standalone/${appName}`], ...bundleFiles.map(file => [file, `VST3/LoopForge AI MIDI.vst3/${path.relative(bundle, file).split(path.sep).join('/')}`]),
  [path.join(rootDir, 'LICENSE'), 'licenses/LoopForge-LICENSE.txt'],
  ...['AGPL-3.0.txt','GPL-3.0.txt','NATIVE-DISTRIBUTION.md'].map(file => [path.join(rootDir,'licenses',file), `licenses/${file}`]),
  [path.join(rootDir, 'docs/NATIVE-PACKAGING.md'), 'README.md'],
  [path.join(rootDir, 'docs/NATIVE-DAW-ACCEPTANCE.md'), 'DAW-ACCEPTANCE.md'],
  [path.join(rootDir, 'docs/FL-STUDIO-TEST.md'), 'FL-STUDIO-TEST.md'],
  [path.join(rootDir, 'docs/releases/0.2.0-alpha.2.md'), 'RELEASE-NOTES.md'],
  ...NOTICE_FILES.map(file => [path.join(juceDir, file), `licenses/JUCE/${file}`])];
 for (const [source] of copies) regular(source);
 if (fs.existsSync(output)) throw new Error(`Output already exists; refusing to overwrite: ${output}`);
 // Output must be separate from all inputs; the packager only removes its own scratch directory.
 for (const source of [rootDir, buildDir, juceDir]) if (source === output || source.startsWith(output + path.sep)) throw new Error('Output cannot contain an input directory.');
 if (git(rootDir,'diff','HEAD','--name-only')) throw new Error('Commit tracked source changes before packaging corresponding source.');
 const buildSha = git(rootDir, 'rev-parse', 'HEAD'), dirty = Boolean(git(rootDir, 'status', '--porcelain'));
 const name = `LoopForge-${version}-${platform}-x64-${buildSha.slice(0, 12)}${dirty ? '-dirty' : ''}-candidate`;
 fs.mkdirSync(path.dirname(output), { recursive: true });
 const scratch = fs.mkdtempSync(path.join(path.dirname(output), '.loopforge-package-'));
 try {
  const packageDir = path.join(scratch, name), files = [];
  for (const [source, relative] of copies.sort((a, b) => a[1] < b[1] ? -1 : a[1] > b[1] ? 1 : 0)) {
   const destination = path.join(packageDir, relative), bytes = fs.readFileSync(source);
   fs.mkdirSync(path.dirname(destination), { recursive: true }); fs.writeFileSync(destination, bytes);
   fs.chmodSync(destination, platform === 'linux' && relative === `Standalone/${appName}` ? 0o755 : 0o644);
   fs.utimesSync(destination, epoch, epoch);
   files.push({ path: relative, bytes: bytes.length, sha256: hash(bytes) });
  }
  fs.mkdirSync(path.join(packageDir,'Sources'), {recursive:true});
  for (const [dir, label, commit] of [[rootDir,'LoopForge',buildSha],[juceDir,'JUCE',jucePin]]) {
   if (git(dir,'submodule','status')) throw new Error('Source archives require no unbundled Git submodules.');
   const relative=`Sources/${label}.tar`, destination=path.join(packageDir,relative);
   execFileSync('git',['-C',dir,'-c','tar.umask=0022','archive','--format=tar','--output',destination,commit]);
   const bytes=fs.readFileSync(destination);files.push({path:relative,bytes:bytes.length,sha256:hash(bytes)});
  }
  const manifest = {
   formatVersion: 1, product: 'LoopForge AI MIDI', version, stage: 'release-candidate', platform, architecture: 'x64',
   source: { buildSha, dirty, provenance: 'Packaging checkout; build correspondence must be verified by the builder.' },
   juce: { commit: jucePin }, signing: { status: appSigned || moduleSigned ? 'unverified-signature-present' : 'unsigned', verified: false },
   runtime: { nodeRequired: false, networkRequiredForOfflineGeneration: false },
   validation: { executableStructureChecked: true, hostLoaded: false, cleanInstallTested: false, redistributionLicenseCleared: false },
   scope: { maximumNewLoopBars: 16, fullSongGeneration: false }, files
  };
  const manifestBytes = Buffer.from(JSON.stringify(manifest, null, 2) + '\n');
  fs.writeFileSync(path.join(packageDir, 'manifest.json'), manifestBytes);
  fs.writeFileSync(path.join(packageDir, 'SHA256SUMS'), [...files, { path: 'manifest.json', sha256: hash(manifestBytes) }].map(f => `${f.sha256}  ${f.path}\n`).join(''));
  // Give directory entries the same modes/times too; no wall-clock or absolute paths enter the ZIP.
  function normalize(dir) { for (const entry of fs.readdirSync(dir)) { const p = path.join(dir, entry); if (fs.statSync(p).isDirectory()) normalize(p); else { fs.chmodSync(p, platform === 'linux' && p === path.join(packageDir, 'Standalone', appName) ? 0o755 : 0o644); fs.utimesSync(p, epoch, epoch); } } fs.chmodSync(dir, 0o755); fs.utimesSync(dir, epoch, epoch); }
  normalize(packageDir);
  const archiveName = `${name}.zip`;
  execFileSync(cmake, ['-E', 'tar', 'cf', path.join(scratch, archiveName), '--format=zip', '--mtime=1980-01-01 00:00:00 UTC', '--', name], { cwd: scratch, stdio: ['ignore', 'pipe', 'pipe'] });
  const zipPath=path.join(scratch,archiveName);fs.writeFileSync(zipPath,canonicalZipTimes(fs.readFileSync(zipPath)));
  fs.writeFileSync(path.join(scratch, 'SHA256SUMS'), `${hash(fs.readFileSync(path.join(scratch, archiveName)))}  ${archiveName}\n`);
  fs.renameSync(scratch, output);
  return { packageDir: path.join(output, name), archive: path.join(output, archiveName) };
 } finally { fs.rmSync(scratch, { recursive: true, force: true }); }
}
module.exports = { packageNative };
if (require.main === module) {
 try {
  const args = process.argv.slice(2), options = {}, keys = { '--build-dir': 'buildDir', '--juce-dir': 'juceDir', '--output': 'output', '--platform': 'platform', '--cmake': 'cmake' };
  if (args.length === 1 && args[0] === '--help') {
   console.log('node scripts/package-native.cjs --build-dir BUILD --juce-dir PINNED_JUCE --output NEW_DIRECTORY [--platform windows|linux] [--cmake CMAKE_EXECUTABLE]');
  } else {
   for (let n = 0; n < args.length; n += 2) { if (!keys[args[n]] || !args[n + 1] || args[n + 1].startsWith('--') || options[keys[args[n]]]) throw new Error(`Invalid or duplicate argument: ${args[n]}`); options[keys[args[n]]] = args[n + 1]; }
   console.log(JSON.stringify(packageNative(options), null, 2));
  }
 } catch (error) { console.error(`Native packaging failed: ${error.message}`); process.exitCode = 1; }
}
