'use strict';
const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),os=require('node:os'),path=require('node:path'),cp=require('node:child_process');
function fixture(t){const dir=fs.mkdtempSync(path.join(os.tmpdir(),'lf-export-test-'));t.after(()=>fs.rmSync(dir,{recursive:true,force:true}));fs.mkdirSync(path.join(dir,'src'));for(const name of ['operations','core','schema','ai','agent-cli'])fs.copyFileSync(path.join(__dirname,'../src',name+'.js'),path.join(dir,'src',name+'.js'));return dir;}
for(const mode of ['partial','flush','close','rename'])test('failed '+mode+' preserves existing exported MIDI and permits retry',t=>{
 const dir=fixture(t);const script=`
 const fs=require('node:fs'),assert=require('node:assert/strict');const root=${JSON.stringify(dir)};
 const {exportIdea}=require(root+'/src/operations'),{generateOne,createMidi}=require(root+'/src/core');
 const old=generateOne({},42),next=generateOne({},43);const result=exportIdea(old,'existing');const before=fs.readFileSync(result.path);
 const originals={write:fs.writeFileSync,flush:fs.fsyncSync,close:fs.closeSync,rename:fs.renameSync};let fired=false;
 const fail=()=>{fired=true;throw Object.assign(Error('injected IO failure'),{code:'EIO'})};
 if(${JSON.stringify(mode)}==='partial')fs.writeFileSync=(dest,data,...args)=>{originals.write(dest,Buffer.from(data).subarray(0,64),...args);fail()};
 if(${JSON.stringify(mode)}==='flush')fs.fsyncSync=fail;
 if(${JSON.stringify(mode)}==='close')fs.closeSync=fd=>{originals.close(fd);if(!fired)fail()};
 if(${JSON.stringify(mode)}==='rename')fs.renameSync=fail;
 assert.throws(()=>exportIdea(next,'existing'));assert.equal(fired,true);assert.deepEqual(fs.readFileSync(result.path),before);
 Object.assign(fs,{writeFileSync:originals.write,fsyncSync:originals.flush,closeSync:originals.close,renameSync:originals.rename});
 assert.deepEqual(fs.readdirSync(root+'/exports'),['existing.mid']);const retry=exportIdea(next,'existing');assert.deepEqual(fs.readFileSync(retry.path),Buffer.from(createMidi(next)));assert.equal(retry.bytes,fs.statSync(retry.path).size);
 `;const r=cp.spawnSync(process.execPath,['-e',script],{encoding:'utf8',timeout:10000});assert.equal(r.status,0,r.stderr);
});
test('actual file-size limit cannot truncate prior MIDI', {skip:process.platform!=='linux'},t=>{
 const check=cp.spawnSync('prlimit',['--version']);if(check.error){t.skip('prlimit unavailable');return;}const dir=fixture(t),core=require('../src/core');fs.mkdirSync(dir+'/exports');const before=Buffer.from(core.createMidi(core.generateOne({},42)));fs.writeFileSync(dir+'/exports/existing.mid',before);
 const script=`process.on('SIGXFSZ',()=>{});const {exportIdea}=require(${JSON.stringify(dir+'/src/operations')});const {generateOne}=require(${JSON.stringify(dir+'/src/core')});try{exportIdea(generateOne({},43),'existing');process.exitCode=2}catch{}`;
 const r=cp.spawnSync('prlimit',['--fsize=64:64',process.execPath,'-e',script],{encoding:'utf8',timeout:10000});assert.equal(r.status,0,r.stderr);assert.deepEqual(fs.readFileSync(dir+'/exports/existing.mid'),before);assert.deepEqual(fs.readdirSync(dir+'/exports'),['existing.mid']);
});
