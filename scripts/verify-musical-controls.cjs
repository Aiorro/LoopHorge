const assert = require('node:assert/strict');
const {chromium} = require('playwright');
(async () => {
 const browser = await chromium.launch({executablePath:process.env.CHROMIUM_PATH||(require('node:fs').existsSync('/usr/bin/chromium')?'/usr/bin/chromium':chromium.executablePath()), args:['--no-sandbox']});
 try {
  const page=await browser.newPage();
  await page.goto(process.env.LOOPFORGE_UI_URL||'http://127.0.0.1:4177');
  for(const [style,key,root,mode,intervals,bpm,bars] of [
   ['trap','D',2,'minor',[0,2,3,5,7,8,10],60,4],
   ['ambient','F#',6,'major',[0,2,4,5,7,9,11],180,8],
   ['r&b','A',9,'dorian',[0,2,3,5,7,9,10],96,4],
   ['nu-metal','C#',1,'phrygian',[0,1,3,5,7,8,10],160,8],
   ['boom-bap','B',11,'minor',[0,2,3,5,7,8,10],85,4]]){
   for(const [id,value] of Object.entries({style,key,mode,bars}))await page.selectOption('#'+id,String(value));
   await page.fill('#bpm',String(bpm));
   const response=page.waitForResponse(r=>r.url().endsWith('/api/generate'));
   await page.click('#generate');
   const data=await (await response).json();assert.equal(data.ideas.length,8);
   for(const idea of data.ideas){
    for(const [k,v] of Object.entries({style,key,mode,bpm,bars}))assert.equal(idea.metadata[k],v);
    for(const ns of Object.values(idea.layers))for(const n of ns)assert.ok(intervals.includes((n.pitch-root+12)%12));
   }
   await page.locator('.idea-card').nth(7).click();
   const exportRequest=page.waitForRequest(req=>req.url().endsWith('/api/export'));
   const download=page.waitForEvent('download');await page.click('#download');
   assert.deepEqual((await exportRequest).postDataJSON().idea,data.ideas[7]);
   const bytes=require('node:fs').readFileSync(await (await download).path());
   const tempo=Math.round(60000000/bpm);
   assert.ok(bytes.includes(Buffer.from([255,81,3,tempo>>16,tempo>>8,tempo])));
   console.log(style,key,mode,bpm,bars,'OK');
  }
 }finally{await browser.close()}
})().catch(e=>{console.error(e);process.exitCode=1});
