'use strict';
const fs=require('node:fs');const path=require('node:path');
const {generateOne,createMidi}=require('./core');
const idea=generateOne({key:'D',mode:'minor',bpm:105,bars:8,style:'nu-metal'},0xC0FFEE);
const out=path.resolve(__dirname,'../samples/loopforge-demo.mid');
fs.writeFileSync(out,createMidi(idea));
fs.writeFileSync(path.resolve(__dirname,'../samples/loopforge-demo.json'),JSON.stringify(idea,null,2));
console.log(out);
