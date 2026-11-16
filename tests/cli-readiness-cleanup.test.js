'use strict';
const test=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),os=require('node:os'),path=require('node:path');
const {spawn}=require('node:child_process');
function alive(pid){try{process.kill(pid,0);return process.platform!=='linux'||! /\) Z /.test(fs.readFileSync(`/proc/${pid}/stat`,'utf8'))}catch{return false}}
test('readiness timeout reaps a SIGTERM-resistant locator and ordinary descendant before resolving',{skip:process.platform==='win32'},async t=>{
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'loopforge-ready-cleanup-'));
 const pids=path.join(dir,'pids.json');
 fs.writeFileSync(path.join(dir,'which'),`#!${process.execPath}\nconst fs=require('node:fs'),{spawn}=require('node:child_process');const child=spawn(process.execPath,['-e',"process.on('SIGTERM',()=>{});setInterval(()=>{},1000)"],{stdio:'ignore'});fs.writeFileSync(${JSON.stringify(pids)},JSON.stringify([process.pid,child.pid]));process.on('SIGTERM',()=>{});setInterval(()=>{},1000);`,{mode:0o755});
 const source=`const {available}=require('./src/agent-cli');available('codex-cli').then(value=>{console.log(JSON.stringify({value}));process.exit(0)});`;
 const child=spawn(process.execPath,['-e',source],{cwd:path.resolve(__dirname,'..'),env:{...process.env,PATH:dir+path.delimiter+process.env.PATH},stdio:['ignore','pipe','pipe']});
 let output='';child.stdout.on('data',b=>output+=b);child.stderr.resume();
 t.after(()=>{child.kill('SIGKILL');if(fs.existsSync(pids))for(const pid of JSON.parse(fs.readFileSync(pids,'utf8')))try{process.kill(pid,'SIGKILL')}catch{}fs.rmSync(dir,{recursive:true,force:true});});
 await new Promise((resolve,reject)=>{const timer=setTimeout(()=>{child.kill('SIGKILL');reject(new Error('readiness did not complete within 6s'))},6000);child.on('error',reject);child.on('close',()=>{clearTimeout(timer);resolve()});});
 assert.deepEqual(JSON.parse(output),{value:false});
 const owned=JSON.parse(fs.readFileSync(pids,'utf8'));
 assert.deepEqual(owned.filter(alive),[],'locator and ordinary child must be dead before readiness resolves');
});
