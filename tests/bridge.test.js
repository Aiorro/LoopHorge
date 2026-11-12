'use strict';
const test=require('node:test'),assert=require('node:assert/strict'),http=require('node:http');
const {listener}=require('../src/server');
async function server(t){const server=http.createServer(listener);await new Promise(r=>server.listen(0,'127.0.0.1',r));t.after(()=>{server.closeAllConnections();server.close()});
 return (path,body)=>new Promise((resolve,reject)=>{const req=http.request({host:'127.0.0.1',port:server.address().port,path,method:'POST',headers:{Host:'127.0.0.1:'+(process.env.LOOPFORGE_PORT||4177),'Content-Type':'application/json'}},res=>{const chunks=[];res.on('data',b=>chunks.push(b));res.on('end',()=>{const raw=Buffer.concat(chunks);resolve({status:res.statusCode,length:res.headers['content-length'],raw,body:JSON.parse(raw)})})});req.on('error',reject);req.end(typeof body==='string'?body:JSON.stringify(body))});}
test('v1 generation returns versioned bounded compositions with explicit meter; legacy remains available',async t=>{
 const request=await server(t);const response=await request('/api/v1/generate',{provider:'offline',count:2,seed:42,bars:4,key:'D',mode:'minor',bpm:105});
 assert.equal(response.status,200);assert.equal(response.body.protocolVersion,1);assert.equal(Number(response.length),response.raw.length);assert.equal(response.body.ideas.length,2);
 for(const idea of response.body.ideas){assert.deepEqual(idea.metadata.timeSignature,{numerator:4,denominator:4});let total=0;for(const notes of Object.values(idea.layers)){total+=notes.length;for(const n of notes){assert.ok(Number.isInteger(n.pitch)&&n.pitch>=0&&n.pitch<=127);assert.ok(n.start>=0&&n.duration>0&&n.start+n.duration<=16)}}assert.ok(total>0&&total<=1600)}
 assert.equal((await request('/api/v1/generate',{provider:'offline',count:2,seed:42,bars:4,key:'D',mode:'minor',bpm:105})).raw.toString(),response.raw.toString());
 const legacy=await request('/api/generate',{provider:'offline',count:1});assert.equal(legacy.status,200);assert.equal(legacy.body.protocolVersion,undefined);assert.equal(legacy.body.ideas.length,1);
});
test('v1 rejects invalid wire types and unsupported versions with stable safe errors',async t=>{
 const request=await server(t);
 for(const body of [null,[],{provider:'unknown'},{provider:'offline',bars:5},{provider:'offline',bpm:'100'},{provider:'offline',count:9},{provider:'offline',model:'bad;command'},{provider:'offline',description:'x'.repeat(601)},{provider:'offline',timeSignature:{numerator:3,denominator:4}},{provider:'offline',extra:true}]){
  const response=await request('/api/v1/generate',body);assert.equal(response.status,422);assert.equal(response.body.protocolVersion,1);assert.equal(response.body.error.code,'INVALID_REQUEST');assert.equal(typeof response.body.error.message,'string');assert.ok(response.body.error.message.length<513);
 }
 const malformed=await request('/api/v1/generate','{');assert.equal(malformed.status,422);assert.equal(malformed.body.error.code,'INVALID_REQUEST');
 const unsupported=await request('/api/v2/generate',{});assert.equal(unsupported.status,426);assert.equal(unsupported.body.error.code,'UNSUPPORTED_VERSION');
 const oversize=await request('/api/v1/generate',{description:'x'.repeat(600000)});assert.equal(oversize.status,413);assert.equal(oversize.body.error.code,'REQUEST_TOO_LARGE');
 const recovery=await request('/api/v1/generate',{provider:'offline',count:1});assert.equal(recovery.status,200);
});
test('v1 probe has a versioned result and actionable authorization diagnostics',async t=>{
 const request=await server(t);const result=await request('/api/v1/probe',{provider:'offline'});assert.equal(result.status,200);assert.equal(result.body.protocolVersion,1);assert.equal(result.body.ok,true);assert.equal(result.body.ideas,undefined);
 const key=process.env.OPENAI_API_KEY;delete process.env.OPENAI_API_KEY;t.after(()=>{if(key===undefined)delete process.env.OPENAI_API_KEY;else process.env.OPENAI_API_KEY=key});
 const error=await request('/api/v1/probe',{provider:'openai'});assert.equal(error.status,400);assert.equal(error.body.error.code,'AUTHORIZATION_REQUIRED');assert.match(error.body.error.message,/OPENAI_API_KEY/);
});
test('v1 cancellation prevents work and malformed upstream JSON never leaks through errors',async t=>{
 const {generateV1}=require('../src/bridge');await assert.rejects(generateV1({provider:'offline'},{signal:AbortSignal.abort()}),error=>error.code==='GENERATION_CANCELLED');
 const request=await server(t);const original=global.fetch;global.fetch=async()=>new Response('PRIVATE_UPSTREAM_SECRET');t.after(()=>{global.fetch=original});
 const response=await request('/api/v1/generate',{provider:'ollama'});assert.equal(response.status,502);assert.equal(response.body.error.code,'INVALID_RESPONSE');assert.doesNotMatch(response.raw.toString(),/PRIVATE_UPSTREAM_SECRET/);
});
for(const phase of ['headers','body'])test(`v1 preserves a provider ${phase} deadline as REQUEST_TIMEOUT`,async t=>{
 const request=await server(t),original=global.fetch;global.fetch=async()=>{const error=new DOMException('PRIVATE_TIMEOUT_DETAIL','TimeoutError');if(phase==='headers')throw error;return new Response(new ReadableStream({start(stream){stream.error(error)}}))};t.after(()=>{global.fetch=original});
 const response=await request('/api/v1/generate',{provider:'ollama'});assert.equal(response.status,504);assert.equal(response.body.error.code,'REQUEST_TIMEOUT');assert.doesNotMatch(response.raw.toString(),/PRIVATE_TIMEOUT_DETAIL/);
});
