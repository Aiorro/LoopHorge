'use strict';
const http=require('node:http');
const fs=require('node:fs');
const path=require('node:path');
const {generate,configuration}=require('./operations');
const {generateV1,failure,BridgeError}=require('./bridge');
const {createMidi}=require('./core');
const {normalizeOptions,validateIdea}=require('./schema');
const HOST='127.0.0.1';
const PORT=Number(process.env.LOOPFORGE_PORT||4177);
const ROOT=path.resolve(__dirname,'../web');
const MIME={'.html':'text/html; charset=utf-8','.css':'text/css; charset=utf-8','.js':'text/javascript; charset=utf-8','.svg':'image/svg+xml'};
function json(res,status,body){const text=JSON.stringify(body);res.writeHead(status,{'Content-Type':'application/json; charset=utf-8','Cache-Control':'no-store','Content-Length':Buffer.byteLength(text)});res.end(text)}
function readJSON(req){return new Promise((resolve,reject)=>{
 let body=[],size=0,oversized=false;
 req.on('data',chunk=>{if(oversized)return;size+=chunk.length;if(size>512*1024){oversized=true;body=[];const error=new Error('Request too large');error.status=413;reject(error);return;}body.push(chunk);});
 req.on('end',()=>{if(oversized)return;try{resolve(JSON.parse(size?Buffer.concat(body,size).toString('utf8'):'{}'))}catch{reject(new Error('Invalid JSON'))}});
 req.on('error',reject);
});}
async function listener(req,res){
 const versioned=/^\/api\/v\d+(?:\/|$)/.test(req.url);
 const fail=(status,code,message)=>json(res,status,versioned?{protocolVersion:1,error:{code,message}}:{error:message});
 try{
  const host=String(req.headers.host||'');
  if(!['127.0.0.1:'+PORT,'localhost:'+PORT].includes(host))return fail(403,'FORBIDDEN','Local host required');
  if(req.method==='POST'){
   const origin=req.headers.origin;
   if(origin&&!['http://127.0.0.1:'+PORT,'http://localhost:'+PORT].includes(origin))return fail(403,'FORBIDDEN','Cross-origin requests are forbidden');
   if(!String(req.headers['content-type']||'').startsWith('application/json'))return fail(415,'UNSUPPORTED_MEDIA_TYPE','JSON Content-Type required');
  }
  if(versioned){
   if(!req.url.startsWith('/api/v1/'))throw new BridgeError('UNSUPPORTED_VERSION','Supported bridge protocol version is 1',426);
   if(!['/api/v1/generate','/api/v1/probe'].includes(req.url))throw new BridgeError('NOT_FOUND','Unknown bridge endpoint',404);
   if(req.method!=='POST')throw new BridgeError('METHOD_NOT_ALLOWED','POST required',405);
   const args=await readJSON(req),controller=new AbortController();res.on('close',()=>controller.abort());
   const result=await generateV1(args,{probe:req.url==='/api/v1/probe',signal:controller.signal});
   if(!res.destroyed)return json(res,200,result);return;
  }
  if(req.method==='GET'&&req.url==='/api/status')return json(res,200,{ok:true,providers:configuration(),mcp:'node src/mcp-server.js'});
  if(req.method==='POST'&&(req.url==='/api/generate'||req.url==='/api/probe')){
    const args=await readJSON(req);const controller=new AbortController();res.on('close',()=>controller.abort());
    const probe=req.url==='/api/probe';const result=await generate(probe?{...args,count:1}:args,{signal:controller.signal});
    return json(res,200,probe?{ok:true,provider:result.provider,model:result.model,message:'Provider generated one validated idea successfully'}:result);
  }
  if(req.method==='POST'&&req.url==='/api/export'){
    const args=await readJSON(req),opts=normalizeOptions(args.idea?.metadata||{});
    const normalized=validateIdea(args.idea,opts);
    const bytes=createMidi(normalized,{split:args.split!==false});
    res.writeHead(200,{'Content-Type':'audio/midi','Content-Disposition':'attachment; filename="loopforge.mid"','Cache-Control':'no-store'});
    return res.end(Buffer.from(bytes));
  }
  if(req.method!=='GET')return json(res,405,{error:'Method not allowed'});
  const requested=new URL(req.url,'http://localhost').pathname;
  const file=path.resolve(ROOT,'.'+(requested==='/'?'/index.html':requested));
  if(file!==ROOT&&!file.startsWith(ROOT+path.sep))return json(res,403,{error:'Forbidden'});
  const final=requested==='/core.js'?path.resolve(__dirname,'core.js'):file;
  const allowed=new Set([path.join(ROOT,'index.html'),path.join(ROOT,'style.css'),path.join(ROOT,'app.js'),path.resolve(__dirname,'core.js')]);
  if(!allowed.has(final)||!fs.existsSync(final))return json(res,404,{error:'Not found'});
  res.writeHead(200,{'Content-Type':MIME[path.extname(final)]||'application/octet-stream','X-Content-Type-Options':'nosniff'});
  fs.createReadStream(final).pipe(res);
 }catch(err){if(!res.headersSent&&!res.destroyed){if(versioned){const error=failure(err);return fail(error.status,error.code,error.message)}json(res,err.status|| (err.message?.includes('API_KEY')?400:422),{error:err.message||'Request failed'});}}
}
if(require.main===module){http.createServer(listener).listen(PORT,HOST,()=>console.log(`LoopForge → http://${HOST}:${PORT}`));}
module.exports={listener};
