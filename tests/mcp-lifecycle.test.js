'use strict';
const test=require('node:test');const assert=require('node:assert/strict');
const fs=require('node:fs');const os=require('node:os');const path=require('node:path');
const {PassThrough}=require('node:stream');
const {startStdio}=require('../src/mcp-server');
const {generateOne}=require('../src/core');

function session(t){
 const input=new PassThrough(),output=new PassThrough();
 const replies=[];let buffer='';
 output.on('data',chunk=>{buffer+=chunk.toString('utf8');let index;while((index=buffer.indexOf('\n'))>=0){const line=buffer.slice(0,index);buffer=buffer.slice(index+1);if(line)replies.push(JSON.parse(line));}});
 let exitCode=null,exitResolve;const exited=new Promise(resolve=>{exitResolve=resolve;});
 const started=startStdio({input,output,exit:code=>{exitCode=code;exitResolve();}});
 const send=obj=>input.write(JSON.stringify(obj)+'\n');
 const waitFor=async(predicate,label,timeoutMs=8000)=>{const deadline=Date.now()+timeoutMs;while(!predicate()){if(Date.now()>deadline)throw new Error('Timed out waiting for '+label);await new Promise(r=>setTimeout(r,10));}};
 t.after(()=>{input.destroy();output.destroy();});
 return {input,output,replies,send,waitFor,started,get exitCode(){return exitCode},exited};
}
function hangingFetch(t){
 const state={called:false,aborted:false};const previous=global.fetch;
 global.fetch=async(_url,options)=>{state.called=true;return new Response(new ReadableStream({start(stream){options.signal.addEventListener('abort',()=>{state.aborted=true;stream.error(new Error('private socket details'));});}}));};
 t.after(()=>{global.fetch=previous;});
 return state;
}
function cliFixture(t){
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'loopforge-mcp-cli-'));const old=process.env.PATH;
 const script=`const fs=require('node:fs');fs.writeFileSync(${JSON.stringify(path.join(dir,'pid'))},String(process.pid));process.stdin.resume();setInterval(()=>{},1000);`;
 fs.writeFileSync(path.join(dir,'fake.cjs'),script);
 if(process.platform==='win32')fs.writeFileSync(path.join(dir,'codex.cmd'),`@echo off\r\n"${process.execPath}" "${path.join(dir,'fake.cjs')}" %*\r\n`);
 else fs.writeFileSync(path.join(dir,'codex'),`#!${process.execPath}\n${script}`,{mode:0o755});
 process.env.PATH=dir+path.delimiter+old;
 t.after(()=>{process.env.PATH=old;fs.rmSync(dir,{recursive:true,force:true});});
 return dir;
}
async function assertPidDead(t,pid){
 let alive=true;
 for(let i=0;i<200;i++){try{process.kill(pid,0);alive=process.platform==='linux'?!/\) Z /.test(fs.readFileSync(`/proc/${pid}/stat`,'utf8')):true;}catch{alive=false;}if(!alive)break;await new Promise(r=>setTimeout(r,10));}
 assert.equal(alive,false,'CLI process survived cancellation');
}
const generateCall=(id,provider)=>({jsonrpc:'2.0',id,method:'tools/call',params:{name:'loopforge_generate',arguments:{provider,count:1}}});

test('MCP cancellation notification aborts a hanging provider and frees the slot',async t=>{
 const s=session(t);const fetchState=hangingFetch(t);
 s.send(generateCall(1,'ollama'));
 await s.waitFor(()=>fetchState.called,'provider request to start');
 s.send({jsonrpc:'2.0',method:'notifications/cancelled',params:{requestId:1}});
 await s.waitFor(()=>fetchState.aborted,'provider stream to be aborted');
 s.send({jsonrpc:'2.0',id:2,method:'ping'});
 await s.waitFor(()=>s.replies.some(r=>r.id===2&&r.result),'ping after cancellation');
 assert.equal(s.replies.some(r=>r.id===1),false,'cancelled request must not receive a response');
 s.send(generateCall(3,'offline'));
 await s.waitFor(()=>s.replies.some(r=>r.id===3&&r.result),'slot reuse after cancellation');
 assert.equal(JSON.parse(s.replies.find(r=>r.id===3).result.content[0].text).ideas.length,1);
});
test('MCP cancellation notification terminates a running CLI and frees the slot',async t=>{
 const dir=cliFixture(t);const s=session(t);
 s.send(generateCall(1,'codex-cli'));
 await s.waitFor(()=>fs.existsSync(path.join(dir,'pid')),'CLI to start');
 const pid=Number(fs.readFileSync(path.join(dir,'pid'),'utf8'));t.after(()=>{try{process.kill(pid,'SIGKILL')}catch{}});
 s.send({jsonrpc:'2.0',method:'notifications/cancelled',params:{requestId:1}});
 await assertPidDead(t,pid);
 s.send({jsonrpc:'2.0',id:2,method:'ping'});
 await s.waitFor(()=>s.replies.some(r=>r.id===2&&r.result),'ping after CLI cancellation');
 assert.equal(s.replies.some(r=>r.id===1),false,'cancelled CLI request must not receive a response');
});
test('MCP EOF aborts active generation and exits cleanly',async t=>{
 const s=session(t);const fetchState=hangingFetch(t);
 s.send(generateCall(1,'ollama'));
 await s.waitFor(()=>fetchState.called,'provider request to start');
 s.input.end();
 await s.waitFor(()=>fetchState.aborted,'provider stream to be aborted on EOF');
 await s.waitFor(()=>s.exitCode===0,'clean exit after EOF');
});
test('MCP graceful shutdown aborts a running CLI and awaits cleanup',async t=>{
 const dir=cliFixture(t);const s=session(t);
 s.send(generateCall(1,'codex-cli'));
 await s.waitFor(()=>fs.existsSync(path.join(dir,'pid')),'CLI to start');
 const pid=Number(fs.readFileSync(path.join(dir,'pid'),'utf8'));t.after(()=>{try{process.kill(pid,'SIGKILL')}catch{}});
 await s.started.shutdown();
 await assertPidDead(t,pid);
 assert.equal(s.exitCode,0,'shutdown must exit cleanly');
});
test('MCP cancellation ignores unknown and malformed request ids',async t=>{
 const s=session(t);
 s.send({jsonrpc:'2.0',method:'notifications/cancelled',params:{requestId:'no-such-request'}});
 s.send({jsonrpc:'2.0',method:'notifications/cancelled',params:{requestId:{nested:true}}});
 s.send({jsonrpc:'2.0',method:'notifications/cancelled'});
 s.send({jsonrpc:'2.0',id:9,method:'ping'});
 await s.waitFor(()=>s.replies.some(r=>r.id===9&&r.result),'ping after stray cancellations');
 assert.equal(s.replies.length,1);
});
test('MCP cancelled provider failure text is not sent for the cancelled request',async t=>{
 const s=session(t);const fetchState=hangingFetch(t);
 s.send(generateCall(1,'ollama'));
 await s.waitFor(()=>fetchState.called,'provider request to start');
 s.send({jsonrpc:'2.0',method:'notifications/cancelled',params:{requestId:1}});
 await s.waitFor(()=>fetchState.aborted,'abort');
 s.send(generateCall(2,'offline'));
 await s.waitFor(()=>s.replies.some(r=>r.id===2&&r.result),'recovery');
 assert.deepEqual(s.replies.map(r=>r.id),[2],'only the recovered request replies');
});