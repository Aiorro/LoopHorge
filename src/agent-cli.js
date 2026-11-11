'use strict';
const {spawn,spawnSync}=require('node:child_process');
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
function available(name){const spec=SPECS[name];if(!spec)return false;
 const test=spawnSync(process.platform==='win32'?'where':'which',[spec.bin],{encoding:'utf8',timeout:2000,windowsHide:true});return test.status===0;
}
function execPrompt(name,input){
 const spec=SPECS[name];if(!spec)throw new Error('Unknown account CLI');
 if(!available(name))throw new Error(`${spec.bin} CLI not found. Install it and sign in on this computer.`);
 const cwd=path.join(os.tmpdir(),'loopforge-ai-agent');fs.mkdirSync(cwd,{recursive:true});
 return new Promise((resolve,reject)=>{
  const child=spawn(spec.bin,spec.args,{cwd,env:process.env,shell:process.platform==='win32',windowsHide:true,stdio:['pipe','pipe','pipe']});
  let output='',stderr='',done=false;
  const timeout=setTimeout(()=>{child.kill();finish(new Error(`${name} timed out after 120 seconds`))},120000);
  function finish(error,result){if(done)return;done=true;clearTimeout(timeout);error?reject(error):resolve(result)}
  child.stdout.on('data',b=>{output+=b.toString('utf8');if(output.length>5_000_000){child.kill();finish(new Error('AI agent output exceeded safety limit'))}});
  child.stderr.on('data',b=>{if(stderr.length<1000)stderr+=b.toString('utf8')});
  child.on('error',e=>finish(new Error(`${name}: ${e.message}`)));
  child.on('close',code=>{
   if(done)return;if(code!==0)return finish(new Error(`${name} exited with code ${code}: ${stderr.slice(-300)}`));
   if(name==='claude-cli'){try{const result=JSON.parse(output);if(typeof result.result==='string')output=result.result;}catch{}}
   finish(null,output);
  });
  child.stdin.on('error',()=>{});child.stdin.end(input);
 });
}
async function generateUsingAccountCLI(provider,opts,{count=8,description=''}={}){
 const output=await execPrompt(provider,prompt(opts,count,description));
 const ideas=normalizeResponse(output,opts,count);
 const sources=ideas.map(()=>provider);
 if(ideas.length<count){const missing=generateEight(opts).slice(0,count-ideas.length);ideas.push(...missing);sources.push(...missing.map(()=> 'offline-fallback'))}
 return {provider,model:'authenticated CLI account',ideas,sources};
}
module.exports={available,execPrompt,generateUsingAccountCLI};
