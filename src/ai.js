'use strict';
const {normalizeResponse} = require('./schema');
const {generateEight} = require('./core');
const ENDPOINTS={
 openai:{url:'https://api.openai.com/v1/chat/completions',key:'OPENAI_API_KEY',model:'gpt-5.2'},
 anthropic:{url:'https://api.anthropic.com/v1/messages',key:'ANTHROPIC_API_KEY',model:'claude-sonnet-4-5'},
 openrouter:{url:'https://openrouter.ai/api/v1/chat/completions',key:'OPENROUTER_API_KEY',model:'openai/gpt-4.1-mini'},
 ollama:{url:'http://127.0.0.1:11434/v1/chat/completions',key:null,model:'qwen2.5:7b'}
};
function configuration(){return Object.fromEntries(Object.entries(ENDPOINTS).map(([name,info])=>[name,{ready:!!(info.key&&process.env[info.key]),local:name==='ollama',defaultModel:info.model}]))}
function prompt(opts,count,description){
 const total=opts.bars*4;
 return [
 'Compose original, coherent instrumental MIDI ideas for a music production plugin. Return ONLY valid JSON, no markdown.',
 `Requested: ${JSON.stringify(opts)}. Number of ideas: ${count}. User direction: ${String(description||'Produce a distinctive, musical motif.').slice(0,600)}`,
 `Schema: {"ideas":[{"layers":{"bass":[{"pitch":36,"start":0,"duration":1,"velocity":100}],"chords":[{"pitch":48,"start":0,"duration":3,"velocity":85}],"melody":[{"pitch":72,"start":0.5,"duration":0.5,"velocity":100}]}}]}.`,
 `Times are beat offsets, not seconds. There are ${total} beats. Output full ${count} ideas. Each should have meaningful chord progressions, bass aligned with chord roots and melodic motifs, with distinct rhythmic contours. Use MIDI pitches 0..127; starts >=0 and <${total}; duration>0; velocity 1..127. Use 4/4. Include all 3 layer arrays for every idea.`,
 'Keep note count modest (roughly 10-32 bass, 12-32 chord, 15-40 melody notes per idea); ensure key/scale agreement and voice-leading. No copyrighted melody quotations.'
 ].join('\n');
}
function completeIdeas(content, opts, count, source) {
 const ideas = normalizeResponse(content, opts, count);
 const sources = ideas.map(() => source);
 if (ideas.length < count) {
  const missing = generateEight(opts).slice(0, count - ideas.length);
  ideas.push(...missing);
  sources.push(...missing.map(() => 'offline-fallback'));
 }
 return {ideas, sources};
}
async function requestAI(provider, opts, {count=8,description='',model,signal,transport=fetch}={}){
 if(signal?.aborted)throw new Error('Generation cancelled');
 if(!Object.hasOwn(ENDPOINTS,provider))throw new Error('Unsupported provider');
 const config=ENDPOINTS[provider],apiKey=config.key?process.env[config.key]:'';
 if(config.key&&!apiKey)throw new Error(`${config.key} environment variable is not set`);
 const actualModel=(typeof model==='string'&&model.length<=100&&/^[\w.\-\/:]+$/.test(model))?model:config.model;
 const instruction=prompt(opts,count,description);
 const headers={'Content-Type':'application/json'};
 let body;
 if(provider==='anthropic'){
   headers['x-api-key']=apiKey;
   headers['anthropic-version']='2023-06-01';
   body={model:actualModel,max_tokens:20000,temperature:0.75,messages:[{role:'user',content:instruction}]};
 }else{
   if(apiKey)headers['Authorization']=`Bearer ${apiKey}`;
   body={model:actualModel,temperature:0.75,messages:[{role:'system',content:'You are a composer. Answer with valid JSON only.'},{role:'user',content:instruction}]};
 }
 const requestSignal=signal?AbortSignal.any([signal,AbortSignal.timeout(90000)]):AbortSignal.timeout(90000);
 let response;
 try{response=await transport(config.url,{method:'POST',headers,body:JSON.stringify(body),signal:requestSignal});}
 catch(error){if(signal?.aborted)throw new Error('Generation cancelled');if(requestSignal.reason?.name==='TimeoutError'||error?.name==='TimeoutError')throw new Error(`${provider} request timed out`);throw new Error(`${provider} connection failed; check connectivity and provider configuration`)}
 if(!response.ok){try{await response.body?.cancel()}catch{}throw new Error(`${provider} returned HTTP ${response.status}; ${response.status===401||response.status===403?'check authorization, API key and model access':response.status===429?'rate limit or quota reached; retry later':response.status===404?'check the model name':'check provider availability'}`);}
 const reader=response.body?.getReader();
 if(!reader)throw new Error('Provider sent empty response');
 const chunks=[];let size=0;
 try{
  while(true){
   const {done,value}=await reader.read();if(done)break;
   size+=value.byteLength;
   if(size>4*1024*1024){await reader.cancel();throw new Error('Provider response exceeded 4 MiB safety limit')}
   chunks.push(value);
  }
 }catch(error){
  if(signal?.aborted)throw new Error('Generation cancelled');
  if(requestSignal.reason?.name==='TimeoutError'||error?.name==='TimeoutError')throw new Error(`${provider} request timed out`);
  if(error.message==='Provider response exceeded 4 MiB safety limit')throw error;
  throw new Error('Provider response could not be read; check connectivity or timeout');
 }finally{reader.releaseLock()}
 const raw=Buffer.concat(chunks).toString('utf8');
 let json;try{json=JSON.parse(raw);}catch{throw new Error('Provider sent invalid JSON response')}
 if(!json||typeof json!=='object'||Array.isArray(json))throw new Error('Provider sent invalid JSON response');
 const content=provider==='anthropic'?(Array.isArray(json.content)?json.content.filter(c=>c?.type==='text'&&typeof c.text==='string').map(c=>c.text).join('\n'):undefined):json.choices?.[0]?.message?.content;
 if(typeof content!=='string'||!content.trim())throw new Error('Provider response contained no text');
 return {...completeIdeas(content, opts, count, 'ai'), model:actualModel, provider};
}
module.exports={requestAI,configuration,ENDPOINTS,prompt,completeIdeas};
