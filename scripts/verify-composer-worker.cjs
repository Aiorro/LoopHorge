'use strict';
// Real loopback responses are held until the C++ test has committed its newer state.
const {spawn}=require('node:child_process');
const {mkdtempSync,writeFileSync,existsSync,rmSync}=require('node:fs');
const {tmpdir}=require('node:os');const path=require('node:path');
const executable=process.argv[2];if(!executable)throw Error('Pass composer_processor_test executable');
const root=mkdtempSync(path.join(tmpdir(),'loopforge-worker-'));
const idea=require('../plugin/tests/idea.json');
idea.metadata.timeSignature={numerator:4,denominator:4};
for(const [layer,notes] of Object.entries(idea.layers))for(const note of notes)note.layer=layer;
const payload=JSON.stringify({protocolVersion:1,provider:'ollama',model:'test',ideas:[idea],sources:['ai']});
let count=0,child,finished=false;
const intervals=new Set();
const server=require('node:http').createServer((req,res)=>{
 req.resume();req.on('end',()=>{const n=++count;writeFileSync(path.join(root,'request'+n),'ready');
  res.writeHead(200,{'Content-Length':Buffer.byteLength(payload)});res.flushHeaders();
  const interval=setInterval(()=>{if(existsSync(path.join(root,'release'+n))){clearInterval(interval);intervals.delete(interval);res.end(payload);}},5);
  intervals.add(interval);res.on('close',()=>{clearInterval(interval);intervals.delete(interval);});
 });
});
function finish(code){if(finished)return;finished=true;clearTimeout(timer);for(const x of intervals)clearInterval(x);server.closeAllConnections();server.close();if(code&&child)child.kill();rmSync(root,{recursive:true,force:true});process.exitCode=code;}
const timer=setTimeout(()=>{console.error('Composer worker fixture timed out');finish(1);},30000);
server.on('error',error=>{console.error(error.message);finish(1);});
server.listen(4177,'127.0.0.1',()=>{child=spawn(path.resolve(executable),[root],{stdio:'inherit'});child.on('error',error=>{console.error(error.message);finish(1);});child.on('exit',code=>finish(code===0?0:1));});
