const test=require('node:test');const assert=require('node:assert/strict');
const {generate}=require('../src/operations');const {normalizeOptions}=require('../src/schema');const {dispatch}=require('../src/mcp-server');
test('legacy generation refuses unsupported lengths without silently shortening',async()=>{
 for(const bars of [16,17,32,100]){assert.throws(()=>normalizeOptions({bars}),/bars/i);await assert.rejects(generate({provider:'offline',bars}),/bars/i);const reply=await dispatch({id:1,method:'tools/call',params:{name:'loopforge_generate',arguments:{bars}}});assert.equal(reply.result.isError,true);}
 for(const bars of [4,8])assert.equal((await generate({provider:'offline',bars,count:1})).ideas[0].metadata.bars,bars);
});
test('generation rejects unsupported duration fields instead of silently using four bars',async()=>{
 for(const params of [{duration:32},{duration:{unit:'bars',amount:32}},{sections:['verse']}])await assert.rejects(generate({provider:'offline',...params}),/Unsupported generation fields/);
});

test('legacy generation rejects unsupported meter and accepts explicit 4/4', async () => {
 const params = {provider:'offline', count:1};
 const timeSignature = {numerator:3, denominator:4};
 await assert.rejects(generate({...params, timeSignature}), /meter|signature/i);
 const result = await generate({...params, timeSignature:{numerator:4, denominator:4}});
 assert.equal(result.ideas.length, 1);
});

test('offline chord delays preserve the end of a 4- or 8-bar loop', () => {
 const {generateOne} = require('../src/core');
 for (const bars of [4, 8]) {
  for (let seed = 1; seed <= 64; seed++) {
   const {layers} = generateOne({style:'ambient', bars}, seed);
   for (const note of layers.chords)
    assert.ok(note.start + note.duration <= bars * 4, JSON.stringify({seed, bars, note}));
  }
 }
});

test('explicit unsupported styles reject before generation and an absent style uses the default', async () => {
 for (const style of ['trap+ambient', 'unknown', null, ['trap']])
  await assert.rejects(generate({style, count:1}), /style/i);
 assert.equal((await generate({count:1})).ideas[0].metadata.style, 'trap');
});
