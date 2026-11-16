'use strict';
const test=require('node:test');const assert=require('node:assert/strict');
const {spawn}=require('node:child_process');const http=require('node:http');
const {listener}=require('../src/server');const {generateOne,createMidi}=require('../src/core');const {validateIdea}=require('../src/schema');
async function mcp(input){return new Promise((resolve,reject)=>{
 const child=spawn(process.execPath,['src/mcp-server.js'],{stdio:['pipe','pipe','pipe']});let output='',errors='';
 const timer=setTimeout(()=>{child.kill();reject(Error('MCP child timed out'));},10000);
 child.stdout.on('data',b=>output+=b);child.stderr.on('data',b=>errors+=b);child.on('error',reject);
 child.on('close',code=>{clearTimeout(timer);if(code!==0)return reject(Error('MCP exit '+code+': '+errors));resolve(output.trim().split('\n').filter(Boolean).map(JSON.parse));});child.stdin.on('error',()=>{});child.stdin.end(input);
});}
test('MCP survives malformed values and oversized lines, then answers ping',async()=>{
 const replies=await mcp('null\n'+JSON.stringify({jsonrpc:'2.0',id:1,method:'initialize',params:null})+'\n'+ 'x'.repeat(1024*1024+1)+'\n'+JSON.stringify({jsonrpc:'2.0',id:2,method:'ping'})+'\n');
 assert.equal(replies.filter(x=>x.error).length,3);assert.ok(replies.some(x=>x.id===2&&x.result));
});
test('HTTP oversize returns 413 and server remains available under concurrent load',async t=>{
 const server=http.createServer(listener);await new Promise(r=>server.listen(0,'127.0.0.1',r));t.after(()=>new Promise(r=>{server.closeAllConnections();server.close(r)}));
 const port=server.address().port;const headers={Host:'127.0.0.1:'+(process.env.LOOPFORGE_PORT||4177),'Content-Type':'application/json'};
 const request=(body)=>new Promise((resolve,reject)=>{const req=http.request({host:'127.0.0.1',port,path:'/api/generate',method:'POST',headers},res=>{let data='';res.on('data',b=>data+=b);res.on('end',()=>resolve({status:res.statusCode,data}));});req.on('error',reject);req.end(body);});
 assert.equal((await request(JSON.stringify({description:'x'.repeat(600000)}))).status,413);
 for(let batch=0;batch<5;batch++){const results=await Promise.all(Array.from({length:20},(_,seed)=>request(JSON.stringify({provider:'offline',count:8,seed}))));for(const r of results){assert.equal(r.status,200);assert.equal(JSON.parse(r.data).ideas.length,8);}}
});
test('1000 seeded compositions validate and serialize without nonfinite fields',()=>{
 for(let seed=0;seed<1000;seed++){const opts={key:'D',mode:'minor',style:'nu-metal',bpm:40+seed%201,bars:seed%2?4:8};const idea=validateIdea(generateOne(opts,seed),opts);const bytes=Buffer.from(createMidi(idea));assert.equal(bytes.subarray(0,4).toString(),'MThd');assert.equal(bytes.readUInt16BE(10),4);}
});
test('MCP bounds concurrent generation requests and keeps protocol alive',async()=>{
 const lines=Array.from({length:100},(_,id)=>JSON.stringify({jsonrpc:'2.0',id,method:'tools/call',params:{name:'loopforge_generate',arguments:{provider:'offline',count:1}}}));
 const replies=await mcp(lines.join('\n')+'\n');assert.equal(replies.length,100);assert.ok(replies.some(r=>r.result));assert.ok(replies.some(r=>r.error?.code===-32000));
});
test('provider cancellation and rate-limit failures are sanitized',async()=>{
 const {requestAI}=require('../src/ai');const opts={key:'D',mode:'minor',style:'nu-metal',bpm:100,bars:4};
 const controller=new AbortController();controller.abort();let called=false;
 await assert.rejects(()=>requestAI('ollama',opts,{signal:controller.signal,transport:async()=>{called=true;throw new Error('must not run')}}),/cancelled/);assert.equal(called,false);
 let cancelled=false;
 const transport=async()=>new Response(new ReadableStream({cancel(){cancelled=true;}}),{status:429});
 await assert.rejects(()=>requestAI('ollama',opts,{transport}),/HTTP 429/);assert.equal(cancelled,true);
});
// Independent byte reader: validates track framing, VLQs, running status and note payloads.
function readMidi(bytes){
 assert.equal(bytes.toString('ascii',0,4),'MThd');const tracks=bytes.readUInt16BE(10);let offset=8+bytes.readUInt32BE(4);const notes=[];
 for(let track=0;track<tracks;track++){
  assert.equal(bytes.toString('ascii',offset,offset+4),'MTrk');const end=offset+8+bytes.readUInt32BE(offset+4);offset+=8;let tick=0,running=0,ended=false;
  const take=()=>{assert.ok(offset<end);return bytes[offset++];};
  const vlq=()=>{let value=0,b=0;for(let n=0;n<4;n++){b=take();value=(value<<7)|(b&127);if(!(b&128))return value;}throw Error('Invalid VLQ');};
  while(offset<end){tick+=vlq();let status=take();if(status<128){assert.ok(running>=128);offset--;status=running;}else if(status<240)running=status;
   if(status===255){const type=take(),size=vlq();assert.ok(offset+size<=end);offset+=size;if(type===47){assert.equal(size,0);ended=true;}continue;}
   assert.ok(status<240);const type=status>>4,a=take(),b=(type===12||type===13)?0:take();assert.ok(a<128&&b<128);
   if(type===8||type===9)notes.push([tick,status&15,a,type===8||b===0?0:b]);
  }
  assert.equal(offset,end);assert.equal(ended,true);
 }
 assert.equal(offset,bytes.length);return notes;
}
test('overlapping same-pitch notes retrigger with counted lifetimes',()=>{
 const idea={metadata:{key:'C',mode:'minor',style:'trap',bpm:100,bars:4},layers:{bass:[{pitch:36,start:0,duration:2,velocity:100},{pitch:36,start:1,duration:2,velocity:90}],chords:[],melody:[]}};
 const notes=readMidi(Buffer.from(createMidi(idea))).filter(n=>n[1]===0);
 assert.deepEqual(notes.map(n=>n.join(',')).sort(),['0,0,36,100','480,0,36,0','480,0,36,90','1440,0,36,0'].sort());
});
test('simultaneous same-pitch unisons share one onset with the last velocity',()=>{
 const idea={metadata:{key:'C',mode:'minor',style:'trap',bpm:100,bars:4},layers:{bass:[{pitch:36,start:0,duration:2,velocity:100},{pitch:36,start:0,duration:2,velocity:90}],chords:[],melody:[]}};
 const notes=readMidi(Buffer.from(createMidi(idea))).filter(n=>n[1]===0);
 assert.deepEqual(notes.map(n=>n.join(',')).sort(),['0,0,36,90','960,0,36,0'].sort());
});
// Independent reference of the counted-lifetime note semantics (mirrors native engine):
// overlapping same-pitch notes retrigger, simultaneous unisons share one onset.
function referenceNotes(idea){
 const PPQ=480,duration=idea.metadata.bars*4*PPQ,out=[];
 ['bass','chords','melody'].forEach((name,ch)=>{
  const edges=[];
  for(const n of idea.layers[name]){
   const start=Math.max(0,Math.min(duration-1,Math.round(n.start*PPQ)));
   const end=Math.max(start+1,Math.min(duration,Math.round((n.start+n.duration)*PPQ)));
   const pitch=Math.max(0,Math.min(127,Math.round(n.pitch)));
   const vel=Math.max(1,Math.min(127,Math.round(n.velocity)));
   edges.push({tick:start,pitch,vel,on:true});edges.push({tick:end,pitch,vel:0,on:false});
  }
  edges.sort((a,b)=>a.tick-b.tick||(a.on===b.on?0:a.on?1:-1));
  const held=new Array(128).fill(0),onsetTick=new Array(128).fill(-1),onsetIdx=new Array(128).fill(-1);
  for(const e of edges){
   const p=e.pitch;
   if(e.on){
    if(onsetTick[p]===e.tick)out[onsetIdx[p]][3]=e.vel;
    else{if(held[p])out.push([e.tick,ch,p,0]);onsetTick[p]=e.tick;onsetIdx[p]=out.length;out.push([e.tick,ch,p,e.vel]);}
    ++held[p];
   }else if(--held[p]===0)out.push([e.tick,ch,p,0]);
  }
 });
 return out;
}
test('independent MIDI decoder preserves notes, channels and timing for 200 compositions',()=>{
 for(let seed=0;seed<200;seed++){
  const idea=generateOne({key:'D',mode:'minor',style:'nu-metal',bpm:100,bars:seed%2?4:8},seed);
  const canonical=list=>list.map(n=>n.join(',')).sort();assert.deepEqual(canonical(readMidi(Buffer.from(createMidi(idea)))),canonical(referenceNotes(idea)));
 }
});
test('malformed provider envelopes return actionable errors without crashing',async()=>{
 const {requestAI}=require('../src/ai');const opts={key:'C',mode:'minor',style:'trap',bpm:100,bars:4};
 for(const body of ['null','[]','{}','{"choices":[{"message":{"content":[]}}]}']){
  await assert.rejects(()=>requestAI('ollama',opts,{transport:async()=>new Response(body)}),/Provider (sent invalid JSON|response contained no text)/);
 }
});
test('HTTP preserves a Unicode AI prompt split across network chunks',async t=>{
 const server=http.createServer(listener);await new Promise(r=>server.listen(0,'127.0.0.1',r));t.after(()=>new Promise(r=>{server.closeAllConnections();server.close(r)}));
 const previous=global.fetch;let received='';global.fetch=async(_url,options)=>{received=JSON.parse(options.body).messages[1].content;return new Response(JSON.stringify({choices:[{message:{content:JSON.stringify({ideas:[generateOne({key:'C',mode:'minor',style:'trap',bpm:100,bars:4},1)]})}}]}));};
 const prefix='{"provider":"ollama","count":1,"description":"';const body=Buffer.from(prefix+'Привет 🎵"}');const split=Buffer.byteLength(prefix)+1;
 try{await new Promise((resolve,reject)=>{const req=http.request({host:'127.0.0.1',port:server.address().port,path:'/api/generate',method:'POST',headers:{Host:'127.0.0.1:'+(process.env.LOOPFORGE_PORT||4177),'Content-Type':'application/json'}},res=>{res.resume();res.on('end',()=>{try{assert.equal(res.statusCode,200);resolve()}catch(e){reject(e)}});});req.on('error',reject);req.write(body.subarray(0,split));setTimeout(()=>req.end(body.subarray(split)),20);});assert.ok(received.includes('Привет 🎵'));}
 finally{global.fetch=previous;}
});
test('provider cancellation during response streaming is reported as cancellation',async()=>{
 const {requestAI}=require('../src/ai');const controller=new AbortController();
 await assert.rejects(requestAI('ollama',{key:'D',mode:'minor',style:'trap',bpm:100,bars:4},{signal:controller.signal,transport:async()=>new Response(new ReadableStream({start(stream){controller.signal.addEventListener('abort',()=>stream.error(new Error('private socket details')));setTimeout(()=>controller.abort(),5)}}))}),/Generation cancelled/);
});
