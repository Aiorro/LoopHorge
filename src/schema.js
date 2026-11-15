'use strict';
const {ROOTS,SCALES,STYLES} = require('./core');
const MAX_NOTES = 1600;
const LAYERS=['bass','chords','melody'];
function num(value, fallback){const n=Number(value);return Number.isFinite(n)?n:fallback;}
function clamp(x,lo,hi){return Math.min(hi,Math.max(lo,x));}
function normalizeOptions(raw={}){
  if (!raw || typeof raw!=='object' || Array.isArray(raw)) throw new Error('Invalid options');
  const key=Object.hasOwn(ROOTS,String(raw.key))?String(raw.key):'C';
  const mode=Object.hasOwn(SCALES,String(raw.mode))?String(raw.mode):'minor';
  const style=Object.hasOwn(STYLES,String(raw.style))?String(raw.style):'trap';
  const bpm=clamp(Math.round(num(raw.bpm,100)),40,240);
  const bars=Number(raw.bars)===8?8:4;
  return {key,mode,style,bpm,bars};
}
function validateIdea(raw, opts){
  if(!raw||typeof raw!=='object'||Array.isArray(raw))throw new Error('Expected idea object');
  const metadata={...opts,seed:Math.floor(num(raw.metadata?.seed,0))>>>0};
  if(!raw.layers||typeof raw.layers!=='object')throw new Error('Missing layers');
  const layers={bass:[],chords:[],melody:[]};
  let total=0;
  for(const layer of LAYERS){
    const notes=raw.layers[layer];
    if(!Array.isArray(notes)) throw new Error(`Missing ${layer} notes`);
    if(notes.length>800)throw new Error(`Too many ${layer} notes`);
    for(const n of notes){
      if(!n||typeof n!=='object')throw new Error('Invalid note');
      const pitch=Number(n.pitch),start=Number(n.start),duration=Number(n.duration),velocity=Number(n.velocity);
      if(![pitch,start,duration,velocity].every(Number.isFinite))throw new Error('Non-numeric note field');
      const maxBeats=metadata.bars*4;
      if(start<0||start>=maxBeats||duration<=0||pitch<0||pitch>127||velocity<1||velocity>127)throw new Error('Out-of-range note');
      const startMillis=Math.min(Math.round(start*1000),maxBeats*1000-1);
      const durationMillis=clamp(Math.round(duration*1000),1,maxBeats*1000-startMillis);
      layers[layer].push({pitch:Math.round(pitch), start:startMillis/1000,
        duration:durationMillis/1000,
        velocity:Math.round(velocity),layer});
      if(++total>MAX_NOTES)throw new Error('Too many total notes');
    }
    layers[layer].sort((a,b)=>a.start-b.start||a.pitch-b.pitch);
  }
  if(!total)throw new Error('Idea contains no notes');
  return {metadata,layers};
}
function extractJSON(text){
 if(typeof text!=='string')throw new Error('Model did not return text');
 let s=text.trim().replace(/^```(?:json)?\s*/i,'').replace(/\s*```$/,'').trim();
 try{return JSON.parse(s);}catch{}
 const first=s.indexOf('{'),last=s.lastIndexOf('}');
 if(first<0||last<=first)throw new Error('No JSON object in model response');
 try{return JSON.parse(s.slice(first,last+1));}catch{throw new Error('Model returned invalid JSON')}
}
function normalizeResponse(value,opts,count=8){
 const data=typeof value==='string'?extractJSON(value):value;
 const ideas=Array.isArray(data?.ideas)?data.ideas:(data?.layers?[data]:[]);
 if(!ideas.length)throw new Error('Model output has no ideas');
 return ideas.slice(0,count).map(x=>validateIdea(x,opts));
}
module.exports={normalizeOptions,validateIdea,normalizeResponse,LAYERS};
