'use strict';
// Minimal, dependency-free JSON-RPC 2.0 MCP stdio implementation.
const {generate,exportIdea,configuration}=require('./operations');
const VERSION='2025-03-26';
const TOOLS=[
 {name:'loopforge_generate',description:'Generate MIDI music ideas as bass, chord and melody notes. Supports offline or API model providers configured with local environment variables.',inputSchema:{type:'object',properties:{provider:{type:'string',enum:['offline','openai','anthropic','openrouter','ollama','codex-cli','claude-cli']},key:{type:'string',description:'Key: C, C#, D ... B'},mode:{type:'string',enum:['minor','major','dorian','phrygian']},style:{type:'string',enum:['trap','boom-bap','r&b','ambient','nu-metal']},bpm:{type:'integer',minimum:40,maximum:240},bars:{type:'integer',enum:[4,8]},count:{type:'integer',minimum:1,maximum:8},description:{type:'string'},model:{type:'string'},seed:{type:'integer'}},additionalProperties:false}},
 {name:'loopforge_export_midi',description:'Validate and export a musical idea to a local .mid file. Idea contains metadata and three layers (bass,chords,melody) with MIDI note events.',inputSchema:{type:'object',properties:{idea:{type:'object'},basename:{type:'string'}},required:['idea'],additionalProperties:false}},
 {name:'loopforge_providers',description:'Check which AI providers are configured; does not reveal credentials.',inputSchema:{type:'object',properties:{}}}
];
function success(id,result){return {jsonrpc:'2.0',id,result}}
function error(id,code,message){return {jsonrpc:'2.0',id,error:{code,message}}}
function validId(id){return typeof id==='string'||Number.isInteger(id)}
async function dispatch(message,{signal}={}){
 if(!message||typeof message!=='object'||Array.isArray(message)||typeof message.method!=='string')return error(null,-32600,'Invalid request');
 const {id,method,params={}}=message;
 if(id===undefined)return null;
 if(!validId(id))return error(null,-32600,'Invalid request ID');
 if(!params||typeof params!=='object'||Array.isArray(params))return error(id??null,-32602,'Invalid params');
 if(method==='initialize')return success(id,{protocolVersion:params.protocolVersion||VERSION,capabilities:{tools:{listChanged:false}},serverInfo:{name:'loopforge-midi',version:'0.1.0'}});
 if(method.startsWith('notifications/'))return error(id,-32600,'Notifications must not have an ID');
 if(method==='ping')return success(id,{});
 if(method==='tools/list')return success(id,{tools:TOOLS});
 if(method==='tools/call'){
  try{
   if(signal?.aborted)throw new Error('Generation cancelled');
   let result;
   if(params.name==='loopforge_generate')result=await generate(params.arguments||{},{signal});
   else if(params.name==='loopforge_export_midi')result=exportIdea(params.arguments?.idea,params.arguments?.basename);
   else if(params.name==='loopforge_providers')result=await configuration();
   else return error(id,-32602,'Unknown tool');
   return success(id,{content:[{type:'text',text:JSON.stringify(result)}],isError:false});
  }catch(e){return success(id,{content:[{type:'text',text:e.message}],isError:true});}
 }
 return error(id,-32601,'Method not found');
}
if(require.main===module){
 const LIMIT=1024*1024,active=new Map();let parts=[],size=0,dropping=false,stopping=false,outputDeadline,outputFailed=false;
 function send(obj){
  if(outputFailed||process.stdout.destroyed||process.stdout.writableEnded)return;
  if(!process.stdout.write(JSON.stringify(obj)+'\n')){
   process.stdin.pause();
   // Paused stdin may hide EOF, so a permanently blocked reply sink must also close the session.
   if(!stopping&&!outputDeadline)outputDeadline=setTimeout(()=>{outputFailed=true;shutdown(1)},5000);
  }
 }
 process.stdout.on('drain',()=>{clearTimeout(outputDeadline);outputDeadline=undefined;if(!stopping)process.stdin.resume()});
 function shutdown(code=0){
  if(stopping)return;stopping=true;process.stdin.pause();clearTimeout(outputDeadline);
  const pending=[...active.values()];for(const request of pending)request.controller.abort();
  // Covers provider/Windows job cleanup and a blocked stdout flush; hard kill/setsid are outside this session boundary.
  const deadline=setTimeout(()=>process.exit(1),5000);
  Promise.allSettled(pending.map(request=>request.task)).then(()=>{
   process.stdin.destroy();
   if(outputFailed||process.stdout.destroyed){clearTimeout(deadline);process.exit(code);return;}
   process.stdout.end(()=>{clearTimeout(deadline);process.exit(code)});
  });
 }
 process.on('SIGTERM',()=>shutdown());process.on('SIGINT',()=>shutdown());
 process.stdin.on('error',()=>shutdown(1));process.stdout.on('error',()=>{outputFailed=true;shutdown(1)});
 function reply(task,id){task.then(result=>{if(result)send(result)},()=>send(error(id,-32603,'Request failed')))}
 function line(bytes){
  if(stopping)return;
  let msg;try{msg=JSON.parse(bytes.toString('utf8'))}catch{send(error(null,-32700,'Parse error'));return;}
  if(!msg||typeof msg!=='object'||Array.isArray(msg)||typeof msg.method!=='string'){reply(dispatch(msg),null);return;}
  if(msg.id===undefined){
   if(msg.method==='notifications/cancelled'&&msg.params&&validId(msg.params.requestId))active.get(msg.params.requestId)?.controller.abort();
   return;
  }
  if(!validId(msg.id)){send(error(null,-32600,'Invalid request ID'));return;}
  if(active.has(msg.id)){send(error(null,-32600,'Duplicate active request ID'));return;}
  if(msg.method!=='tools/call'){reply(dispatch(msg),msg.id);return;}
  if(active.size>=8){send(error(msg.id,-32000,'Server busy; retry later'));return;}
  const request={controller:new AbortController()};active.set(msg.id,request);
  request.task=Promise.resolve().then(()=>dispatch(msg,{signal:request.controller.signal})).finally(()=>active.delete(msg.id));
  reply(request.task,msg.id);
 }
 process.stdin.on('data',chunk=>{
  let start=0;
  while(start<chunk.length){const newline=chunk.indexOf(10,start);const end=newline<0?chunk.length:newline;const part=chunk.subarray(start,end);
   if(!dropping){size+=part.length;if(size>LIMIT){dropping=true;parts=[];send(error(null,-32600,'Message exceeded 1 MiB limit'));}else parts.push(part);}
   if(newline>=0){if(!dropping)line(Buffer.concat(parts,size));parts=[];size=0;dropping=false;}
   start=end+1;
  }
 });
 process.stdin.on('end',()=>{if(size&&!dropping)line(Buffer.concat(parts,size));shutdown()});
}
module.exports={dispatch,TOOLS};
