'use strict';
const http=require('node:http');const {spawn}=require('node:child_process');const path=require('node:path');
const executable=path.resolve(process.argv[2]);
const idea=require('../plugin/tests/idea.json');
idea.metadata.timeSignature={numerator:4,denominator:4};
for(const [layer,notes] of Object.entries(idea.layers))for(const note of notes)note.layer=layer;
const rawReply=(text)=>(_req,res)=>{res.writeHead(200,{'Content-Length':Buffer.byteLength(text)});res.end(text)};
const envelope=ideas=>({protocolVersion:1,provider:'ollama',model:'test',sources:ideas.map(()=> 'ai'),ideas});
const reply=body=>rawReply(JSON.stringify({...(body.ideas?envelope(body.ideas):{}),...body}));
(async()=>{
 for(const [mode,message,handler] of [
  ['--failure','Unsupported bridge protocol',reply({protocolVersion:2,ideas:[idea]})],
  ['--failure','Unsupported bridge protocol',reply({protocolVersion:undefined,ideas:[idea]})],
  ['--failure','Invalid composition',reply({protocolVersion:1,ideas:[idea,{...idea,layers:{...idea.layers,melody:[{pitch:200,start:0,duration:1,velocity:100,layer:'melody'}]}}]})],
  ['--failure','Invalid composition',reply({protocolVersion:1,ideas:[{...idea,metadata:{...idea.metadata,timeSignature:{numerator:3,denominator:4}}}]})],
  ['--failure','length mismatch',(_req,res)=>{const body=JSON.stringify(envelope([idea]));res.writeHead(200,{'Content-Length':Buffer.byteLength(body)+10});res.end(body)}],
  ['--timeout','timed out',(_req,res)=>{res.writeHead(200,{'Content-Length':'100'});res.flushHeaders()}],
  ['--failure','Invalid bridge response',reply({ideas:[idea],model:undefined})],
  ['--failure','Invalid bridge response',reply({ideas:[idea],provider:true})],
  ['--failure','Invalid bridge response',reply({ideas:[idea],sources:[]})],
  ['--probe-failure','Invalid bridge response',reply({protocolVersion:1,provider:'ollama',model:'test',message:'ready',ok:'false'})],
  ['--failure','Invalid bridge JSON',rawReply(JSON.stringify(envelope([idea]))+'garbage')],
  ['--failure','Invalid bridge JSON',rawReply(JSON.stringify(envelope([idea]))+' {}')],
  ['--reconnect','',(()=>{let calls=0;return (req,res)=>{if(calls++===0)return req.socket.destroy();reply(envelope([idea]))(req,res)}})()],
  ['--failure','rejected',(_req,res)=>{res.writeHead(429);res.end('private upstream details')}],
  ['--failure','Invalid bridge response',reply({protocolVersion:1,ideas:[]})],
  ['--failure','exceeded',(_req,res)=>{const body='x'.repeat(4*1024*1024+16384);res.writeHead(200,{'Content-Length':String(body.length)});res.end(body)}],
  ['--user-cancel','',(_req,res)=>{res.writeHead(200,{'Content-Length':'100'});res.flushHeaders()}],
  ['--cancel','',(_req,res)=>{res.writeHead(200,{'Content-Length':'100'});res.flushHeaders()}]
 ]){
  const server=http.createServer(handler);server.on('clientError',(_error,socket)=>socket.destroy());
  await new Promise((resolve,reject)=>{server.once('error',reject);server.listen(4177,'127.0.0.1',resolve)});
  try{await new Promise((resolve,reject)=>{const child=spawn(executable,[path.resolve('plugin/tests/idea.json'),mode,message],{stdio:'inherit'});const timer=setTimeout(()=>{child.kill();reject(Error('Native failure test timed out'))},15000);child.on('error',reject);child.on('exit',code=>{clearTimeout(timer);code===0?resolve():reject(Error('Native failure test exit '+code));});});}
  finally{server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
 }
 console.log('Native v1 versions, envelope/JSON/framing, timeout, reconnect, cancellation, size and HTTP error recovery passed');
})().catch(error=>{console.error(error.message);process.exitCode=1});
