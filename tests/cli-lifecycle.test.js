'use strict';
const test=require('node:test');const assert=require('node:assert/strict');const fs=require('node:fs');const os=require('node:os');const path=require('node:path');
const {execPrompt}=require('../src/agent-cli');
function fixture(t,script,bin='codex'){
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'loopforge-cli-'));const old=process.env.PATH;
 const file=path.join(dir,'fake.cjs');fs.writeFileSync(file,script);
 if(process.platform==='win32')fs.writeFileSync(path.join(dir,bin+'.cmd'),`@echo off\r\n"${process.execPath}" "${file}" %*\r\n`);
 else fs.writeFileSync(path.join(dir,bin),`#!${process.execPath}\n${script}`,{mode:0o755});
 process.env.PATH=dir+path.delimiter+old;
 t.after(()=>{process.env.PATH=old;fs.rmSync(dir,{recursive:true,force:true})});return dir;
}
test('CLI rejects pre-aborted requests before launching',async t=>{
 fixture(t,"process.stdin.resume();process.stdin.on('end',()=>console.log('unexpected success'))");
 await assert.rejects(execPrompt('codex-cli','hello',{signal:AbortSignal.abort()}),/cancelled/);
});
test('CLI model is a single validated argument and output preserves UTF-8',async t=>{
 fixture(t,"process.stdin.resume();process.stdin.on('end',()=>process.stdout.write(JSON.stringify(process.argv.slice(2))+' Привет'))");
 const output=await execPrompt('codex-cli','hello',{model:'test/model-1'});assert.match(output,/"--model","test\/model-1"/);assert.match(output,/Привет/);
 await assert.rejects(execPrompt('codex-cli','hello',{model:'bad; command'}),/model/);
});
test('explicit CLI generation works after a cached negative readiness check',async t=>{
 const dir=fixture(t,"process.stdin.resume();process.stdin.on('end',()=>console.log('ready'))");
 process.env.PATH=dir;
 const command=path.join(dir,process.platform==='win32'?'codex.cmd':'codex');
 fs.renameSync(command,command+'.pending');
 const {available}=require('../src/agent-cli');
 assert.equal(await available('codex-cli'),false);
 fs.renameSync(command+'.pending',command);
 assert.equal((await execPrompt('codex-cli','hello')).trim(),'ready');
});
for(const mode of ['abort','timeout','overflow'])test(`CLI ${mode} terminates resistant descendants`,async t=>{
 const dir=fixture(t,'');
 // The fake agent reports the descendant PID, then keeps the request open.
 const script=`const fs=require('node:fs'),{spawn}=require('node:child_process');
 const child=spawn(process.execPath,['-e',"process.on('SIGTERM',()=>{});setInterval(()=>{},1000)"],{stdio:'ignore'});
 fs.writeFileSync(${JSON.stringify(path.join(dir,'pid'))},String(child.pid));
 process.on('SIGTERM',()=>{});process.stdin.resume();setInterval(()=>{${mode==='overflow'?"process.stdout.write('x'.repeat(16384))":''}},10);`;
 fs.writeFileSync(path.join(dir,process.platform==='win32'?'fake.cjs':'codex'),(process.platform==='win32'?'':`#!${process.execPath}\n`)+script,{mode:0o755});
 const controller=new AbortController();const started=Date.now();
 const result=execPrompt('codex-cli','hello',{signal:controller.signal,timeoutMs:mode==='timeout'?(process.platform==='win32'?4000:400):10000,maxOutputBytes:1024}).then(()=>null,e=>e);
 for(let i=0;i<600&&!fs.existsSync(path.join(dir,'pid'));i++)await new Promise(r=>setTimeout(r,10));
 assert.ok(fs.existsSync(path.join(dir,'pid')),'agent must start');const pid=Number(fs.readFileSync(path.join(dir,'pid'),'utf8'));
 t.after(()=>{try{process.kill(pid,'SIGKILL')}catch{}});
 if(mode==='abort')controller.abort();
 const error=await result;assert.match(error.message,mode==='abort'?/cancelled/:mode==='timeout'?/timed out/:/limit/);assert.ok(Date.now()-started<(process.platform==='win32'?9000:2500));
 // On Linux a killed orphan can remain a zombie until container init reaps it.
 let alive=true;
 for(let i=0;i<100;i++){try{process.kill(pid,0);alive=process.platform==='linux'?!/\) Z /.test(fs.readFileSync(`/proc/${pid}/stat`,'utf8')):true}catch{alive=false}if(!alive)break;await new Promise(r=>setTimeout(r,10))}
 assert.equal(alive,false,'descendant still running');
});
test('HTTP disconnect cancels the CLI tree through operations',async t=>{
 const http=require('node:http');const {listener}=require('../src/server');const dir=fixture(t,'');
 const script=`const fs=require('node:fs'),{spawn}=require('node:child_process');const c=spawn(process.execPath,['-e','setInterval(()=>{},1000)'],{stdio:'ignore'});fs.writeFileSync(${JSON.stringify(path.join(dir,'pid'))},String(c.pid));process.stdin.resume();setInterval(()=>{},1000);`;
 fs.writeFileSync(path.join(dir,process.platform==='win32'?'fake.cjs':'codex'),(process.platform==='win32'?'':`#!${process.execPath}\n`)+script,{mode:0o755});
 const server=http.createServer(listener);await new Promise(r=>server.listen(0,'127.0.0.1',r));t.after(()=>{server.closeAllConnections();server.close()});
 const req=http.request({host:'127.0.0.1',port:server.address().port,path:'/api/generate',method:'POST',headers:{Host:'127.0.0.1:'+(process.env.LOOPFORGE_PORT||4177),'Content-Type':'application/json'}});req.on('error',()=>{});req.end('{"provider":"codex-cli","model":"test-model"}');
 for(let i=0;i<600&&!fs.existsSync(path.join(dir,'pid'));i++)await new Promise(r=>setTimeout(r,10));
 assert.ok(fs.existsSync(path.join(dir,'pid')));const pid=Number(fs.readFileSync(path.join(dir,'pid'),'utf8'));t.after(()=>{try{process.kill(pid,'SIGKILL')}catch{}});req.destroy();
 let alive=true;for(let i=0;i<100;i++){try{process.kill(pid,0);alive=process.platform==='linux'?!/\) Z /.test(fs.readFileSync(`/proc/${pid}/stat`,'utf8')):true}catch{alive=false}if(!alive)break;await new Promise(r=>setTimeout(r,10))}
 assert.equal(alive,false,'disconnected HTTP client left CLI descendant running');
});

test('Claude CLI keeps plan mode and unwraps its JSON envelope',async t=>{
 fixture(t,"process.stdin.resume();process.stdin.on('end',()=>console.log(JSON.stringify({result:JSON.stringify({args:process.argv.slice(2),ok:true})})))",'claude');
 const result=JSON.parse(await execPrompt('claude-cli','hello',{model:'test-model'}));assert.equal(result.ok,true);assert.ok(result.args.includes('--permission-mode'));assert.ok(result.args.includes('plan'));assert.ok(result.args.includes('test-model'));
});

test('CLI success reaps descendants after their parent has exited',async t=>{
 const dir=fixture(t,'');const script=`const fs=require('node:fs'),{spawn}=require('node:child_process');const c=spawn(process.execPath,['-e','setInterval(()=>{},1000)'],{stdio:'ignore'});fs.writeFileSync(${JSON.stringify(path.join(dir,'pid'))},String(c.pid));console.log('completed');process.exit(0);`;
 fs.writeFileSync(path.join(dir,process.platform==='win32'?'fake.cjs':'codex'),(process.platform==='win32'?'':`#!${process.execPath}\n`)+script,{mode:0o755});
 assert.match(await execPrompt('codex-cli','hello'),/completed/);const pid=Number(fs.readFileSync(path.join(dir,'pid'),'utf8'));t.after(()=>{try{process.kill(pid,'SIGKILL')}catch{}});
 let alive=true;for(let i=0;i<100;i++){try{process.kill(pid,0);alive=process.platform==='linux'?!/\) Z /.test(fs.readFileSync(`/proc/${pid}/stat`,'utf8')):true}catch{alive=false}if(!alive)break;await new Promise(r=>setTimeout(r,10))}assert.equal(alive,false,'orphan still running after success');
});

for(const pipe of ['stdout','stderr','both'])for(const code of [0,7])test(`root exit ${code} completes despite inherited ${pipe}`,async t=>{
 const dir=fixture(t,'');const pidfile=path.join(dir,'pid');
 const script=`const fs=require('node:fs'),{spawn}=require('node:child_process');
 const c=spawn(process.execPath,['-e',"process.on('SIGTERM',()=>{});setInterval(()=>{},1000)"],{stdio:['ignore',${pipe==='stderr'?"'ignore'":'1'},${pipe==='stdout'?"'ignore'":'2'}]});
 fs.writeFileSync(${JSON.stringify(pidfile)},String(c.pid));
 process.stdout.write('Привет '+ 'x'.repeat(65536),()=>process.exit(${code}));`;
 fs.writeFileSync(path.join(dir,process.platform==='win32'?'fake.cjs':'codex'),(process.platform==='win32'?'':`#!${process.execPath}\n`)+script,{mode:0o755});
 let result,error;try{result=await execPrompt('codex-cli','hello',{timeoutMs:process.platform==='win32'?10000:2000})}catch(e){error=e}
 assert.ok(fs.existsSync(pidfile));const pid=Number(fs.readFileSync(pidfile,'utf8'));t.after(()=>{try{process.kill(pid,'SIGKILL')}catch{}});
 if(code===0){assert.ifError(error);assert.equal(result,'Привет '+'x'.repeat(65536));}else{assert.match(error?.message||'',/exited with code 7/);assert.doesNotMatch(error.message,/timed out/);}
 let alive=true;for(let i=0;i<100;i++){try{process.kill(pid,0);alive=process.platform==='linux'?!/\) Z /.test(fs.readFileSync(`/proc/${pid}/stat`,'utf8')):true}catch{alive=false}if(!alive)break;await new Promise(r=>setTimeout(r,10));}assert.equal(alive,false,'ordinary descendant survived root exit');
});
