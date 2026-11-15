# Error audit and remediation routing

Audited code:b257237b8805801a0625ae7c08462702df86722f on2026-11-15. Expected load: one local interactive MIDI composer, optional authenticated external providers/CLIs, bounded native playback. No paid credentials/network providers were used. This pass changes diagnostic/work-routing documents only; findings below are not fixes. task.md is the sole actionable execution queue; errors.md owns stable IDs and statuses.

Three read-only investigations: native commit/lifecycle/resource paths; musical core/coverage; Node provider/MCP/export/process paths. Root separately read browser editing/preview/export, schema/options, HTTP/static routing, workflows and deployment instructions. Root reran all supplied reproduction binaries/scripts and checked the connected source. Git status initially contained only the two pre-existing untracked sample files; they remain untouched.

Fresh baseline: native CTest35/35 and Node42/42 passed. These are existing-suite results, not proof that the following defects are covered or fixed. The music investigator independently rebuilt/reran seven standard-only targets. No current failure of ERR059–066 within their documented regression scope was established; two new musical edge cases are recorded separately rather than silently changing earlier Fixed evidence.

## Confirmed defects, highest priority first

| Priority / ID | Concrete trigger and impact | Responsible correction area | Closure evidence required |
| --- | --- | --- | --- |
| P1 / ERR068 | operations.js:29 writes directly to an existing MIDI. An isolated517byte valid target became64bytes when a capped write raised EFBIG. Failed export loses the previous file. | Node export/MCP; operations.js | Unique same-directory staging; checked write/flush/close then atomic replace. Inject mid-write failure: original bytes unchanged, no target corruption/staging residue; valid retry. |
| P1 / ERR067 | schema.js:50 forwards native JSON.parse SyntaxError containing generated content. MCP and legacy HTTP echo it. Fixture model content `{"x":SECRET_KEY}` exposes the full placeholder in the error. | Provider/schema redaction; schema.js, ai.js, agent-cli.js, MCP/HTTP boundaries | API+CLI malformed content through MCP, legacy HTTP and v1 returns fixed safe diagnostic without source text/private marker; retain actionable safe errors. |
| P1 / ERR010 | Processor.h:28–29 clears held-note ownership during release/reprepare. A later stopped callback emits0offs after1on in both Legacy and Composer. | Native MIDI lifecycle; Processor/Playback | Keep bounded cleanup ownership until next valid callback, emit matching offs/pedal resets once at sample0, then valid restart;32/64sample allocation guards. Final destruction without any callback remains a host-delivery limitation. |
| P1 / ERR008 | Processor.h:215–222 can allocate after control-state mutation but before publishSlot. One-shot bad_alloc106 yields restore=false, revision1→2/Composer saved state, while playback still emits old legacy pitch38 instead of new60. Reserved writing slot is not rolled back. | Native transactional commit; Processor | Prebuild allocating state/status; nonthrowing commit-to-publication. Sweep allocation failures across restore/generation: rejected operation preserves bytes/mode/revision/history/playback and later slot reuse. |
| P2 / ERR070 | agent-cli.js:51 waits for close rather than root exit. A successful root exits while an ordinary descendant retains stdout; request falsely times out (512ms in500ms probe; default120s). | CLI lifecycle | Observe root exit, contain remaining ordinary descendants, drain bounded output and finish by root status. Test inherited stdout/stderr, success/nonzero status, no survivors and timely result. |
| P2 / ERR069 | mcp-server.js:18 ignores cancellation; line24 passes no signal. Fake CLI survives cancellation, stdin EOF and subsequent MCP SIGTERM. | MCP session/request lifecycle | Track typed request IDs/controllers, forward signal, cancel on notification/EOF/graceful shutdown, await containment; no paid work/ordinary descendants after cancellation, recovery and slot reuse. SIGKILL containment not promised. |
| P2 / ERR073 | agent-cli.js:14 synchronously discovers both CLIs during configuration/status. Two350ms locators block the event loop747ms; a10ms timer runs at749ms. | Async provider readiness | Nonblocking bounded cached/deduplicated discovery; concurrent status/ping/cancel remain responsive under slow probes. Do not infer the original Windows ERR004 cause or simply increase its deadline. |
| P2 / ERR072 | Rhythm.h:47 computes swing phase from bar-local base. In3/4 half boom-bap snare starts960/1920 become960/2160 at swing100, violating the documented two-bar stretch of unswung source attacks. | E02 groove/phase | Preserve source phrase phase across bars while retaining local bounds/headroom, shared alignment and monotonicity. Test3/4 plus another odd-length metre, all feels and existing4/4 literals. |
| P2 / ERR071 | Rhythm.h:33–34 maps both snare and ghost to unit1 in1/x; canonicalization merges them at strong velocity. Actual1/4 prompt accepts; ghost on/off gives identical32event vectors. | E02 drums/metre | Noncolliding quiet ghost for accepted1/2,1/4,1/8,1/16 across5styles×3feels, or explicit unsupported rejection. Preserve strong snares/bounds/duplicate handling. |

P1 grades for ERR008 and ERR067 are coordinator decisions: the investigators suggested P2 for rare allocation failure and diagnostic disclosure respectively. A recoverable single allocation can leave saved state/playback contradictory; an invalid provider response can expose private generated/prompt material. Impact makes both blockers before feature work, even though the reproduction uses fault injection/fake marker rather than a real memory incident/credential. No P0 defect established in this pass.

## Existing findings reassessed

- ERR003: narrow the missing coverage to a third metre change relative to a nonzero previous origin. Unequal3/4→5/8 already exists in JSON/MIDI fixtures. Actual0:4/4→1920:3/4→3360:5/8 accepts; global-origin4320 and off-by-one3361 must reject. Assign E06 validation coverage, not an arithmetic rewrite.
- ERR004: historical Windows Node22 discovery failure remains Investigating; Linux cannot prove its cause. Related event-loop blocking is now separately reproduced as ERR073. Preserve the recorded2000ms acceptance until a proven cause/design change.
- ERR005: at b257237 Native43749007011 and Prototype43749007198 failed/cancelled with all8jobs at runner_id0/steps0. No compile/test ran. Cause unknown; connector does not expose the requested check annotations endpoint. Assign CI/environment owner; no guessed billing/permission diagnosis.
- ERR006: fresh Windows/real DAW/pluginval/OS drag/chooser acceptance remains Blocked; assign platform/host validation. This is a release gate, not a locally reproduced new code defect.
- ERR007: aggregate256MiB/peak workspace remains unmeasured. No measured over-limit failure established here. Assign native resource instrumentation and maximum-document/queued-request/UI stress, then budget correction if measured evidence requires it.
- ERR008/010: promoted from risks/acceptance gaps to reproduced local defects as above. Do not retain the earlier "only unverified" characterization for the locally testable paths.
- ERR009: synchronous message-thread IO/decode is source-confirmed; no slow-storage stall was measured. Assign native file-IO responsiveness measurement and bounded async design if needed; classify as limitation, not a fabricated crash.
- ERR011: separate setsid/session-escaping descendant survived explicit cancellation in a cleaned-up fixture. Existing process groups do not contain deliberate escape. Assign process containment/threat-policy review; ordinary pipe/MCP lifecycle fixes cannot claim this case closed.

The original catalogue remains scoped: historical Fixed does not mean a fresh audit of every previous failure. No new musical listening/idiom/authenticity claim, full-wrapper race/allocation claim, host delivery proof, or aggregate-memory certification follows from this pass. Browser source was read; browser automation was not rerun here. A potential export-layer mismatch was discarded after confirming the button says EXPORT SELECTED IDEA. No finding based only on a misleading identifier was filed.

## Routing and dependency policy

The exact actionable order/owners/statuses live in task.md. P1 confidentiality/data integrity/MIDI lifecycle fixes precede feature work; reproduced P2 lifecycle/latency/music defects follow. Native lifecycle and native commit share Processor.h and must run sequentially; Node redaction/CLI/MCP readiness fixes also overlap and must be serialized. The independent Node staging writer and music helpers can proceed in separate work areas. CI/platform blockers run as a separate acceptance lane without pretending local green tests close them. Resource/coverage/IO measurements have explicit deliverables; they are not mixed into the confirmed-bug count.

No production fixes were applied in this audit. Work packages are assigned/queued, not falsely reported as running or complete. Future fix commits must update task.md and errors.md, observe RED→GREEN, run full relevant suites and obtain independent review before feature work resumes.

## Reproductions verified by coordinator

All fixture CLI children/temporary directories were explicitly cleaned up; no repository exports were modified. Fake SECRET_KEY is a literal test marker, not an actual credential. Native probes use current-thread one-shot allocation failure and the existing SharedCode archive, not a modified shared build.

- `/tmp/loopforge-audit-cli-b257237.cjs`: modes redaction/export/mcp/pipe/locator/setsid. Node24.22.1/Linux, bounded fake transports/processes, no port4177 or paid credentials.
- `/tmp/lf-safety-probe.cpp` and `/tmp/lf-safety-probe`: default mode reproduces four ledger-loss cases; `oom` sweeps200allocation positions and observes the partial publication at106.
- `/tmp/music-coverage-exact.cpp` and `/tmp/music-coverage-exact`: exact musical vectors for ghosts, odd-metre half swing and correct nonzero-origin arithmetic.

Sources are included below so evidence is durable after temporary workspace cleanup. Paths in sources are adapted to repository-root execution. Diagnostic sources are not new production dependencies or executing tests in the normal suite.

### Node probe

```js
'use strict';
const fs=require('fs'),os=require('os'),path=require('path'),cp=require('child_process');
const root=process.env.LOOPFORGE_REPO||process.cwd();
const pause=ms=>new Promise(r=>setTimeout(r,ms));
const alive=pid=>{try{process.kill(pid,0);return !/\) Z /.test(fs.readFileSync('/proc/'+pid+'/stat','utf8'))}catch{return false}};
function fixture(){return fs.mkdtempSync(path.join(os.tmpdir(),'lf-readonly-audit-'))}
function fake(dir,name,body){fs.writeFileSync(path.join(dir,name),'#!'+process.execPath+'\n'+body,{mode:0o755})}
async function main(){
const mode=process.argv[2];
if(mode==='redaction'){
 const {dispatch}=require(root+'/src/mcp-server');global.fetch=async()=>new Response(JSON.stringify({choices:[{message:{content:'{"x":SECRET_KEY}'}}]}));
 console.log(JSON.stringify(await dispatch({id:1,method:'tools/call',params:{name:'loopforge_generate',arguments:{provider:'ollama'}}})));return;
}
if(mode==='export'){
 const dir=fixture();try{fs.mkdirSync(dir+'/src');fs.mkdirSync(dir+'/exports');for(const file of ['operations.js','core.js','schema.js','ai.js','agent-cli.js'])fs.copyFileSync(root+'/src/'+file,dir+'/src/'+file);
 const core=require(root+'/src/core'),opts={key:'C',mode:'minor',style:'trap',bpm:100,bars:4};const original=Buffer.from(core.createMidi(core.generateOne(opts,42)));const output=dir+'/exports/existing.mid';fs.writeFileSync(output,original);
 const code=`process.on('SIGXFSZ',()=>{}); const {generateOne}=require(${JSON.stringify(dir+'/src/core')}); const {exportIdea}=require(${JSON.stringify(dir+'/src/operations')});try{console.log(exportIdea(generateOne(${JSON.stringify(opts)},43),'existing'))}catch(e){console.log('caught '+e.code)}`;
 const result=cp.spawnSync('prlimit',['--fsize=64:64',process.execPath,'-e',code],{encoding:'utf8'});const after=fs.readFileSync(output);console.log(JSON.stringify({status:result.status,signal:result.signal,stdout:result.stdout,before:original.length,after:after.length,equal:original.equals(after)}));
 }finally{fs.rmSync(dir,{recursive:true,force:true})}return;
}
if(mode==='locator'){
 const dir=fixture(),old=process.env.PATH;try{fake(dir,'which',"setTimeout(()=>{console.log('/fake/tool');process.exit(0)},350)");process.env.PATH=dir+path.delimiter+old;
 const {configuration}=require(root+'/src/operations');const start=Date.now();let timer=false;setTimeout(()=>{timer=true;console.log(JSON.stringify({timerDelayMs:Date.now()-start}))},10);const providers=configuration();console.log(JSON.stringify({configurationMs:Date.now()-start,timerRan:timer,codexReady:providers['codex-cli'].ready,claudeReady:providers['claude-cli'].ready}));await pause(20);
 }finally{process.env.PATH=old;fs.rmSync(dir,{recursive:true,force:true})}return;
}
if(mode==='pipe'||mode==='setsid'){
 const dir=fixture(),old=process.env.PATH,pidfile=dir+'/pid';let pid;try{
 const childOpts=mode==='pipe'?"{stdio:['ignore',1,2]}":"{detached:true,stdio:'ignore'}";
 fake(dir,'codex',`const {spawn}=require('child_process'),fs=require('fs');const c=spawn(process.execPath,['-e','setInterval(()=>{},1000)'],${childOpts});fs.writeFileSync(${JSON.stringify(pidfile)},String(c.pid));console.log('completed');${mode==='pipe'?"process.exit(0)":"process.stdin.resume();setInterval(()=>{},1000);"}`);
 process.env.PATH=dir+path.delimiter+old;const {execPrompt}=require(root+'/src/agent-cli');const controller=new AbortController(),start=Date.now();const result=execPrompt('codex-cli','hello',{timeoutMs:500,signal:controller.signal}).then(result=>({result}),e=>({error:e.message}));
 if(mode==='setsid'){for(let i=0;i<200&&!fs.existsSync(pidfile);i++)await pause(10);controller.abort();}
 console.log(JSON.stringify({...await result,elapsedMs:Date.now()-start}));pid=Number(fs.readFileSync(pidfile,'utf8'));await pause(30);console.log(JSON.stringify({mode,descendantAlive:alive(pid)}));
 }finally{if(pid)try{process.kill(pid,'SIGKILL')}catch{};process.env.PATH=old;fs.rmSync(dir,{recursive:true,force:true})}return;
}
if(mode==='mcp'){
 const dir=fixture(),pidfile=dir+'/pid';let pid,proc;try{
 fake(dir,'codex',`const fs=require('fs');fs.writeFileSync(${JSON.stringify(pidfile)},String(process.pid));process.stdin.resume();setInterval(()=>{},1000);`);
 proc=cp.spawn(process.execPath,[root+'/src/mcp-server.js'],{env:{...process.env,PATH:dir+path.delimiter+process.env.PATH},stdio:['pipe','pipe','pipe']});let output='';proc.stdout.on('data',x=>output+=x);proc.stderr.resume();
 proc.stdin.write(JSON.stringify({jsonrpc:'2.0',id:7,method:'tools/call',params:{name:'loopforge_generate',arguments:{provider:'codex-cli',count:1}}})+'\n');for(let i=0;i<200&&!fs.existsSync(pidfile);i++)await pause(10);if(!fs.existsSync(pidfile))throw Error('fake CLI did not start');pid=Number(fs.readFileSync(pidfile,'utf8'));
 proc.stdin.write(JSON.stringify({jsonrpc:'2.0',method:'notifications/cancelled',params:{requestId:7,reason:'user cancelled'}})+'\n');await pause(250);console.log(JSON.stringify({phase:'cancel',cliAlive:alive(pid),output}));proc.stdin.end();await pause(250);console.log(JSON.stringify({phase:'EOF',cliAlive:alive(pid),mcpAlive:proc.exitCode===null}));proc.kill('SIGTERM');await pause(250);console.log(JSON.stringify({phase:'MCP_SIGTERM',cliAlive:alive(pid),signal:proc.signalCode}));
 }finally{if(pid)try{process.kill(-pid,'SIGKILL')}catch{};proc?.kill('SIGKILL');fs.rmSync(dir,{recursive:true,force:true})}return;
}
throw Error('Usage: node '+__filename+' redaction|export|locator|pipe|setsid|mcp');
}
main().catch(e=>{console.error(e);process.exitCode=1});
```

### Native probe

```cpp
#include "plugin/Source/Processor.h"
#include <iostream>
#include <cstdlib>
#include <new>
#include <thread>
#include <sys/resource.h>
thread_local long failAfter=-1;
void* operator new(std::size_t n){if(failAfter==0){failAfter=-1;throw std::bad_alloc();}if(failAfter>0)--failAfter;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
const std::string legacy=R"({"version":1,"idea":{"metadata":{"bars":4,"bpm":120},"layers":{"bass":[{"pitch":38,"start":0,"duration":4,"velocity":90}],"chords":[],"melody":[]}}})";
std::string state(Processor& p){juce::MemoryBlock b;p.getStateInformation(b);return {static_cast<const char*>(b.getData()),b.getSize()};}
std::string project(){composer::ProjectState p;p.activeMode=composer::ProjectMode::composer;composer::Composition c;c.id="safety";c.endTick={1920};c.tempos={{{0},500000}};c.meters={{{0},4,4}};c.keys={{{0},0,composer::Scale::major}};composer::Track t;t.id=1;t.name="Track";t.channel=16;t.notes={{2,60,90,{0},{960}}};t.controls={{3,{0},64,127}};c.tracks={t};p.composition=std::move(c);p.legacyStateJson=legacy;return composer::encodeProjectJson(p).json;}
struct Clock:juce::AudioPlayHead{double ppq=0;bool playing=true;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(120);p.setPpqPosition(ppq);p.setIsPlaying(playing);return p;}};
int count(juce::MidiBuffer& b,bool on){int n=0;for(auto e:b)n+=on?e.getMessage().isNoteOn():e.getMessage().isNoteOff();return n;}
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI init;auto text=project();
if(argc>1&&std::string(argv[1])=="oom") {int found=0;for(int n=0;n<200;++n){auto p=std::make_unique<Processor>();p->restoreStateInformation(legacy.data(),legacy.size());auto before=state(*p);auto rev=p->snapshot().revision;bool result=false,escaped=false;failAfter=n;try{result=p->restoreStateInformation(text.data(),text.size());}catch(const std::exception& e){failAfter=-1;escaped=true;std::cout<<"escaped fail="<<n<<" what="<<e.what()<<"\n";}failAfter=-1;auto after=state(*p);if(!result&&(before!=after||p->snapshot().revision!=rev)){std::cout<<"PARTIAL fail="<<n<<" escaped="<<escaped<<" revision="<<rev<<"->"<<p->snapshot().revision<<" composer="<<p->isComposerMode()<<"\n";juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.ensureSize(32768);p->play=true;p->processBlock(audio,midi);std::cout<<"render pitches:";for(auto e:midi)if(e.getMessage().isNoteOn())std::cout<<" "<<e.getMessage().getNoteNumber();std::cout<<" status="<<p->getStatus()<<"\n";found++;}if(escaped)found++;}std::cout<<"oom findings="<<found<<"\n";return 0;}
for(bool composer:{false,true})for(bool prepare:{false,true}){auto p=std::make_unique<Processor>();Clock clock;p->setPlayHead(&clock);p->prepareToPlay(48000,64);const auto& input=composer?text:legacy;p->restoreStateInformation(input.data(),input.size());juce::AudioBuffer<float> a(2,64);juce::MidiBuffer m;m.ensureSize(32768);p->processBlock(a,m);int on=count(m,true);if(prepare)p->prepareToPlay(48000,64);else p->releaseResources();clock.playing=false;p->processBlock(a,m);std::cout<<"composer="<<composer<<" reset="<<(prepare?"prepare":"release")<<" before_on="<<on<<" stop_off="<<count(m,false)<<" messages="<<m.getNumEvents()<<"\n";}
}
```

### Musical probe

```cpp
#include "plugin/Source/engine/PromptCompose.h"
#include "plugin/Source/engine/Time.h"
#include <iostream>
using namespace composer;
std::vector<int64_t> ev(const Track&t){std::vector<int64_t> o;for(auto&n:t.notes)for(auto x:{int64_t(n.pitch),int64_t(n.velocity),n.start.value,n.duration.value})o.push_back(x);return o;}
int main(){auto resolved=LocalPromptInterpreter{}.interpret("4 bars 1/4 boom bap drums",42);std::cout<<"promptValid="<<resolved.valid()<<" generate="<<bool(generateResolvedLoop(resolved).composition)<<"\n";Config c;c.numerator=1;c.denominator=4;c.drums=true;c.humanization=0;c.drumFills=false;c.style=Style::boomBap;auto a=*generateDrums(c).track;c.ghostNotes=false;auto b=*generateDrums(c).track;std::cout<<"ghost on/off exactEventsEqual="<<(ev(a)==ev(b))<<" count="<<a.notes.size()<<","<<b.notes.size()<<" firstBar=";for(auto&n:a.notes)if(n.start.value<480)std::cout<<n.pitch<<"@"<<n.start.value<<":"<<n.velocity<<",";std::cout<<"\n";c=Config{};c.numerator=3;c.denominator=4;c.feel=Feel::half;c.style=Style::boomBap;c.humanization=0;c.drums=true;c.ghostNotes=c.drumFills=false;for(int swing:{0,100}){c.swing=swing;std::cout<<"3/4 half swing"<<swing<<" snare=";auto drums=generateDrums(c);for(auto&n:drums.track->notes)if(n.pitch==38&&n.start.value<2880)std::cout<<n.start.value<<",";std::cout<<"\n";}
Composition v;v.id="meter";v.endTick={6240};v.tempos={{{0},500000}};v.keys={{{0},0,Scale::major}};Track t;t.id=1;t.name="Bass";t.channel=1;t.notes={{2,60,90,{0},{480}}};v.tracks={t};v.meters={{{0},4,4},{{1920},3,4},{{3360},5,8}};std::cout<<"unequal/nonzero previous origin valid="<<bool(validate(v));v.meters[2].tick={4320};std::cout<<" global-align invalid="<<!bool(validate(v))<<"\n";
}
```
