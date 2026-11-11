'use strict';
const test=require('node:test');const assert=require('node:assert/strict');
const fs=require('node:fs');const path=require('node:path');
const {generateOne,generateEight,createMidi,transposePattern,STYLES}=require('../src/core');
const {normalizeResponse,normalizeOptions,validateIdea}=require('../src/schema');
const {generate,exportIdea}=require('../src/operations');
const {dispatch}=require('../src/mcp-server');
const opts={key:'D',mode:'minor',style:'nu-metal',bpm:125,bars:8};
test('eight complete unique ideas, deterministic seeds',()=>{
 const a=generateEight(opts,1234),b=generateEight(opts,1234);
 assert.deepEqual(a,b);assert.equal(a.length,8);
 assert.equal(new Set(a.map(x=>JSON.stringify(x.layers))).size,8);
 for(const idea of a){const valid=validateIdea(idea,opts);assert.ok(Object.values(valid.layers).every(x=>x.length>0))}
});
test('supports all styles and lengths',()=>{for(const style of Object.keys(STYLES))for(const bars of [4,8]){const x=generateOne({...opts,style,bars},9);assert.equal(x.metadata.bars,bars);assert.ok(x.layers.melody.length>0)}});
test('MIDI header type 1 with four tracks, correct PPQ',()=>{
 const bytes=createMidi(generateOne(opts,12));const b=Buffer.from(bytes);
 assert.equal(b.subarray(0,4).toString('ascii'),'MThd');assert.equal(b.readUInt16BE(8),1);assert.equal(b.readUInt16BE(10),4);assert.equal(b.readUInt16BE(12),480);
 assert.equal(b.toString('ascii').match(/MTrk/g).length,4);
});
test('correctly clamps transposed pattern',()=>{const x=generateOne(opts,1);const y=transposePattern(x,100);assert.ok(y.layers.melody.every(n=>n.pitch<=127))});
test('rejects malformed AI output',()=>{
 const obj=generateOne(opts,9);obj.layers.bass[0].start=-1;assert.throws(()=>validateIdea(obj,opts),/Out-of-range/);
 assert.throws(()=>normalizeResponse('sorry, no JSON',opts),/No JSON/);
});
test('normalize limited AI response and parse fenced JSON',()=>{
 const idea=generateOne(opts,45);const json='```json\n'+JSON.stringify({ideas:[idea]})+'\n```';
 assert.equal(normalizeResponse(json,opts,8).length,1);
});
test('offline orchestration returns 8 patterns',async()=>{
 const data=await generate({...opts,provider:'offline',seed:34});assert.equal(data.ideas.length,8);assert.ok(data.sources.every(x=>x==='offline'));
});
test('MCP tool discovery and calling',async()=>{
 let response=await dispatch({id:1,method:'initialize',params:{protocolVersion:'2025-03-26'}});assert.equal(response.result.serverInfo.name,'loopforge-midi');
 response=await dispatch({id:2,method:'tools/list'});assert.ok(response.result.tools.some(t=>t.name==='loopforge_generate'));
 response=await dispatch({id:3,method:'tools/call',params:{name:'loopforge_generate',arguments:{provider:'offline',count:1}}});
 assert.equal(JSON.parse(response.result.content[0].text).ideas.length,1);
});
test('MCP exports file with validated idea',async()=>{
 const idea=generateOne(opts,222);const response=await dispatch({id:4,method:'tools/call',params:{name:'loopforge_export_midi',arguments:{idea,basename:'test-export'}}});
 const data=JSON.parse(response.result.content[0].text);assert.equal(data.filename,'test-export.mid');assert.equal(fs.readFileSync(data.path).subarray(0,4).toString(),'MThd');fs.unlinkSync(data.path);
});
test('OpenAI provider adapter, mocked network only',async()=>{
 const before=process.env.OPENAI_API_KEY,oldFetch=global.fetch;
 process.env.OPENAI_API_KEY='temporary-test-key';
 global.fetch=async(url,config)=>{
  assert.equal(url,'https://api.openai.com/v1/chat/completions');assert.equal(config.headers.Authorization,'Bearer temporary-test-key');
  return {ok:true,text:async()=>JSON.stringify({choices:[{message:{content:JSON.stringify({ideas:[generateOne(opts,77)]})}}]})};
 };
 try{const result=await generate({...opts,provider:'openai',count:2,model:'gpt-5.2'});
 assert.equal(result.sources[0],'ai');assert.equal(result.sources[1],'offline-fallback');assert.equal(result.ideas.length,2);
 }finally{global.fetch=oldFetch;if(before===undefined)delete process.env.OPENAI_API_KEY;else process.env.OPENAI_API_KEY=before}
});
test('provider errors do not expose upstream response content through MCP',async()=>{
 const before=process.env.OPENAI_API_KEY, oldFetch=global.fetch;
 process.env.OPENAI_API_KEY='temporary-test-key';
 global.fetch=async()=>({ok:false,status:401,text:async()=>'{"error":"secret temporary-test-key and private prompt"}'});
 try{
  const response=await dispatch({id:5,method:'tools/call',params:{name:'loopforge_generate',arguments:{provider:'openai',count:1}}});
  assert.equal(response.result.isError,true);
  const message=response.result.content[0].text;
  assert.match(message,/openai returned HTTP 401/);
  assert.doesNotMatch(message,/temporary-test-key|private prompt/);
 }finally{global.fetch=oldFetch;if(before===undefined)delete process.env.OPENAI_API_KEY;else process.env.OPENAI_API_KEY=before}
});
test('normalized notes remain within the composition after rounding',()=>{
 const idea=generateOne({...opts,bars:4},7);
 idea.layers.bass=[{pitch:36,start:15.9999,duration:0.00001,velocity:100}];
 const note=validateIdea(idea,{...opts,bars:4}).layers.bass[0];
 assert.ok(note.start<16);
 assert.ok(note.duration>0);
 assert.ok(note.start+note.duration<=16);
});
test('CLI errors do not expose stderr through MCP',{skip:process.platform==='win32'},async()=>{
 const os=require('node:os');const dir=fs.mkdtempSync(path.join(os.tmpdir(),'loopforge-cli-test-'));const previous=process.env.PATH;
 fs.writeFileSync(path.join(dir,'codex'),'#!/bin/sh\necho "private credential" >&2\nexit 2\n',{mode:0o755});
 process.env.PATH=dir+path.delimiter+(previous||'');
 try{
  const response=await dispatch({id:6,method:'tools/call',params:{name:'loopforge_generate',arguments:{provider:'codex-cli',count:1}}});
  assert.equal(response.result.isError,true);
  assert.match(response.result.content[0].text,/exited with code 2/);
  assert.doesNotMatch(response.result.content[0].text,/private credential/);
 }finally{if(previous===undefined)delete process.env.PATH;else process.env.PATH=previous;fs.rmSync(dir,{recursive:true,force:true})}
});
