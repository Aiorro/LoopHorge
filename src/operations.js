'use strict';
const fs=require('node:fs');
const path=require('node:path');
const crypto=require('node:crypto');
const {generateEight,createMidi}=require('./core');
const {normalizeOptions,validateIdea}=require('./schema');
const {requestAI,configuration:apiConfiguration}=require('./ai');
const {generateUsingAccountCLI,available}=require('./agent-cli');
async function generate(params={}, {signal}={}){
 const opts=normalizeOptions(params);
 const provider=String(params.provider||'offline');
 const count=Math.max(1,Math.min(8,Math.round(Number(params.count)||8)));
 if(provider==='codex-cli'||provider==='claude-cli')return await generateUsingAccountCLI(provider,opts,{count,description:params.description});
 if(provider==='offline'){
   const patterns=generateEight(opts, Number.isInteger(params.seed)?params.seed:undefined).slice(0,count);
   return {ideas:patterns,sources:patterns.map(()=> 'offline'),provider:'offline',model:'rules-v1'};
 }
 return await requestAI(provider,opts,{count,model:params.model,description:params.description,signal});
}
function exportsDir(){return path.resolve(__dirname,'../exports')}
function exportIdea(raw,basename){
 const opts=normalizeOptions(raw?.metadata||{});
 const idea=validateIdea(raw,opts);
 const filename= typeof basename==='string' && /^[a-zA-Z0-9_-]{1,45}$/.test(basename) ? basename : `loopforge-${Date.now()}-${crypto.randomBytes(3).toString('hex')}`;
 const directory=exportsDir();fs.mkdirSync(directory,{recursive:true});
 const output=path.join(directory,filename+'.mid');
 fs.writeFileSync(output,createMidi(idea));
 return {path:output,filename:path.basename(output),bytes:fs.statSync(output).size};
}
function configuration(){return {...apiConfiguration(),'codex-cli':{ready:available('codex-cli'),defaultModel:'ChatGPT signed-in CLI'},'claude-cli':{ready:available('claude-cli'),defaultModel:'Claude signed-in CLI'}}}
module.exports={generate,exportIdea,configuration,exportsDir};
