'use strict';
const test=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),{spawn}=require('node:child_process');
const SERVER=path.resolve(__dirname,'../src/mcp-server.js');
const delay=ms=>new Promise(resolve=>setTimeout(resolve,ms));
const deadline=process.platform==='win32'?20000:4000;
async function until(check,message,limit=deadline){
 const end=Date.now()+limit;while(Date.now()<end){const result=check();if(result)return result;await delay(10)}
 assert.fail(message);
}
function alive(pid){try{process.kill(pid,0);return process.platform!=='linux'||! /\) Z /.test(fs.readFileSync(`/proc/${pid}/stat`,'utf8'))}catch{return false}}
function fixture(t,{api=false,stubborn=false}={}){
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'loopforge-mcp-life-'));
 const script=`const fs=require('node:fs'),{spawn}=require('node:child_process');
 const model=process.argv[process.argv.indexOf('--model')+1];
 const child=spawn(process.execPath,['-e',"process.on('SIGTERM',()=>{});setInterval(()=>{},1000)"],{stdio:'ignore'});
 const ready=${JSON.stringify(dir)}+'/'+model+'.'+process.pid+'.tmp';
 fs.writeFileSync(ready,JSON.stringify([process.pid,child.pid]));
 fs.writeFileSync(${JSON.stringify(dir)}+'/'+process.pid+'.owned',JSON.stringify([process.pid,child.pid]));
 fs.renameSync(ready,${JSON.stringify(dir)}+'/'+model+'.json');
 process.on('SIGTERM',()=>{});process.stdin.resume();setInterval(()=>{},1000);`;
 const file=path.join(dir,'fake.cjs');fs.writeFileSync(file,script);
 if(process.platform==='win32')fs.writeFileSync(path.join(dir,'codex.cmd'),`@echo off\r\n"${process.execPath}" "${file}" %*\r\n`);
 else fs.writeFileSync(path.join(dir,'codex'),`#!${process.execPath}\n${script}`,{mode:0o755});
 const preload=path.join(dir,'api.cjs');
 fs.writeFileSync(preload,`const fs=require('node:fs');global.fetch=async(_url,{signal,body})=>{
 const model=JSON.parse(body).model;fs.writeFileSync(${JSON.stringify(dir)}+'/'+model+'.started','');
 return new Promise((_resolve,reject)=>{${stubborn?"setInterval(()=>{},1000);":"signal.addEventListener('abort',()=>setTimeout(()=>{fs.writeFileSync("+JSON.stringify(dir)+"+'/'+model+'.aborted','');reject(new Error('fixture transport aborted'))},80),{once:true});"}});
 };`);
 const child=spawn(process.execPath,[...(api?['-r',preload]:[]),SERVER],{env:{...process.env,PATH:dir+path.delimiter+(process.env.PATH||'')},stdio:['pipe','pipe','pipe']});
 const replies=[];let partial='',stderr='';child.stdout.on('data',chunk=>{partial+=chunk.toString();let n;while((n=partial.indexOf('\n'))>=0){const line=partial.slice(0,n);partial=partial.slice(n+1);if(line)replies.push(JSON.parse(line))}});
 child.stderr.on('data',chunk=>stderr+=chunk);child.stdin.on('error',()=>{});
 const closed=new Promise(resolve=>child.on('close',(code,signal)=>resolve({code,signal})));
 t.after(async()=>{
  child.kill('SIGKILL');child.stdout.resume();await closed;
  for(const name of fs.readdirSync(dir).filter(name=>name.endsWith('.owned')))for(const pid of JSON.parse(fs.readFileSync(path.join(dir,name),'utf8')))try{process.kill(pid,'SIGKILL')}catch{}
  fs.rmSync(dir,{recursive:true,force:true});
 });
 return {dir,child,replies,closed,stderr:()=>stderr,
  send:message=>child.stdin.write(JSON.stringify({jsonrpc:'2.0',...message})+'\n'),
  reply:(id,count=1)=>until(()=>replies.filter(reply=>reply.id===id).length>=count&&replies.filter(reply=>reply.id===id)[count-1],`No reply for ${typeof id} ${id}`),
  pids:model=>until(()=>fs.existsSync(path.join(dir,model+'.json'))&&JSON.parse(fs.readFileSync(path.join(dir,model+'.json'),'utf8')),`CLI ${model} did not start`),
  marker:name=>until(()=>fs.existsSync(path.join(dir,name)),`Missing ${name}`)
 };
}
function generate(client,id,model,provider='codex-cli'){client.send({id,method:'tools/call',params:{name:'loopforge_generate',arguments:{provider,model,count:1}}})}
function cancel(client,id){client.send({method:'notifications/cancelled',params:{requestId:id,reason:'test cancellation'}})}
async function dead(pids){await until(()=>pids.every(pid=>!alive(pid)),'ordinary CLI tree survived cancellation')}
async function exited(client,limit=deadline){let result;client.closed.then(value=>{result=value});await until(()=>result,'MCP did not exit after shutdown',limit);assert.equal(client.stderr(),'');return result}

test('MCP cancellation owns the CLI tree, recovers its slot and ignores completed request cancellation',async t=>{
 const client=fixture(t);generate(client,1,'first');const first=await client.pids('first');cancel(client,1);
 assert.match((await client.reply(1)).result.content[0].text,/cancelled/);await dead(first);
 client.send({id:2,method:'ping'});assert.deepEqual((await client.reply(2)).result,{});
 generate(client,1,'unused','offline');assert.equal((await client.reply(1,2)).result.isError,false);
 cancel(client,1);generate(client,1,'reused');const reused=await client.pids('reused');assert.ok(reused.every(alive));
 cancel(client,1);assert.match((await client.reply(1,3)).result.content[0].text,/cancelled/);await dead(reused);
 assert.equal(client.replies.length,4,'notifications must not produce replies');
 client.child.stdin.end();assert.equal((await exited(client)).code,0);
});

test('MCP distinguishes numeric/string IDs and rejects duplicate active ownership',async t=>{
 const client=fixture(t);generate(client,7,'numeric');generate(client,'7','string');const numeric=await client.pids('numeric'),string=await client.pids('string');
 generate(client,7,'duplicate');await until(()=>client.replies.some(reply=>reply.error),'duplicate request was not rejected');
 assert.equal(fs.existsSync(path.join(client.dir,'duplicate.json')),false);assert.ok(numeric.every(alive));assert.ok(string.every(alive));
 cancel(client,7);assert.match((await client.reply(7)).result.content[0].text,/cancelled/);await dead(numeric);assert.ok(string.every(alive));
 cancel(client,'7');assert.match((await client.reply('7')).result.content[0].text,/cancelled/);await dead(string);
 client.child.stdin.end();assert.equal((await exited(client)).code,0);
});

test('MCP never launches tools/call notifications or requests with invalid IDs',async t=>{
 const client=fixture(t);
 for(const id of [undefined,null,{},true])client.send({...(id===undefined?{}:{id}),method:'tools/call',params:{name:'loopforge_generate',arguments:{provider:'codex-cli',model:'uncancellable'}}});
 client.send({id:10,method:'ping'});await client.reply(10);await delay(100);
 assert.equal(fs.existsSync(path.join(client.dir,'uncancellable.json')),false);
 assert.ok(client.replies.every(reply=>Object.hasOwn(reply,'id')),'every response must carry a valid response ID');
 client.child.stdin.end();assert.equal((await exited(client)).code,0);
});

for(const phase of ['EOF','SIGTERM','SIGINT'])test(`MCP ${phase} cleans up root and ordinary CLI descendants`,{skip:process.platform==='win32'&&phase!=='EOF'},async t=>{
 const client=fixture(t);generate(client,1,'shutdown');const pids=await client.pids('shutdown');
 if(phase==='EOF')client.child.stdin.end();else client.child.kill(phase);
 assert.equal((await exited(client)).code,0);await dead(pids);
 assert.match((await client.reply(1)).result.content[0].text,/cancelled/);
});

test('MCP cancellation bypasses all eight occupied API slots and capacity recovers',async t=>{
 const client=fixture(t,{api:true});for(let id=0;id<8;id++)generate(client,id,'api-'+id,'ollama');
 await Promise.all(Array.from({length:8},(_,id)=>client.marker('api-'+id+'.started')));
 generate(client,8,'too-many','ollama');assert.equal((await client.reply(8)).error.code,-32000);
 client.send({id:99,method:'ping'});assert.deepEqual((await client.reply(99)).result,{});
 cancel(client,0);assert.match((await client.reply(0)).result.content[0].text,/cancelled/);await client.marker('api-0.aborted');
 generate(client,9,'api-recovered','ollama');await client.marker('api-recovered.started');cancel(client,9);
 assert.match((await client.reply(9)).result.content[0].text,/cancelled/);
 for(let id=1;id<8;id++)cancel(client,id);
 for(let id=1;id<8;id++)assert.match((await client.reply(id)).result.content[0].text,/cancelled/);
 assert.equal(client.replies.length,11,'notifications must bypass capacity without sending responses');
 client.child.stdin.end();assert.equal((await exited(client)).code,0);
});

test('MCP EOF awaits API abort completion before exiting',async t=>{
 const client=fixture(t,{api:true});generate(client,1,'api-shutdown','ollama');await client.marker('api-shutdown.started');client.child.stdin.end();
 assert.equal((await exited(client)).code,0);assert.ok(fs.existsSync(path.join(client.dir,'api-shutdown.aborted')));
 assert.match((await client.reply(1)).result.content[0].text,/cancelled/);
});

test('MCP shutdown deadline bounds a transport that does not cooperate with abort',async t=>{
 const client=fixture(t,{api:true,stubborn:true});generate(client,1,'stubborn','ollama');await client.marker('stubborn.started');const started=Date.now();client.child.stdin.end();
 assert.equal((await exited(client,8000)).code,1);assert.ok(Date.now()-started<8000);
});

for(const phase of ['EOF','SIGTERM'])test(`MCP ${phase} remains bounded when a client stops reading replies`,{skip:process.platform==='win32'&&phase==='SIGTERM'},async t=>{
 const client=fixture(t);generate(client,'job','blocked');const pids=await client.pids('blocked');
 let exit;client.child.once('exit',(code,signal)=>{exit={code,signal}});client.child.stdout.pause();
 for(let id=0;id<256;id++)client.send({id,method:'tools/list'});
 await delay(150);if(phase==='EOF')client.child.stdin.end();else client.child.kill('SIGTERM');
 await until(()=>exit,'blocked stdout prevented shutdown',8000);assert.equal(exit.code,1);assert.equal(exit.signal,null);
 client.child.stdout.resume();await exited(client);await dead(pids);
});

test('MCP resumes input after reply backpressure drains and clears the stall deadline',async t=>{
 const client=fixture(t);generate(client,'job','drain');const pids=await client.pids('drain');client.child.stdout.pause();
 for(let id=0;id<256;id++)client.send({id,method:'tools/list'});
 await delay(150);client.child.stdout.resume();await until(()=>client.replies.length===256,'reply stream did not drain');
 await delay(5100);client.send({id:1000,method:'ping'});assert.deepEqual((await client.reply(1000)).result,{});
 cancel(client,'job');assert.match((await client.reply('job')).result.content[0].text,/cancelled/);await dead(pids);
 client.child.stdin.end();assert.equal((await exited(client)).code,0);
});
