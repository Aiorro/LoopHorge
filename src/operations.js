'use strict';
const fs=require('node:fs');
const path=require('node:path');
const crypto=require('node:crypto');
const {generateEight,createMidi}=require('./core');
const {isLegacyMeter,normalizeOptions,validateIdea}=require('./schema');
const {requestAI,configuration:apiConfiguration}=require('./ai');
const {generateUsingAccountCLI,available}=require('./agent-cli');
async function generate(params={}, {signal}={}){
 const allowed=new Set(['provider','key','mode','style','bpm','bars','model','count','seed','description','action','timeSignature']);
 if(params&&typeof params==='object'&&Object.keys(params).some(name=>!allowed.has(name)))throw new Error('Unsupported generation fields; specify loop bars explicitly');
 const opts=normalizeOptions(params);
 if (params.timeSignature !== undefined && !isLegacyMeter(params.timeSignature))
   throw new Error('Legacy generation supports only a 4/4 time signature');
 const model=params.model===''?undefined:params.model;
 if(model!==undefined&&(typeof model!=='string'||model.length>100||!/^[-\w.\/:]+$/.test(model)))throw new Error('Invalid model name');
 const provider=String(params.provider||'offline');
 const count=Math.max(1,Math.min(8,Math.round(Number(params.count)||8)));
 if(provider==='codex-cli'||provider==='claude-cli')return await generateUsingAccountCLI(provider,opts,{count,description:params.description,signal,model});
 if(provider==='offline'){
   const patterns=generateEight(opts, Number.isInteger(params.seed)?params.seed:undefined).slice(0,count);
   return {ideas:patterns,sources:patterns.map(()=> 'offline'),provider:'offline',model:'rules-v1'};
 }
 return await requestAI(provider,opts,{count,model,description:params.description,signal});
}
function exportsDir(){return path.resolve(__dirname,'../exports')}
function exportIdea(raw,basename){
 const opts=normalizeOptions(raw?.metadata||{});
 const idea=validateIdea(raw,opts);
 const filename= typeof basename==='string' && /^[a-zA-Z0-9_-]{1,45}$/.test(basename) ? basename : `loopforge-${Date.now()}-${crypto.randomBytes(3).toString('hex')}`;
 const directory=exportsDir();fs.mkdirSync(directory,{recursive:true});
 const output=path.join(directory,filename+'.mid');
 const bytes=Buffer.from(createMidi(idea));
 const staging=path.join(directory,`.${filename}-${crypto.randomBytes(12).toString('hex')}.tmp`);
 let fd,owned=false;
 try{
   fd=fs.openSync(staging,'wx',0o600);owned=true;
   fs.writeFileSync(fd,bytes);
   if(fs.fstatSync(fd).size!==bytes.length)throw new Error('Incomplete MIDI write');
   fs.fsyncSync(fd);
   const closing=fd;fd=undefined;fs.closeSync(closing);
   fs.renameSync(staging,output);owned=false;
 }finally{
   if(fd!==undefined)try{fs.closeSync(fd)}catch{}
   if(owned)try{fs.unlinkSync(staging)}catch{}
 }
 return {path:output,filename:filename+'.mid',bytes:bytes.length};
}
async function configuration(){
 const [codex,claude]=await Promise.all([available('codex-cli'),available('claude-cli')]);
 return {...apiConfiguration(),'codex-cli':{ready:codex,defaultModel:'ChatGPT signed-in CLI'},'claude-cli':{ready:claude,defaultModel:'Claude signed-in CLI'}};
}
module.exports={generate,exportIdea,configuration,exportsDir};
