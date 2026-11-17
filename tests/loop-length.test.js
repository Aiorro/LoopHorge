const test=require('node:test');const assert=require('node:assert/strict');
const {generate}=require('../src/operations');const {normalizeOptions}=require('../src/schema');const {dispatch}=require('../src/mcp-server');
test('legacy generation refuses unsupported lengths without silently shortening',async()=>{
 for(const bars of [16,17,32,100]){assert.throws(()=>normalizeOptions({bars}),/bars/i);await assert.rejects(generate({provider:'offline',bars}),/bars/i);const reply=await dispatch({id:1,method:'tools/call',params:{name:'loopforge_generate',arguments:{bars}}});assert.equal(reply.result.isError,true);}
 for(const bars of [4,8])assert.equal((await generate({provider:'offline',bars,count:1})).ideas[0].metadata.bars,bars);
});
test('generation rejects unsupported duration fields instead of silently using four bars',async()=>{
 for(const params of [{duration:32},{duration:{unit:'bars',amount:32}},{sections:['verse']}])await assert.rejects(generate({provider:'offline',...params}),/Unsupported generation fields/);
});
