/* Browser application. Requests go only to the local LoopForge server. */
(function(){'use strict';
const $=id=>document.getElementById(id), core=window.LoopForgeCore;
let ideas=[],sources=[],selected=0,activeLayer='bass',playing=false,audioCtx=null,activeNodes=[],drag=null;
const ROW=17,BEAT_WIDTH=34;
const RANGES={bass:{top:71,bottom:24},chords:{top:83,bottom:36},melody:{top:95,bottom:48}};
function pitchRange(){return RANGES[activeLayer]}
const el={ideas:$('ideas'),message:$('message'),roll:$('roll'),piano:$('piano')};
const fields=['style','key','mode','bpm','bars'];
function options(){return Object.fromEntries(fields.map(k=>[k,(k==='bpm'||k==='bars')?Number($(k).value):$(k).value]));}
function noteName(p){const names=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];return names[p%12]+(Math.floor(p/12)-1)}
function message(s,isError=false){el.message.textContent=s;el.message.classList.toggle('error',isError)}
function current(){return ideas[selected]}
function demo(){ideas=core.generateEight(options(),20261111);sources=ideas.map(()=> 'offline');selected=0;render();message('8 local ideas ready. Click a card to edit, preview, or export.');}
async function generate(){
 const params={...options(),provider:$('provider').value,model:$('model').value.trim(),description:$('description').value.trim(),count:8};
 const btn=$('generate');btn.disabled=true;btn.innerHTML='GENERATING...';stop();
 try{
  const response=await fetch('/api/generate',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(params)});
  const data=await response.json();if(!response.ok)throw Error(data.error||'Generation failed');
  ideas=data.ideas;sources=data.sources;selected=0;render();
  message(`8 ideas loaded · ${data.provider} / ${data.model}${sources.includes('offline-fallback')?' · Some AI ideas were supplemented by the offline engine':''}`);
 }catch(e){message('Generation failed: '+e.message,true)}
 finally{btn.disabled=false;btn.innerHTML='<span>✦</span> GENERATE 8 IDEAS <span>→</span>'}
}
function render(){renderCards();renderRoll();}
function renderCards(){
 el.ideas.textContent='';
 for(let i=0;i<ideas.length;i++){
  const idea=ideas[i];const card=document.createElement('button');card.type='button';card.className='idea-card'+(i===selected?' active':'');
  const num=document.createElement('div');num.className='idea-number';num.textContent='DIRECTION / '+String(i+1).padStart(2,'0');
  const tag=document.createElement('i');tag.textContent=sources[i]==='ai'?'AI ✦':'LOCAL';num.append(tag);
  const title=document.createElement('div');title.className='idea-name';title.textContent=(i===selected?'▶ ':'')+String(idea.metadata.key)+' '+idea.metadata.mode.toUpperCase();
  const lines=document.createElement('div');lines.className='mini-keys';
  for(let j=0;j<36;j++){const bar=document.createElement('span');const n=idea.layers.melody[j%Math.max(1,idea.layers.melody.length)];bar.style.height=(9+(n?((n.pitch*7+j*11)%32):0))+'px';lines.append(bar)}
  card.append(num,title,lines);card.addEventListener('click',()=>{selected=i;stop();render();});el.ideas.append(card);
 }
}
function renderRoll(){
 const idea=current();if(!idea)return;
 const {top:PITCH_TOP,bottom:PITCH_BOTTOM}=pitchRange();
 const total=idea.metadata.bars*4;el.roll.style.width=(total*BEAT_WIDTH)+'px';
 el.roll.style.backgroundSize=`${4*BEAT_WIDTH}px 100%,${BEAT_WIDTH}px 100%,100% ${ROW}px`;
 el.piano.replaceChildren();el.roll.replaceChildren();
 for(let pitch=PITCH_TOP;pitch>=PITCH_BOTTOM;pitch--){const row=document.createElement('div');row.className='piano-key'+([1,3,6,8,10].includes(pitch%12)?' black':'');row.textContent=(pitch%12===0||pitch%12===6)?noteName(pitch):'';el.piano.append(row)}
 for(const layer of ['bass','chords','melody']){const notes=idea.layers[layer];$('n'+layer).textContent=notes.length+' NOTES'}
 for(const [index,n] of idea.layers[activeLayer].entries()){
  if(n.pitch<PITCH_BOTTOM||n.pitch>PITCH_TOP)continue;
  const e=document.createElement('div');e.className='note '+activeLayer;
  e.style.left=(n.start*BEAT_WIDTH)+'px';e.style.top=((PITCH_TOP-n.pitch)*ROW+1)+'px';e.style.width=Math.max(4,n.duration*BEAT_WIDTH-1)+'px';
  e.title=`${noteName(n.pitch)} · beat ${n.start} · duration ${n.duration}`;
  e.dataset.index=String(index);
  e.addEventListener('pointerdown',event=>{
    if(event.altKey){idea.layers[activeLayer].splice(index,1);renderRoll();return}
    drag={idx:index,startX:event.clientX,startY:event.clientY,originStart:n.start,originPitch:n.pitch};e.setPointerCapture(event.pointerId);
  });
  e.addEventListener('pointermove',event=>{
    if(!drag||drag.idx!==index)return;
    const deltaBeats=Math.round((event.clientX-drag.startX)/BEAT_WIDTH*4)/4;
    const deltaPitch=Math.round((drag.startY-event.clientY)/ROW);
    n.start=Math.max(0,Math.min(total-n.duration,drag.originStart+deltaBeats));
    n.pitch=Math.max(0,Math.min(127,drag.originPitch+deltaPitch));
    e.style.left=(n.start*BEAT_WIDTH)+'px';e.style.top=((PITCH_TOP-n.pitch)*ROW+1)+'px';
  });
  e.addEventListener('pointerup',()=>{drag=null;renderRoll()});
  el.roll.append(e);
 }
 $('note-info').textContent=idea.metadata.key+' '+idea.metadata.mode.toUpperCase()+' · '+idea.metadata.bpm+' BPM · '+idea.metadata.bars+' BARS';
}
function addNote(event){if(event.target!==el.roll)return;const idea=current();if(!idea)return;
 const {top:PITCH_TOP,bottom:PITCH_BOTTOM}=pitchRange();
 const rect=el.roll.getBoundingClientRect();const start=Math.max(0,Math.round((event.clientX-rect.left)/BEAT_WIDTH*4)/4);
 const pitch=PITCH_TOP-Math.floor((event.clientY-rect.top)/ROW);const max=idea.metadata.bars*4;
 if(start>=max||pitch<PITCH_BOTTOM||pitch>PITCH_TOP)return;
 const duration=Math.min(0.75,max-start);idea.layers[activeLayer].push({pitch,start,duration,velocity:96,layer:activeLayer});renderRoll();}
function transposeLayer(amount){const idea=current();if(!idea)return;for(const n of idea.layers[activeLayer])n.pitch=Math.max(0,Math.min(127,n.pitch+amount));renderRoll()}
function stop(){playing=false;for(const n of activeNodes){try{n.stop()}catch{}}activeNodes=[];$('play').textContent='▶ PREVIEW';}
function preview(){if(playing){stop();return}const idea=current();if(!idea)return;
 audioCtx=audioCtx||new (window.AudioContext||window.webkitAudioContext)();audioCtx.resume();playing=true;
 $('play').textContent='■ STOP';const secondsPerBeat=60/idea.metadata.bpm,begin=audioCtx.currentTime+0.08;
 for(const [layer,notes] of Object.entries(idea.layers)){
  for(const n of notes){const start=begin+n.start*secondsPerBeat;const end=start+Math.max(.04,n.duration*secondsPerBeat);
   const osc=audioCtx.createOscillator(),gain=audioCtx.createGain();osc.type=layer==='bass'?'sine':layer==='chords'?'triangle':'sawtooth';
   osc.frequency.value=440*Math.pow(2,(n.pitch-69)/12);gain.gain.setValueAtTime(0,start);
   const volume=layer==='chords'?.045:layer==='bass'?.1:.028;
   gain.gain.linearRampToValueAtTime(volume*n.velocity/127,start+.012);gain.gain.exponentialRampToValueAtTime(.0001,end);
   osc.connect(gain);gain.connect(audioCtx.destination);osc.start(start);osc.stop(end+.02);activeNodes.push(osc);
  }
 }
 setTimeout(()=>{if(playing)stop()},Math.ceil((idea.metadata.bars*4*secondsPerBeat+0.3)*1000));
}
async function download(){const idea=current();if(!idea)return;try{
 const res=await fetch('/api/export',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({idea})});
 if(!res.ok)throw Error((await res.json()).error||'Export failed');
 const blob=await res.blob();const url=URL.createObjectURL(blob);const a=document.createElement('a');a.href=url;a.download=`LoopForge_${idea.metadata.key}_${idea.metadata.mode}_${idea.metadata.bpm}bpm_${selected+1}.mid`;
 a.click();URL.revokeObjectURL(url);message('Exported standard multi-track MIDI. Drag into your DAW.');
 }catch(e){message('Export failed: '+e.message,true)}
}
async function statuses(){try{
 const data=await (await fetch('/api/status')).json();$('connectionStatus').textContent='Local engine online';$('providerStatus').replaceChildren();
 for(const [name,status] of Object.entries(data.providers)){
  const item=document.createElement('div');item.className='provider-chip';const label=document.createElement('span');label.textContent=name.toUpperCase();const txt=document.createElement('b');txt.textContent=status.local?'START OLLAMA':status.ready?'READY':name.endsWith('-cli')?'CLI REQUIRED':'KEY REQUIRED';txt.className=status.ready?'':'unset';item.append(label,txt);$('providerStatus').append(item)
 }
 }catch{$('connectionStatus').textContent='Preview mode';}
}
$('generate').addEventListener('click',generate);$('play').addEventListener('click',preview);$('download').addEventListener('click',download);$('exportLayer').addEventListener('click',download);
$('octdown').addEventListener('click',()=>transposeLayer(-12));$('octup').addEventListener('click',()=>transposeLayer(12));el.roll.addEventListener('dblclick',addNote);
document.querySelectorAll('.layer').forEach(b=>b.addEventListener('click',()=>{activeLayer=b.dataset.layer;document.querySelectorAll('.layer').forEach(x=>x.classList.toggle('selected',x===b));renderRoll()}));
$('provider').addEventListener('change',()=>{const offline=$('provider').value==='offline';$('model').disabled=offline;$('description').disabled=offline;});
$('provider').dispatchEvent(new Event('change'));demo();statuses();
})();
