'use strict';
const test=require('node:test');const assert=require('node:assert/strict');
const fs=require('node:fs');const http=require('node:http');const {PassThrough}=require('node:stream');
const {spawnSync}=require('node:child_process');
const {listener}=require('../src/server');

test('invalid LOOPFORGE_PORT is rejected with a clear message instead of crashing',()=>{
 for(const value of ['abc','0','-1','70000','4177.5']){
  const r=spawnSync(process.execPath,['src/server.js'],{env:{...process.env,LOOPFORGE_PORT:value},encoding:'utf8',timeout:15000,windowsHide:true});
  assert.equal(r.status,1,`port ${value} must exit with code 1`);
  assert.match(r.stderr,/LOOPFORGE_PORT must be an integer between 1 and 65535/);
 }
});
test('static file read errors are handled without crashing the server',async t=>{
 const real=fs.createReadStream;
 let uncaught=null;const onUncaught=e=>{uncaught=e;};process.on('uncaughtException',onUncaught);
 fs.createReadStream=()=>{const s=new PassThrough();setImmediate(()=>s.emit('error',new Error('disk read failed')));return s;};
 const server=http.createServer(listener);await new Promise(r=>server.listen(0,'127.0.0.1',r));
 t.after(()=>{fs.createReadStream=real;process.removeListener('uncaughtException',onUncaught);server.closeAllConnections();server.close();});
 const headers={Host:'127.0.0.1:'+(process.env.LOOPFORGE_PORT||4177)};
 const base='http://127.0.0.1:'+server.address().port;
 await new Promise(resolve=>{const req=http.request(base+'/',{headers},res=>{res.on('error',()=>{});res.on('close',resolve);res.resume();});req.on('error',resolve);req.end();});
 await new Promise(r=>setTimeout(r,50));
 assert.equal(uncaught,null,'stream error must be handled, not crash the process');
 const status=await new Promise((resolve,reject)=>{const req=http.request(base+'/api/status',{headers},res=>{let d='';res.on('data',b=>d+=b);res.on('end',()=>resolve({status:res.statusCode}));});req.on('error',reject);req.end();});
 assert.equal(status.status,200,'server must remain available after a stream error');
});