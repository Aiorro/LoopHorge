/* Browser application. Requests go only to the local LoopForge server. */
(function(){'use strict';
const $=id=>document.getElementById(id), core=window.LoopForgeCore;
let ideas=[],sources=[],selected=0,activeLayer='bass',playing=false,audioCtx=null,activeNodes=[],drag=null,selectedNote=0,playbackTimer=null,generating=false;
const ROW=17,BEAT_WIDTH=34;
const RANGES={bass:{top:71,bottom:24},chords:{top:83,bottom:36},melody:{top:95,bottom:48}};
function pitchRange(){const base=RANGES[activeLayer],pitches=(current()?.layers[activeLayer]||[]).map(n=>n.pitch);return {top:Math.min(127,Math.max(base.top,...pitches.map(p=>p+2))),bottom:Math.max(0,Math.min(base.bottom,...pitches.map(p=>p-2)))};}
const el={ideas:$('ideas'),message:$('message'),roll:$('roll'),piano:$('piano')};
const fields=['style','key','mode','bpm','bars'];
function options(){return Object.fromEntries(fields.map(k=>[k,(k==='bpm'||k==='bars')?Number($(k).value):$(k).value]));}
function noteName(p){const names=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];return names[p%12]+(Math.floor(p/12)-1)}
function message(s,isError=false){el.message.textContent=s;el.message.classList.toggle('error',isError)}
function current(){return ideas[selected]}
function demo(){ideas=core.generateEight(options(),20261111);sources=ideas.map(()=> 'offline');selected=0;render();message('8 local ideas ready. Click a card to edit, preview, or export.');}
async function generate(){
 if(generating||!$('bpm').reportValidity())return;cancelDrag();
 const params={...options(),provider:$('provider').value,model:$('model').value.trim(),description:$('description').value.trim(),count:8};
 const btn=$('generate');generating=true;btn.setAttribute('aria-disabled','true');btn.innerHTML='GENERATING...';stop();
 try{
  const response=await fetch('/api/generate',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(params)});
  const data=await response.json();if(!response.ok)throw Error(data.error||'Generation failed');
  cancelDrag();ideas=data.ideas;sources=data.sources;selected=0;selectedNote=0;render();
  message(`8 ideas loaded · ${data.provider} / ${data.model}${sources.includes('offline-fallback')?' · Some AI ideas were supplemented by the offline engine':''}`);
 }catch(e){message('Generation failed: '+e.message,true)}
 finally{generating=false;btn.setAttribute('aria-disabled','false');btn.innerHTML='<span aria-hidden="true">✦</span> GENERATE 8 IDEAS <span aria-hidden="true">→</span>'}
}
function render(){renderCards();renderRoll(false,true);}
function renderCards(){
 const meta=current()?.metadata;
 if(meta){
  $('fieldIdea').textContent='IDEA '+String(selected+1).padStart(2,'0')+' / '+String(ideas.length).padStart(2,'0');
  $('fieldKey').textContent=meta.key+' '+meta.mode;
  $('fieldTiming').textContent=meta.bpm+' BPM   ·   '+meta.bars+' bars';
  document.querySelector('.loop-field').style.setProperty('--core-x',(71+selected*2)+'%');
 }
 const focused=[...el.ideas.children].indexOf(document.activeElement);el.ideas.textContent='';
 for(let i=0;i<ideas.length;i++){
  const idea=ideas[i];const card=document.createElement('button');card.type='button';card.className='idea-card'+(i===selected?' active':'');card.setAttribute('aria-pressed',String(i===selected));
  const num=document.createElement('div');num.className='idea-number';num.textContent='IDEA '+String(i+1).padStart(2,'0');
  const tag=document.createElement('i');tag.textContent=['ai','codex-cli','claude-cli'].includes(sources[i])?'AI ✦':'LOCAL';num.append(tag);
  const title=document.createElement('div');title.className='idea-name';title.textContent=String(idea.metadata.key)+' '+idea.metadata.mode.toUpperCase();
  if(i===selected){const mark=document.createElement('span');mark.className='selection-mark';mark.textContent='✓';mark.setAttribute('aria-hidden','true');num.prepend(mark)}card.append(num,title);card.addEventListener('click',()=>{cancelDrag();selected=i;selectedNote=0;stop();render();});el.ideas.append(card);if(focused===i)card.focus({preventScroll:true});
 }
}
function cancelDrag(){if(drag){Object.assign(drag.note,drag.original);drag=null;}}
function activeNotes(){return current()?.layers[activeLayer]||[]}
function updateInspector(){
 const list=activeNotes();selectedNote=Math.max(0,Math.min(selectedNote,list.length-1));const note=list[selectedNote];
 const selector=$('noteSelect');selector.replaceChildren();for(const [i,n] of list.entries()){const option=document.createElement('option');option.value=i;option.textContent=`${i+1} · ${noteName(n.pitch)} · beat ${n.start}`;selector.append(option)}selector.value=String(selectedNote);selector.disabled=!note;
 for(const [id,key] of [['notePitch','pitch'],['noteStart','start'],['noteDuration','duration']]){$(id).value=note?String(note[key]):'';$(id).disabled=!note;}
 if(note){$('noteStart').max=String((current().metadata.bars*4000-Math.round(note.duration*1000))/1000);$('noteDuration').max=String((current().metadata.bars*4000-Math.round(note.start*1000))/1000);}
 $('deleteNote').disabled=!note;
 for(const button of el.roll.querySelectorAll('.note')){const chosen=Number(button.dataset.index)===selectedNote;button.tabIndex=chosen?0:-1;button.setAttribute('aria-pressed',String(chosen));}
}
function renderRoll(focusNote=false,center=false){
 const idea=current();if(!idea)return;const {top:PITCH_TOP,bottom:PITCH_BOTTOM}=pitchRange();const total=idea.metadata.bars*4;
 el.roll.style.minWidth=(total*BEAT_WIDTH)+'px';el.roll.style.height=((PITCH_TOP-PITCH_BOTTOM+1)*ROW)+'px';el.roll.style.backgroundSize=`${400/total}% 100%,${100/total}% 100%,100% ${ROW}px`;
 el.piano.replaceChildren();el.roll.replaceChildren();
 for(let pitch=PITCH_TOP;pitch>=PITCH_BOTTOM;pitch--){const row=document.createElement('div');row.className='piano-key'+([1,3,6,8,10].includes(pitch%12)?' black':'');row.textContent=(pitch%12===0||pitch%12===6)?noteName(pitch):'';el.piano.append(row)}
 for(const layer of ['bass','chords','melody'])$('n'+layer).textContent=idea.layers[layer].length+' NOTES';
 for(const [index,n] of idea.layers[activeLayer].entries()){
  if(n.pitch<PITCH_BOTTOM||n.pitch>PITCH_TOP)continue;const e=document.createElement('button');e.type='button';e.className='note '+activeLayer;e.dataset.index=String(index);e.setAttribute('aria-label',`${noteName(n.pitch)}, beat ${n.start}, length ${n.duration}`);e.setAttribute('aria-describedby','note-help');
  const position=()=>{e.style.left=`min(calc(100% - 4px), ${100*n.start/total}%)`;e.style.top=((PITCH_TOP-n.pitch)*ROW+1)+'px';e.style.width=`max(4px, calc(${100*n.duration/total}% - 1px))`;};position();
  e.addEventListener('focus',()=>{selectedNote=index;updateInspector()});
  e.addEventListener('click',()=>{selectedNote=index;updateInspector()});
  e.addEventListener('keydown',event=>{
   if(event.ctrlKey||event.metaKey||event.altKey)return;
   if(!['ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Delete','Backspace'].includes(event.key))return;event.preventDefault();cancelDrag();selectedNote=index;
   if(event.key==='Delete'||event.key==='Backspace'){deleteSelected(true);return;}
   stop();if(event.key==='ArrowUp'||event.key==='ArrowDown')n.pitch=Math.max(0,Math.min(127,n.pitch+(event.key==='ArrowUp'?1:-1)));
   else if(event.shiftKey){n.start=Math.min(n.start,total-.25);n.duration=Math.round(Math.max(.25,Math.min(total-n.start,n.duration+(event.key==='ArrowRight'?.25:-.25)))*1000)/1000;}
   else n.start=Math.max(0,Math.min(total-n.duration,n.start+(event.key==='ArrowRight'?.25:-.25)));
   renderCards();renderRoll(true);
  });
  e.addEventListener('pointerdown',event=>{
   if(event.button!==0)return;cancelDrag();selectedNote=index;updateInspector();stop();if(event.altKey){deleteSelected(true);return;}
   drag={element:e,pointerId:event.pointerId,note:n,original:{...n},idea,layer:activeLayer,index,startX:event.clientX,startY:event.clientY};e.setPointerCapture(event.pointerId);
  });
  e.addEventListener('pointermove',event=>{if(!drag||drag.element!==e||drag.pointerId!==event.pointerId)return;
   n.start=Math.max(0,Math.min(total-n.duration,drag.original.start+Math.round((event.clientX-drag.startX)*total/el.roll.clientWidth*4)/4));n.pitch=Math.max(0,Math.min(127,drag.original.pitch+Math.round((drag.startY-event.clientY)/ROW)));position();updateInspector();
  });
  const finish=(event,cancel)=>{if(!drag||drag.element!==e||drag.pointerId!==event.pointerId)return;if(cancel)cancelDrag();else drag=null;renderCards();renderRoll(true);};
  e.addEventListener('pointerup',event=>finish(event,false));e.addEventListener('pointercancel',event=>finish(event,true));e.addEventListener('lostpointercapture',event=>finish(event,true));el.roll.append(e);
 }
 updateInspector();$('note-info').textContent=idea.metadata.key+' '+idea.metadata.mode.toUpperCase()+' · '+idea.metadata.bpm+' BPM · '+idea.metadata.bars+' BARS';
 const wrap=document.querySelector('.piano-wrap');if(center&&activeNotes().length){const note=activeNotes()[selectedNote]||activeNotes()[0];wrap.scrollTop=(PITCH_TOP-note.pitch)*ROW-wrap.clientHeight/2;wrap.scrollLeft=note.start*el.roll.clientWidth/total;}
 if(focusNote){const button=el.roll.querySelector(`[data-index="${selectedNote}"]`);if(button){button.focus({preventScroll:true});button.scrollIntoView({block:'nearest',inline:'nearest'});}else $('addNote').focus({preventScroll:true});}
}
function insertNote(start,pitch){const idea=current();if(!idea)return;const list=activeNotes();if(list.length>=800||Object.values(idea.layers).reduce((sum,notes)=>sum+notes.length,0)>=1600){message('Note limit reached. Delete a note before adding another.',true);return;}
 const end=idea.metadata.bars*4;start=Math.max(0,Math.min(end-.25,start));pitch=Math.max(0,Math.min(127,pitch));list.push({pitch,start,duration:Math.min(.75,end-start),velocity:96,layer:activeLayer});selectedNote=list.length-1;stop();renderCards();renderRoll(false,true);message('Note added. Edit its pitch and timing above the grid.');}
function addNote(event){if(event.target!==el.roll)return;cancelDrag();const {top,bottom}=pitchRange();const rect=el.roll.getBoundingClientRect(),start=Math.round((event.clientX-rect.left)*current().metadata.bars*4/rect.width*4)/4,pitch=top-Math.floor((event.clientY-rect.top)/ROW);if(start>=current().metadata.bars*4||pitch<bottom||pitch>top)return;insertNote(start,pitch);}
function deleteSelected(focusNote=false){cancelDrag();const idea=current(),list=activeNotes();if(!list.length)return;if(Object.values(idea.layers).reduce((sum,notes)=>sum+notes.length,0)<=1){message('Keep at least one note in the idea.',true);return;}list.splice(selectedNote,1);selectedNote=Math.min(selectedNote,list.length-1);stop();renderCards();renderRoll(focusNote);message('Note deleted.');}
function transposeLayer(amount){cancelDrag();const idea=current();if(!idea)return;for(const n of activeNotes())n.pitch=Math.max(0,Math.min(127,n.pitch+amount));stop();renderCards();renderRoll(false,true)}
function stop(){clearTimeout(playbackTimer);playbackTimer=null;playing=false;for(const n of activeNodes){try{n.stop()}catch{}}activeNodes=[];$('play').textContent='▶ PREVIEW';}
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
 playbackTimer=setTimeout(()=>{if(playing)stop()},Math.ceil((idea.metadata.bars*4*secondsPerBeat+0.3)*1000));
}
async function download(){const idea=current();if(!idea)return;try{
 const res=await fetch('/api/export',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({idea})});
 if(!res.ok)throw Error((await res.json()).error||'Export failed');
 const blob=await res.blob();const url=URL.createObjectURL(blob);const a=document.createElement('a');a.href=url;a.download=`LoopForge_${idea.metadata.key}_${idea.metadata.mode}_${idea.metadata.bpm}bpm_${selected+1}.mid`;
 a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);message('Exported standard multi-track MIDI. Drag into your DAW.');
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
document.querySelectorAll('.layer').forEach(b=>b.addEventListener('click',()=>{cancelDrag();activeLayer=b.dataset.layer;selectedNote=0;document.querySelectorAll('.layer').forEach(x=>{x.classList.toggle('selected',x===b);x.setAttribute('aria-pressed',String(x===b));});renderRoll(false,true)}));
$('provider').addEventListener('change',()=>{const offline=$('provider').value==='offline';$('model').disabled=offline;$('description').disabled=offline;});
$('noteSelect').addEventListener('change',()=>{cancelDrag();selectedNote=Number($('noteSelect').value);renderRoll(false,true);});
$('addNote').addEventListener('click',()=>{cancelDrag();const n=activeNotes()[selectedNote];insertNote(n?Math.round((n.start+n.duration)*4)/4:0,n?n.pitch:activeLayer==='bass'?36:60);});
$('deleteNote').addEventListener('click',()=>deleteSelected());
for(const [id,key] of [['notePitch','pitch'],['noteStart','start'],['noteDuration','duration']])$(id).addEventListener('change',()=>{cancelDrag();const n=activeNotes()[selectedNote];if(!n||!$(id).reportValidity())return;const value=Number($(id).value);if(!Number.isFinite(value))return;n[key]=value;stop();renderCards();renderRoll();});
function updateNavigation(){const hash=location.hash||'#studio';document.querySelectorAll('nav .nav').forEach(link=>{const selected=link.getAttribute('href')===hash;link.classList.toggle('active',selected);if(selected)link.setAttribute('aria-current','location');else link.removeAttribute('aria-current');});}
window.addEventListener('hashchange',updateNavigation);updateNavigation();
$('provider').dispatchEvent(new Event('change'));demo();statuses();
})();
