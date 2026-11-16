'use strict';
const test=require('node:test');const {after}=require('node:test');const assert=require('node:assert/strict');
const fs=require('node:fs');const os=require('node:os');const path=require('node:path');const http=require('node:http');
const {spawn}=require('node:child_process');
const {available}=require('../src/agent-cli');
const {configuration}=require('../src/operations');
const {listener}=require('../src/server');

// A locator that always outlives the 2000ms probe deadline, so probe timing is
// governed by the deadline rather than by machine-specific process-start cost.
// Windows: spawn without shell only resolves .exe, so the fake `where` is a
// warmed copy of node.exe (hardlinked per fixture) running a long-lived script.
let sharedDir=null,sharedExe=null;
async function locatorBinary(){
 if(sharedExe)return sharedExe;
 sharedDir=fs.mkdtempSync(path.join(os.tmpdir(),'loopforge-locator-bin-'));
 sharedExe=path.join(sharedDir,'where.exe');
 fs.copyFileSync(process.execPath,sharedExe);
 fs.writeFileSync(path.join(sharedDir,'warm'),'process.exit(0)');
 await new Promise(resolve=>{const child=spawn(sharedExe,['warm'],{cwd:sharedDir,windowsHide:true,stdio:'ignore'});const timer=setTimeout(()=>{try{child.kill()}catch{}resolve();},30000);child.on('close',()=>{clearTimeout(timer);resolve();});});
 return sharedExe;
}
after(()=>{if(sharedDir)fs.rmSync(sharedDir,{recursive:true,force:true});});
async function slowLocatorFixture(t){
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'loopforge-locator-'));
 const oldPath=process.env.PATH,oldCwd=process.cwd();
 if(process.platform==='win32'){
  fs.linkSync(await locatorBinary(),path.join(dir,'where.exe'));
  const script='setTimeout(()=>process.exit(0),5000);';
  fs.writeFileSync(path.join(dir,'codex'),script);fs.writeFileSync(path.join(dir,'claude'),script);
  process.chdir(dir);
 }else{
  fs.writeFileSync(path.join(dir,'which'),'#!/bin/sh\nsleep 5\nexit 0\n',{mode:0o755});
 }
 process.env.PATH=dir+path.delimiter+oldPath;
 t.after(()=>{process.env.PATH=oldPath;process.chdir(oldCwd);fs.rmSync(dir,{recursive:true,force:true});});
 return {dir};
}
test('readiness probes are asynchronous and deduplicated per binary',async t=>{
 await slowLocatorFixture(t);
 const p1=available('codex-cli'),p2=available('codex-cli');
 assert.equal(typeof p1.then,'function','available must return a promise, not block synchronously');
 assert.equal(p1,p2,'concurrent callers must share one in-flight probe');
 const c1=configuration(),c2=configuration();
 assert.equal(typeof c1.then,'function');
 const [a,b]=await Promise.all([p1,c1]);
 assert.equal(a,false);assert.equal(b['codex-cli'].ready,false);
 await c2;
});
test('configuration keeps the event loop responsive during slow CLI locators',async t=>{
 await slowLocatorFixture(t);
 // Ordering, not absolute ms: process creation can briefly block on Windows (AV),
 // but the locator's execution must never block the loop. Register the timer first.
 let configDone=false,timerFiredWhilePending=null;
 const timer=setTimeout(()=>{timerFiredWhilePending=!configDone;},10);
 const pending=configuration().then(c=>{configDone=true;return c;});
 const config=await pending;clearTimeout(timer);
 assert.equal(timerFiredWhilePending,true,'10ms timer must fire while locators are still running');
 assert.ok(config['codex-cli']&&config['claude-cli']);
});
test('cached readiness resolves without respawning the locator',async t=>{
 await slowLocatorFixture(t);
 assert.equal(await available('codex-cli'),false);
 const started=Date.now();
 assert.equal(await available('codex-cli'),false);
 assert.ok(Date.now()-started<500,'cached readiness must resolve immediately, not re-probe');
});
test('PATH changes invalidate the readiness cache',async t=>{
 const f=await slowLocatorFixture(t);
 await available('codex-cli');
 process.env.PATH=process.env.PATH+path.delimiter+f.dir;
 const started=Date.now();
 await available('codex-cli');
 assert.ok(Date.now()-started>=1500,'a changed PATH must trigger a fresh slow probe');
});
test('locator slower than the 2000ms deadline reports not ready without blocking',async t=>{
 await slowLocatorFixture(t);
 const started=Date.now();let probeDone=false,timerFiredWhilePending=null;
 const timer=setTimeout(()=>{timerFiredWhilePending=!probeDone;},10);
 const pending=available('codex-cli').then(v=>{probeDone=true;return v;});
 assert.equal(await pending,false);clearTimeout(timer);
 const elapsed=Date.now()-started;
 assert.equal(timerFiredWhilePending,true,'10ms timer must fire while the locator is still running');
 assert.ok(elapsed>=1900&&elapsed<3500,`probe deadline must stay near 2000ms, took ${elapsed}ms`);
});
test('HTTP serves other requests while CLI readiness probes are slow',async t=>{
 await slowLocatorFixture(t);
 const server=http.createServer(listener);await new Promise(r=>server.listen(0,'127.0.0.1',r));
 t.after(()=>new Promise(r=>{server.closeAllConnections();server.close(r)}));
 const headers={Host:'127.0.0.1:'+(process.env.LOOPFORGE_PORT||4177)};
 const request=url=>new Promise((resolve,reject)=>{const req=http.request(url,{headers},res=>{let data='';res.on('data',b=>data+=b);res.on('end',()=>resolve({status:res.statusCode,data}));});req.on('error',reject);req.end();});
 const base='http://127.0.0.1:'+server.address().port;
 let statusDone=false;
 const status=request(base+'/api/status').then(r=>{statusDone=true;return r;});
 const page=await request(base+'/');
 assert.equal(page.status,200);
 assert.equal(statusDone,false,'static page must be served while readiness probes are still running');
 const result=await status;assert.equal(result.status,200);assert.ok(JSON.parse(result.data).providers['codex-cli']);
});