'use strict';
const test=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),http=require('node:http');
const {dispatch}=require('../src/mcp-server');
const {listener}=require('../src/server');
const PRIVATE_OUTPUT='{"x":SECRET_KEY}';

function fakeCLI(t){
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'loopforge-redaction-')),previous=process.env.PATH;
 const script=`process.stdin.resume();process.stdin.on('end',()=>process.stdout.write(${JSON.stringify(PRIVATE_OUTPUT)}));`;
 if(process.platform==='win32'){
  const file=path.join(dir,'fake.cjs');fs.writeFileSync(file,script);
  fs.writeFileSync(path.join(dir,'codex.cmd'),`@echo off\r\n"${process.execPath}" "${file}" %*\r\n`);
 }else fs.writeFileSync(path.join(dir,'codex'),`#!${process.execPath}\n${script}`,{mode:0o755});
 process.env.PATH=dir+path.delimiter+(previous||'');
 t.after(()=>{if(previous===undefined)delete process.env.PATH;else process.env.PATH=previous;fs.rmSync(dir,{recursive:true,force:true})});
}
async function httpRequest(t){
 const server=http.createServer(listener);await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 t.after(()=>new Promise(resolve=>{server.closeAllConnections();server.close(resolve)}));
 return (endpoint,args)=>new Promise((resolve,reject)=>{
  const req=http.request({hostname:'127.0.0.1',port:server.address().port,path:endpoint,method:'POST',headers:{Host:'127.0.0.1:'+(process.env.LOOPFORGE_PORT||4177),'Content-Type':'application/json'}},res=>{
   const chunks=[];res.on('data',chunk=>chunks.push(chunk));res.on('end',()=>resolve({status:res.statusCode,body:JSON.parse(Buffer.concat(chunks).toString('utf8'))}));
  });req.on('error',reject);req.end(JSON.stringify(args));
 });
}
for(const source of ['API','CLI'])test(`${source} malformed composition diagnostics redact private model output`,async t=>{
 const provider=source==='API'?'ollama':'codex-cli';
 if(source==='API'){
  const original=global.fetch;global.fetch=async()=>new Response(JSON.stringify({choices:[{message:{content:PRIVATE_OUTPUT}}]}));
  t.after(()=>{global.fetch=original});
 }else fakeCLI(t);
 const request=await httpRequest(t);
 await t.test('MCP',async()=>{
  const reply=await dispatch({jsonrpc:'2.0',id:1,method:'tools/call',params:{name:'loopforge_generate',arguments:{provider,count:1}}});
  assert.equal(reply.result.isError,true);
  const message=reply.result.content[0].text;assert.doesNotMatch(message,/SECRET_KEY/);assert.match(message,/invalid JSON/i);
 });
 await t.test('legacy HTTP',async()=>{
  const reply=await request('/api/generate',{provider,count:1});assert.equal(reply.status,422);
  assert.doesNotMatch(JSON.stringify(reply.body),/SECRET_KEY/);assert.match(reply.body.error,/invalid JSON/i);
 });
 await t.test('versioned HTTP',async()=>{
  const reply=await request('/api/v1/generate',{provider,count:1});assert.equal(reply.status,502);
  assert.equal(reply.body.error.code,'INVALID_RESPONSE');assert.doesNotMatch(JSON.stringify(reply.body),/SECRET_KEY/);
 });
});
