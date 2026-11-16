'use strict';
const {spawn}=require('node:child_process');
const fs=require('node:fs');
const path=require('node:path');
const os=require('node:os');
const {normalizeResponse}=require('./schema');
const {generateEight}=require('./core');
const {prompt}=require('./ai');
const SPECS={
 'codex-cli': {bin:'codex',args:['exec','--ephemeral','--sandbox','read-only','--skip-git-repo-check','-']},
 'claude-cli': {bin:'claude',args:['-p','--output-format','json','--max-turns','1','--permission-mode','plan']}
};
const READINESS_TTL_MS=5000;
const readiness=new Map();
function probeLocator(bin){
 return new Promise(resolve=>{
  const child=spawn(process.platform==='win32'?'where':'which',[bin],{stdio:['ignore','ignore','ignore'],windowsHide:true});
  let timedOut=false,grace;
  const finish=value=>{clearTimeout(timer);clearTimeout(grace);resolve(value)};
  const timer=setTimeout(()=>{timedOut=true;try{child.kill()}catch{}grace=setTimeout(()=>finish(false),1000);},2000);
  child.on('error',()=>finish(false));
  // Resolve only after close so a timed-out locator cannot outlive the probe (fixture cleanup, slot reuse).
  child.on('close',code=>finish(!timedOut&&code===0));
 });
}
function available(name){
 const spec=SPECS[name];if(!spec)return Promise.resolve(false);
 const searchPath=process.env.PATH||'';
 const cached=readiness.get(name);
 if(cached&&cached.searchPath===searchPath){
  if(cached.promise)return cached.promise;
  if(Date.now()-cached.at<READINESS_TTL_MS)return Promise.resolve(cached.value);
 }
 const entry={searchPath};
 entry.promise=probeLocator(spec.bin).then(value=>{
  if(readiness.get(name)===entry){entry.value=value;entry.at=Date.now();delete entry.promise;}
  return value;
 });
 readiness.set(name,entry);
 return entry.promise;
}
async function execPrompt(name,input,{signal,model,timeoutMs=120000,maxOutputBytes=5_000_000}={}){
 if(signal?.aborted)throw new Error('Generation cancelled');
 const spec=SPECS[name];if(!spec)throw new Error('Unknown account CLI');
 if(model!==undefined&&(typeof model!=='string'||model.length>100||!/^[-\w.\/:]+$/.test(model)))throw new Error('Invalid model name');
 if(!await available(name))throw new Error(`${spec.bin} CLI not found. Install it and sign in on this computer.`);
 const cwd=fs.mkdtempSync(path.join(os.tmpdir(),'loopforge-ai-agent-'));
 try{return await new Promise((resolve,reject)=>{
  const args=model?[...spec.args.filter(a=>a!=='-'),'--model',model,...(name==='codex-cli'?['-']:[])]:spec.args;
  const windows=process.platform==='win32';
  const env=windows?{...process.env,LOOPFORGE_JOB_HELPER:path.join(__dirname,'cli-job.cs'),LOOPFORGE_JOB_COMMAND:[spec.bin,...args].join(' ')}:process.env;
  const launcher=windows?path.join(process.env.SystemRoot||'C:/Windows','System32','WindowsPowerShell','v1.0','powershell.exe'):spec.bin;
  const launchArgs=windows?['-NoLogo','-NoProfile','-NonInteractive','-Command',"$ErrorActionPreference='Stop'; try { Add-Type -Path $env:LOOPFORGE_JOB_HELPER; exit [LoopForgeCliJob]::Run() } catch { [Console]::Error.WriteLine('CLI process containment failed'); exit 125 }"]:args;
  const child=spawn(launcher,launchArgs,{cwd,env,shell:false,detached:!windows,windowsHide:true,stdio:['pipe','pipe','pipe']});
  const chunks=[];let size=0,done=false;
  const timeout=setTimeout(()=>finish(new Error(`${name} timed out`)),Math.min(120000,Math.max(1,timeoutMs)));
  function terminate(){
   if(!child.pid)return Promise.resolve();
   if(process.platform!=='win32'){try{process.kill(-child.pid,'SIGKILL')}catch{}return Promise.resolve()}
   if(child.exitCode!==null||child.signalCode!==null)return Promise.resolve();
   return new Promise((resolve,reject)=>{
    const deadline=setTimeout(()=>reject(new Error('CLI process containment could not be confirmed')),3000);
    child.once('close',()=>{clearTimeout(deadline);resolve()});
    if(!child.kill()){clearTimeout(deadline);reject(new Error('CLI process containment could not be confirmed'))}
   });
  }
  async function finish(error,result){
   if(done)return;done=true;clearTimeout(timeout);signal?.removeEventListener('abort',cancel);
   try{await terminate()}catch(cleanup){return reject(cleanup)}
   error?reject(error):resolve(result);
  }
  function cancel(){finish(new Error('Generation cancelled'))}
  signal?.addEventListener('abort',cancel,{once:true});if(signal?.aborted)cancel();
  child.stdout.on('data',b=>{if(done)return;size+=b.length;if(size>maxOutputBytes)finish(new Error('AI agent output exceeded safety limit'));else chunks.push(b)});
  child.stderr.resume();
  child.on('error',()=>finish(new Error(`${name} could not start; check CLI installation`)));
  // Root exit precedes pipe close; reap ordinary descendants so inherited pipes can drain.
  child.on('exit',()=>{if(!done&&!windows)void terminate()});
  child.on('close',code=>{
   if(done)return;
   if(windows&&code===125)return finish(new Error('Windows CLI containment failed; check PowerShell policy and Job Object support'));
   if(code!==0)return finish(new Error(`${name} exited with code ${code}; check CLI login and model access`));
   let output=Buffer.concat(chunks).toString('utf8');
   if(name==='claude-cli'){try{const result=JSON.parse(output);if(typeof result.result==='string')output=result.result;}catch{}}
   finish(null,output);
  });
  child.stdin.on('error',()=>{});child.stdin.end(input);
 });}finally{try{fs.rmSync(cwd,{recursive:true,force:true})}catch{}}
}
async function generateUsingAccountCLI(provider,opts,{count=8,description='',signal,model}={}){
 const output=await execPrompt(provider,prompt(opts,count,description),{signal,model});
 const ideas=normalizeResponse(output,opts,count);
 const sources=ideas.map(()=>provider);
 if(ideas.length<count){const missing=generateEight(opts).slice(0,count-ideas.length);ideas.push(...missing);sources.push(...missing.map(()=> 'offline-fallback'))}
 return {provider,model:model||'authenticated CLI account',ideas,sources};
}
module.exports={available,execPrompt,generateUsingAccountCLI};
