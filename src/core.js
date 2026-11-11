/* LoopForge musical engine - independent, rule-based, deterministic; no external services. */
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory();
  else root.LoopForgeCore = factory();
})(typeof globalThis !== 'undefined' ? globalThis : this, function () {
  'use strict';
  const PPQ = 480;
  const SCALES = {
    minor: [0, 2, 3, 5, 7, 8, 10],
    major: [0, 2, 4, 5, 7, 9, 11],
    dorian: [0, 2, 3, 5, 7, 9, 10],
    phrygian: [0, 1, 3, 5, 7, 8, 10]
  };
  const ROOTS = {'C':0,'C#':1,'D':2,'D#':3,'E':4,'F':5,'F#':6,'G':7,'G#':8,'A':9,'A#':10,'B':11};
  const STYLES = {
    trap:      { density: 0.66, octave: 5, bassOctave: 2, progression: [0,5,2,6], sync: .62 },
    'boom-bap':{ density: 0.48, octave: 5, bassOctave: 2, progression: [0,3,4,0], sync: .35 },
    'r&b':     { density: 0.60, octave: 5, bassOctave: 2, progression: [0,5,3,4], sync: .55 },
    ambient:   { density: 0.30, octave: 5, bassOctave: 2, progression: [0,3,5,4], sync: .18 },
    'nu-metal':{ density: 0.60, octave: 4, bassOctave: 2, progression: [0,6,5,4], sync: .70 }
  };
  function rng(seed) {
    let state = (Number(seed) >>> 0) || 1;
    return function () {
      state ^= state << 13; state ^= state >>> 17; state ^= state << 5;
      return (state >>> 0) / 4294967296;
    };
  }
  const clip = (x, low, high) => Math.max(low, Math.min(high, x));
  function hashString(str) {
    let h = 2166136261;
    for (const ch of String(str)) { h ^= ch.charCodeAt(0); h = Math.imul(h,16777619); }
    return h >>> 0;
  }
  function midiPitch(degree, octave, root, scale) {
    const idx = ((degree % 7) + 7) % 7;
    const oct = Math.floor(degree / 7);
    return clip((octave + 1) * 12 + root + scale[idx] + oct * 12, 0, 127);
  }
  function chordTones(degree, octave, root, scale, seventh) {
    const intervals = seventh ? [0, 2, 4, 6] : [0, 2, 4];
    return intervals.map(i => midiPitch(degree + i, octave, root, scale));
  }
  function note(pitch, start, duration, velocity, layer) {
    return {pitch: clip(Math.round(pitch), 0, 127), start: Math.round(start * 1000) / 1000,
      duration: Math.round(clip(duration, .0625, 32) * 1000) / 1000,
      velocity: clip(Math.round(velocity), 1, 127), layer};
  }
  function generateOne(opts, seed) {
    const {key='C', mode='minor', style='trap', bpm=100, bars=4} = opts;
    if (!Object.hasOwn(ROOTS, key)) throw new Error('Unknown musical key: ' + key);
    if (!Object.hasOwn(SCALES, mode)) throw new Error('Unknown scale: ' + mode);
    if (!Object.hasOwn(STYLES, style)) throw new Error('Unknown style: ' + style);
    if (!Number.isFinite(Number(bpm)) || bpm < 40 || bpm > 240) throw new Error('BPM must be 40..240');
    if (![4,8].includes(Number(bars))) throw new Error('Bars must be 4 or 8');
    const root = ROOTS[key], scale = SCALES[mode], preset = STYLES[style];
    const random = rng(seed); const layers = {bass:[], chords:[], melody:[]};
    const progression = preset.progression.map(x => (x + (random() < .15 ? 1 : 0)) % 7);
    const totalBeats = bars * 4;
    for (let bar=0; bar<bars; bar++) {
      const barStart = bar*4;
      const degree = progression[bar % progression.length];
      const chordPitches = chordTones(degree, 3, root, scale, style === 'r&b' || style === 'ambient');
      chordPitches.forEach((p, idx) => layers.chords.push(note(p,barStart + (random()<.2 ? .25:0),
        (style === 'ambient' ? 3.8 : style === 'nu-metal' ? 2 : 3.5) - idx*.06,
        66 + Math.floor(random()*24), 'chords')));
      const bassRoot = midiPitch(degree, preset.bassOctave, root, scale);
      const rhythm = style === 'ambient' ? [0,2] : style === 'nu-metal' ? [0,.5,1,2,2.5,3] : [0,1.5,2.5,3.5];
      for (let i=0; i<rhythm.length; i++) {
        if (i>0 && random()< .19) continue;
        const st = barStart+rhythm[i];
        const next = rhythm[i+1] ?? 4;
        const passing = i===rhythm.length-1 && random()<.35 ? 7 : 0;
        const base = passing ? midiPitch((degree+4)%7,preset.bassOctave,root,scale) : bassRoot;
        layers.bass.push(note(base,st,Math.min(next-rhythm[i], 1.1)*.84,
          78 + Math.floor(random()*35), 'bass'));
      }
      let previousDegree = degree + 7;
      for (let step=0;step<8;step++) {
        if (random() > preset.density) continue;
        const st = barStart + step*.5 + (random()<preset.sync ? .125 : 0);
        let candidate = (random()<.7 ? degree : progression[(bar+1)%progression.length]) + 7;
        candidate += [-2,-1,0,0,0,1,2,4][Math.floor(random()*8)];
        if (Math.abs(candidate-previousDegree)>4) candidate = previousDegree + Math.sign(candidate-previousDegree)*3;
        previousDegree=candidate;
        const length = (random()<.2 ? 1 : random()<.4 ? .5 : .25);
        const pitch = midiPitch(candidate, preset.octave-1, root, scale);
        layers.melody.push(note(pitch,st,Math.min(length,barStart+4-st),
          64 + Math.floor(random()*46), 'melody'));
      }
    }
    return {metadata: {key,mode,style,bpm:Number(bpm),bars:Number(bars),seed:seed>>>0}, layers};
  }
  function generateEight(opts, baseSeed) {
    const seed = baseSeed === undefined ? Math.floor(Math.random()*4294967296) : baseSeed>>>0;
    return Array.from({length:8},(_,i)=>generateOne(opts,(seed+Math.imul(i+1,2654435761))>>>0));
  }
  function transposePattern(pattern, semitones) {
    const n = Number(semitones)||0;
    return {...pattern, layers:Object.fromEntries(Object.entries(pattern.layers).map(([name,notes])=>
      [name,notes.map(x=>({...x,pitch:clip(Math.round(x.pitch+n),0,127)}))]))};
  }
  function varlen(value) {
    let n = Math.max(0,Math.round(value)); const out=[n&127];
    while ((n >>= 7) > 0) out.unshift((n&127)|128);
    return out;
  }
  function be(value, bytes) {return Array.from({length:bytes},(_,i)=>(value>>>((bytes-i-1)*8))&255);}
  const utf8 = (s) => Array.from(new TextEncoder().encode(s));
  function chunk(id, bytes) {return [...id.split('').map(x=>x.charCodeAt(0)),...be(bytes.length,4),...bytes];}
  function track(events) {
    const arr=events.slice().sort((a,b)=>a.tick-b.tick || a.priority-b.priority);
    let last=0, out=[];
    for(const ev of arr) {out.push(...varlen(ev.tick-last),...ev.bytes);last=ev.tick;}
    out.push(0,255,47,0);
    return chunk('MTrk',out);
  }
  function createMidi(pattern,{split=true,transpose=0}={}) {
    const {bpm=100,bars=4}=pattern.metadata;
    const tempo=Math.round(60000000/bpm);
    const duration=Math.round(bars*4*PPQ);
    const meta=track([
      {tick:0,priority:0,bytes:[255,81,3,...be(tempo,3)]},
      {tick:0,priority:1,bytes:[255,88,4,4,2,24,8]},
      {tick:0,priority:2,bytes:[255,3,9,...utf8('LoopForge')]},
      {tick:duration,priority:4,bytes:[255,1,0]}
    ]);
    const names=['bass','chords','melody'];
    const makeLayer=(selected)=>{
      const events=[];
      for(const name of selected){
        const ch=names.indexOf(name);
        for(const n of pattern.layers[name]||[]){
          const start=clip(Math.round(n.start*PPQ),0,duration-1), end=clip(Math.round((n.start+n.duration)*PPQ),start+1,duration);
          const pitch=clip(Math.round(n.pitch+transpose),0,127);
          events.push({tick:start,priority:2,bytes:[0x90|ch,pitch,clip(Math.round(n.velocity),1,127)]});
          events.push({tick:end,priority:0,bytes:[0x80|ch,pitch,0]});
        }
      }
      return track(events);
    };
    const tracks=split ? [meta,...names.map(n=>makeLayer([n]))] : [meta,makeLayer(names)];
    const content=[...chunk('MThd',[...be(1,2),...be(tracks.length,2),...be(PPQ,2)]),...tracks.flat()];
    return new Uint8Array(content);
  }
  return {PPQ,SCALES,ROOTS,STYLES,generateOne,generateEight,createMidi,transposePattern,hashString};
});
