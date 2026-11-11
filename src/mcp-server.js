'use strict';
// Minimal, dependency-free JSON-RPC 2.0 MCP stdio implementation.
const readline=require('node:readline');
const {generate,exportIdea,configuration}=require('./operations');
const VERSION='2025-03-26';
const TOOLS=[
 {name:'loopforge_generate',description:'Generate MIDI music ideas as bass, chord and melody notes. Supports offline or API model providers configured with local environment variables.',inputSchema:{type:'object',properties:{provider:{type:'string',enum:['offline','openai','anthropic','openrouter','ollama','codex-cli','claude-cli']},key:{type:'string',description:'Key: C, C#, D ... B'},mode:{type:'string',enum:['minor','major','dorian','phrygian']},style:{type:'string',enum:['trap','boom-bap','r&b','ambient','nu-metal']},bpm:{type:'integer',minimum:40,maximum:240},bars:{type:'integer',enum:[4,8]},count:{type:'integer',minimum:1,maximum:8},description:{type:'string'},model:{type:'string'},seed:{type:'integer'}},additionalProperties:false}},
 {name:'loopforge_export_midi',description:'Validate and export a musical idea to a local .mid file. Idea contains metadata and three layers (bass,chords,melody) with MIDI note events.',inputSchema:{type:'object',properties:{idea:{type:'object'},basename:{type:'string'}},required:['idea'],additionalProperties:false}},
 {name:'loopforge_providers',description:'Check which AI providers are configured; does not reveal credentials.',inputSchema:{type:'object',properties:{}}}
];
function send(obj){process.stdout.write(JSON.stringify(obj)+'\n')}
function success(id,result){return {jsonrpc:'2.0',id,result}}
function error(id,code,message){return {jsonrpc:'2.0',id,error:{code,message}}}
async function dispatch(message){
 const {id,method,params={}}=message;
 if(method==='initialize')return success(id,{protocolVersion:params.protocolVersion||VERSION,capabilities:{tools:{listChanged:false}},serverInfo:{name:'loopforge-midi',version:'0.1.0'}});
 if(method==='notifications/initialized'||method==='notifications/cancelled')return null;
 if(method==='ping')return success(id,{});
 if(method==='tools/list')return success(id,{tools:TOOLS});
 if(method==='tools/call'){
  try{
   let result;
   if(params.name==='loopforge_generate')result=await generate(params.arguments||{});
   else if(params.name==='loopforge_export_midi')result=exportIdea(params.arguments?.idea,params.arguments?.basename);
   else if(params.name==='loopforge_providers')result=configuration();
   else return error(id,-32602,'Unknown tool');
   return success(id,{content:[{type:'text',text:JSON.stringify(result)}],isError:false});
  }catch(e){return success(id,{content:[{type:'text',text:e.message}],isError:true});}
 }
 if(id===undefined)return null;
 return error(id,-32601,'Method not found');
}
if(require.main===module){
 const lines=readline.createInterface({input:process.stdin,crlfDelay:Infinity});
 lines.on('line',async line=>{
  let msg;try{msg=JSON.parse(line)}catch{send(error(null,-32700,'Parse error'));return}
  if(Array.isArray(msg)){send(error(null,-32600,'Batch requests not supported'));return}
  try{const reply=await dispatch(msg);if(reply)send(reply);}catch(e){send(error(msg.id??null,-32603,e.message))}
 });
}
module.exports={dispatch,TOOLS};
