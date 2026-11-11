'use strict';
const http=require('node:http');const {spawn}=require('node:child_process');const path=require('node:path');
const executable=path.resolve(process.argv[2]);
(async()=>{
 for(const [mode,message,handler] of [
  ['--failure','rejected',(_req,res)=>{res.writeHead(429);res.end('private upstream details')}],
  ['--failure','Invalid composition',(_req,res)=>{res.writeHead(200,{'Content-Length':'12'});res.end('{"ideas":[]}')}],
  ['--failure','exceeded',(_req,res)=>{const body='x'.repeat(4*1024*1024+16384);res.writeHead(200,{'Content-Length':String(body.length)});res.end(body)}],
  ['--user-cancel','',(_req,res)=>{res.writeHead(200,{'Content-Length':'100'});res.flushHeaders()}],
  ['--cancel','',(_req,res)=>{res.writeHead(200,{'Content-Length':'100'});res.flushHeaders()}]
 ]){
  const server=http.createServer(handler);server.on('clientError',(_error,socket)=>socket.destroy());
  await new Promise((resolve,reject)=>{server.once('error',reject);server.listen(4177,'127.0.0.1',resolve)});
  try{await new Promise((resolve,reject)=>{const child=spawn(executable,[path.resolve('plugin/tests/idea.json'),mode,message],{stdio:'inherit'});const timer=setTimeout(()=>{child.kill();reject(Error('Native failure test timed out'))},15000);child.on('error',reject);child.on('exit',code=>{clearTimeout(timer);code===0?resolve():reject(Error('Native failure test exit '+code));});});}
  finally{server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
 }
 console.log('Native HTTP 429, invalid JSON composition, oversized body and stalled shutdown passed');
})().catch(error=>{console.error(error.message);process.exitCode=1});
