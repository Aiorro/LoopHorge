'use strict';
const test=require('node:test'),assert=require('node:assert/strict'),http=require('node:http');
const {listener}=require('../src/server');
test('provider probe succeeds offline and reports missing authorization without exposing credentials',async t=>{
 const server=http.createServer(listener);await new Promise(r=>server.listen(0,'127.0.0.1',r));t.after(()=>{server.closeAllConnections();server.close()});
 const url=`http://127.0.0.1:${server.address().port}/api/probe`;const headers={'Content-Type':'application/json',Host:'127.0.0.1:'+(process.env.LOOPFORGE_PORT||4177)};
 const request=(url,opts)=>new Promise((resolve,reject)=>{const req=http.request(url,opts,res=>{let data='';res.on('data',b=>data+=b);res.on('end',()=>resolve({status:res.statusCode,json:async()=>JSON.parse(data)}))});req.on('error',reject);req.end(opts.body)});
 let response=await request(url,{method:'POST',headers,body:'{"provider":"offline","model":""}'});assert.equal(response.status,200);const result=await response.json();assert.equal(result.ok,true);assert.equal(result.provider,'offline');assert.equal(result.ideas,undefined);
 const previous=process.env.OPENAI_API_KEY;delete process.env.OPENAI_API_KEY;t.after(()=>{if(previous===undefined)delete process.env.OPENAI_API_KEY;else process.env.OPENAI_API_KEY=previous});
 response=await request(url,{method:'POST',headers,body:'{"provider":"openai"}'});assert.equal(response.status,400);assert.match((await response.json()).error,/OPENAI_API_KEY/);
 response=await request(url,{method:'POST',headers,body:'{"provider":"unknown"}'});assert.equal(response.status,422);
 response=await request(url,{method:'POST',headers,body:'{"provider":"ollama","model":"bad;command"}'});assert.equal(response.status,422);
});
