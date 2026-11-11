'use strict';
const {spawn}=require('node:child_process');const path=require('node:path');
const executable=process.argv[2];if(!executable)throw Error('Pass the processor_test executable path');
const server=spawn(process.execPath,['-e',`global.fetch=async()=>new Response(JSON.stringify({choices:[{message:{content:JSON.stringify({ideas:require('./src/core').generateEight({key:'D',mode:'minor',style:'trap',bars:4,bpm:100},42)})}}]}));require('node:http').createServer(require('./src/server').listener).listen(4177,'127.0.0.1',()=>console.log('ready'));`],{env:{...process.env,LOOPFORGE_PORT:'4177'},stdio:['ignore','pipe','pipe']});
let ready=false,runner,finished=false;
function finish(code){if(finished)return;finished=true;clearTimeout(timer);server.kill();if(runner&&code)runner.kill();process.exitCode=code;}
const timer=setTimeout(()=>{console.error('Native API validation timed out');finish(1)},30000);
server.on('error',error=>{console.error(error.message);finish(1)});
server.stderr.on('data',data=>process.stderr.write(data));
server.on('exit',()=>{if(!finished){console.error('Test server exited; port 4177 must be free');finish(1)}});
server.stdout.on('data',()=>{
 if(ready||finished)return;ready=true;
 runner=spawn(path.resolve(executable),[path.resolve('plugin/tests/idea.json'),'--api'],{stdio:'inherit'});
 runner.on('error',error=>{console.error(error.message);finish(1)});runner.on('exit',code=>finish(code===0?0:1));
});
