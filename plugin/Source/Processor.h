#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Wire.h"
#include "Offline.h"
#include "Editing.h"
#include "engine/ProjectJson.h"
#include "engine/Prepared.h"
#include "engine/PromptCompose.h"
#include "PreparedMidi.h"
#include <deque>
#include <atomic>
#include <mutex>

static_assert(std::atomic<int>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);
class Processor final : public juce::AudioProcessor, private juce::Thread, private juce::AsyncUpdater {
public:
 explicit Processor(int timeoutMs=95000):AudioProcessor(BusesProperties().withOutput("Audition",juce::AudioChannelSet::stereo(),true)),Thread("LoopForge composition"),requestTimeoutMs(juce::jlimit(1,95000,timeoutMs)) {
  slots[0].state=3;wire::schedule(builtInIdea(),slots[0].data);startThread();
 }
 ~Processor() override {signalThreadShouldExit();notify();stopThread(-1);cancelPendingUpdate();}
 const juce::String getName()const override{return "LoopForge AI MIDI";}
 bool acceptsMidi()const override{return true;} bool producesMidi()const override{return true;} bool isMidiEffect()const override{return false;}
 double getTailLengthSeconds()const override{return 0;} int getNumPrograms()override{return 1;} int getCurrentProgram()override{return 0;}
 void setCurrentProgram(int)override{} const juce::String getProgramName(int)override{return "Default";} void changeProgramName(int,const juce::String&)override{}
 bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 void prepareToPlay(double sr,int)override {rate=std::isfinite(sr)&&sr>=1&&sr<=768000?sr:44100;midiLimitReached=false;for(int p=0;p<128;++p)increments[p]=juce::MathConstants<double>::twoPi*juce::MidiMessage::getMidiNoteInHertz(p)/rate;lifecycleDrain=true;internalBeat=0;internalSeconds=0;lastEnd=-1;lastStep=0;haveLastBlock=false;}
 void releaseResources()override{lifecycleDrain=true;internalSeconds=0;haveLastBlock=false;}
 void processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi)override {
  juce::ScopedNoDenormals noDenormals;audio.clear();midi.clear();eventsWritten=0;
  if(audio.getNumSamples()==0)return;
  // Lifecycle methods have no MIDI sink; retain ownership until a callback can release it.
  if(lifecycleDrain){lifecycleDrain=false;if(renderComposer){composer::PreparedMidiSink sink(midi);composerPlayback.stop(sink);}else flush(midi,0);if(!midi.isEmpty())return;}
  bool changed=false;int next=pending.exchange(-1);
  if(next>=0){slots[next].state=3;int old=active;active=next;slots[old].state=0;changed=true;}
  const auto& runtime=slots[active];
  if(runtime.composer!=renderComposer){
   if(renderComposer){composer::PreparedMidiSink sink(midi);composerPlayback.stop(sink);}else flush(midi,0);
   renderComposer=runtime.composer;internalBeat=internalSeconds=0;haveLastBlock=false;
   // An empty old ledger needs no drain block; preserve short notes at a cold host origin.
   if(!midi.isEmpty())return;
  }
  if(runtime.composer){
   composer::PlaybackBlock block;block.clock=composer::Clock::compositionSeconds;block.position=internalSeconds;block.sampleRate=rate;block.samples=audio.getNumSamples();block.playing=play.load();block.hostStartQuarterBeat=runtime.origin;
   if(auto* ph=getPlayHead())if(auto pos=ph->getPosition())if(auto ppq=pos->getPpqPosition()){
    block.clock=composer::Clock::hostQuarterBeats;block.position=*ppq;block.playing=pos->getIsPlaying();if(auto bpm=pos->getBpm())block.bpm=*bpm;
   }
   composer::PreparedMidiSink sink(midi);auto result=composerPlayback.render(*runtime.song,block,sink);
   midiLimitReached=result.overflow||sink.rejected();
   if(block.clock==composer::Clock::compositionSeconds)internalSeconds=block.playing?result.nextPosition:0;
   return;
  }
  const auto& s=runtime.data;double beat=internalBeat,bpm=s.bpm;bool playing=play.load();bool host=false;
  if(auto* ph=getPlayHead())if(auto pos=ph->getPosition()){
   if(auto ppq=pos->getPpqPosition()){host=true;beat=*ppq;playing=pos->getIsPlaying();if(auto tempo=pos->getBpm())bpm=*tempo;}
  }
  if(!std::isfinite(beat)){beat=0;playing=false;}
  if(!std::isfinite(bpm)||bpm<1||bpm>1000)bpm=s.bpm;
  const double step=bpm/(60.0*rate);const int samples=audio.getNumSamples();
  const bool discontinuity=haveLastBlock&&std::abs(beat-lastEnd)>juce::jmax(step,lastStep)*2;
  const double firstStep=haveLastBlock&&!discontinuity&&!changed?lastStep:step;
  if(changed || !playing || discontinuity)flush(midi,0);
  double previousLocal=std::fmod(beat-firstStep,s.beats);if(previousLocal<0)previousLocal+=s.beats;
  for(int sample=0;sample<samples;++sample){
   if(playing){double b=beat+sample*step;double local=std::fmod(b,s.beats);if(local<0)local+=s.beats;
    if(local<previousLocal)flush(midi,sample);previousLocal=local;
    bool overflow=false;
    s.forEventsAt(local,sample==0?firstStep:step,[&](const Event& e){
      if(eventsWritten>=maxMidiEvents-384-2){overflow=true;return;}
      auto& held=noteCounts[e.channel-1][e.pitch];
      auto& voice=voices[e.channel-1][e.pitch];
      if(e.on){
       if(held)emit(midi,juce::MidiMessage::noteOff(e.channel,e.pitch),sample);
       ++held;emit(midi,juce::MidiMessage::noteOn(e.channel,e.pitch,(juce::uint8)e.velocity),sample);voice=e.velocity/127.0f;
      }else if(held&&--held==0){emit(midi,juce::MidiMessage::noteOff(e.channel,e.pitch),sample);voice=0;}

    });
    if(overflow){flush(midi,sample);playing=false;midiLimitReached=true;}
   }
   float value=0;
   if(audition.load())for(int ch=0;ch<3;++ch)for(int p=0;p<128;++p)if(voices[ch][p]>0){
    phases[ch][p]+=increments[p];
    if(phases[ch][p]>juce::MathConstants<double>::twoPi)phases[ch][p]-=juce::MathConstants<double>::twoPi;
    value+=float(std::sin(phases[ch][p]))*voices[ch][p]*0.025f;
   }
   value=std::tanh(value);
   for(int ch=0;ch<audio.getNumChannels();++ch)audio.setSample(ch,sample,value);
  }
  haveLastBlock=playing;lastStep=step;lastEnd=playing?beat+samples*step:-1;if(!host)internalBeat=playing?beat+samples*step:0;
 }
 bool hasEditor()const override{return true;} juce::AudioProcessorEditor* createEditor()override;
 void getStateInformation(juce::MemoryBlock& dest)override {
  std::lock_guard lock(mutex);
  if(draftRecipeJson){
   composer::ProjectState project;auto decoded=composer::decodeProjectJson(stateJson);
   if(decoded.project){if(decoded.project->generationRecipeJson==draftRecipeJson){dest.replaceAll(stateJson.data(),stateJson.size());return;}project=std::move(*decoded.project);}
   else if(!stateJson.empty())project.legacyStateJson=stateJson;
   else{auto* root=new juce::DynamicObject;root->setProperty("version",1);root->setProperty("idea",builtInIdea());project.legacyStateJson=juce::JSON::toString(juce::var(root)).toStdString();}
   project.generationRecipeJson=draftRecipeJson;auto encoded=composer::encodeProjectJson(project);if(!encoded)throw std::bad_alloc();dest.replaceAll(encoded.json.data(),encoded.json.size());return;
  }
  if(!stateJson.empty())dest.replaceAll(stateJson.data(),stateJson.size());else{auto text=juce::JSON::toString(saved);dest.replaceAll(text.toRawUTF8(),text.getNumBytesAsUTF8());}
 }
 void setStateInformation(const void* data,int size)override {restoreStateInformation(data,size);}
 bool restoreStateInformation(const void* data,int size){
  if(!data||size<=0||size>int(composer::jsonDetail::maximumBytes)){std::lock_guard lock(mutex);status="Restore rejected: invalid project size";return false;}
  const std::string_view text(static_cast<const char*>(data),size_t(size));
  try{
   auto decoded=composer::decodeProjectJson(text);
   if(decoded.project)return restoreV2(std::move(*decoded.project),std::string(text));
   auto value=wire::document(text);Schedule checked;
   if(!wire::integer(value["version"],1,1)||!wire::schedule(value["idea"],checked)){std::lock_guard lock(mutex);status="Restore rejected: invalid or unsupported project";return false;}
   std::string original(text);juce::var list=juce::Array<juce::var>{value["idea"]};juce::String restoredStatus="Restored composition";
   std::lock_guard lock(mutex);
   if(!install(value["idea"],false,&original)){status="Restore rejected: playback storage unavailable";return false;}
   saved=std::move(value);ideas=std::move(list);selectedIndex=0;draftRecipeJson.reset();++draftEpoch;cancelRestoredWork();status=std::move(restoredStatus);return true;
  }catch(const std::bad_alloc&){std::lock_guard lock(mutex);status="Restore rejected: insufficient memory";return false;}
 }
 struct DraftPresentation{uint64_t epoch=0;std::optional<std::string> json;};
 std::optional<DraftPresentation> composerDraft(uint64_t known){std::lock_guard lock(mutex);if(known==draftEpoch)return {};return DraftPresentation{draftEpoch,draftRecipeJson};}
 bool hasSavableState(){std::lock_guard lock(mutex);return !stateJson.empty()||draftRecipeJson.has_value();}
 bool setComposerDraft(const composer::GenerationRecipe& draft,uint64_t expectedEpoch=UINT64_MAX){
  auto encoded=composer::encodeRecipeJson(draft);if(!encoded)return false;
  {std::lock_guard lock(mutex);if(expectedEpoch!=UINT64_MAX&&expectedEpoch!=draftEpoch)return false;if(draftRecipeJson&&*draftRecipeJson==encoded.json)return true;
   // Conservative JSON-escaping headroom; old large state is never truncated to fit metadata.
   if(projectV2&&!canonicalStateBytes)return false;
   const auto base=projectV2?*canonicalStateBytes:(stateJson.empty()?composer::limits::legacyBytes:stateJson.size());
   if((!projectV2&&base>composer::limits::legacyBytes)||!recipeFits(base*(projectV2?1:6),encoded.json))return false;
   draftRecipeJson=std::move(encoded.json);
  }notifyCommittedState();return true;
 }
 struct Presentation {uint64_t revision=0;bool composer=false,hasState=false;std::string json;};
 std::optional<Presentation> presentation(uint64_t knownRevision){std::lock_guard lock(mutex);if(knownRevision==revision)return {};return Presentation{revision,composerMode,!stateJson.empty(),composerMode?stateJson:std::string{}};}
 bool isComposerMode(){std::lock_guard lock(mutex);return composerMode;}
 void request(const juce::String& body){std::lock_guard lock(mutex);if(composerMode){status="Legacy generation unavailable in Composer mode";return;}if(busy)return;composerRequest=false;requestBody=body;requestRevision=revision;++jobGeneration;cancelled=false;status="Queued...";busy=true;notify();}
 bool requestComposer(const juce::String& direction,uint32_t seed,composer::ManualOverrides manual={}){
  std::lock_guard lock(mutex);if(busy)return false;
  if(direction.getNumBytesAsUTF8()>4096){status="Composer rejected: direction exceeds4096UTF8bytes";return false;}
  std::string base=stateJson;
  if(base.empty()){auto* root=new juce::DynamicObject;root->setProperty("version",1);root->setProperty("idea",builtInIdea());base=juce::JSON::toString(juce::var(root)).toStdString();}
  requestDirection=direction.toStdString();requestBase=std::move(base);requestSeed=seed;requestManual=std::move(manual);requestMode=composerMode;composerRequest=true;
  requestRevision=revision;++jobGeneration;cancelled=false;status="Composer queued...";busy=true;notify();return true;
 }
 bool cancel(){std::lock_guard lock(mutex);if(!busy)return false;cancelled=true;status="Cancelling...";notify();return true;}
 juce::String composerSummary(){std::lock_guard lock(mutex);return resolvedSummary;}
 bool canSwitchMode(){std::lock_guard lock(mutex);return composerMode?saved["idea"].isObject():hasComposerDocument;}
 bool selectMode(composer::ProjectMode mode){
  std::string base;CommitGuard guard;
  try{
  {std::lock_guard lock(mutex);const bool next=mode==composer::ProjectMode::composer;
   if(next==composerMode||!(next?hasComposerDocument:saved["idea"].isObject()))return false;
   base=stateJson;cancelRestoredWork();guard={revision,jobGeneration,composerMode,false};}
   auto decoded=composer::decodeProjectJson(base);if(!decoded.project)return false;decoded.project->activeMode=mode;
   auto encoded=composer::encodeProjectJson(*decoded.project);if(!encoded)return false;
   const bool changed=restoreV2(std::move(*decoded.project),std::move(encoded.json),guard);if(changed)notifyCommittedState();return changed;
  }catch(const std::bad_alloc&){std::lock_guard lock(mutex);status="Mode change rejected: insufficient memory";return false;}
 }

 void choose(int index){std::lock_guard lock(mutex);if(composerMode)return;if(auto* list=ideas.getArray())if(index>=0&&index<list->size()&&install((*list)[index]))selectedIndex=index;}
 juce::String getStatus(){
  std::lock_guard lock(mutex);juce::String detail;
  if(composerMode){char number[64];auto end=std::to_chars(number,number+sizeof(number),composerOrigin,std::chars_format::general,std::numeric_limits<double>::max_digits10);
   detail=" Composer: MIDI output and export; audition and editing unavailable. Origin PPQ "+juce::String::fromUTF8(number,int(end.ptr-number))+".";
  }
  if(projectV2)detail+=" Old plugin versions cannot open v2 projects.";
  return status+detail+(midiLimitReached?(composerMode?" MIDI block limit reached; stop/start transport or seek to resume.":" MIDI block limit reached; playback paused for this block."):"");
 }
 juce::var getIdea(){std::lock_guard lock(mutex);return composerMode?juce::var{}:saved["idea"];}
 int selectedIdeaIndex(){std::lock_guard lock(mutex);return composerMode?0:selectedIndex;}
 int ideaCount(){std::lock_guard lock(mutex);return !composerMode&&ideas.isArray()?ideas.size():0;}
 editing::Snapshot snapshot(){std::lock_guard lock(mutex);return {composerMode?juce::var{}:saved["idea"].clone(),revision,replacement};}
 bool canUndo(){std::lock_guard lock(mutex);return !composerMode&&!undo.empty();}
 bool canRedo(){std::lock_guard lock(mutex);return !composerMode&&!redo.empty();}
 bool edit(uint64_t expected,const juce::String& layer,int index,int pitch,double start,double duration,bool remove=false){
  bool changed=false;{std::lock_guard lock(mutex);if(composerMode||expected!=revision){status="Composition changed; edit cancelled";return false;}auto candidate=saved["idea"].clone();if(!editing::change(candidate,layer,index,pitch,start,duration,remove))return false;changed=commitEdit(candidate);}if(changed)updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));return changed;
 }
 bool quantize(uint64_t expected,const juce::String& layer){
  bool changed=false;{std::lock_guard lock(mutex);if(composerMode||expected!=revision)return false;auto candidate=saved["idea"].clone();if(!editing::quantize(candidate,layer))return false;changed=commitEdit(candidate);}if(changed)updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));return changed;
 }
 bool undoEdit(){return travelHistory(true);}
 bool redoEdit(){return travelHistory(false);}
 std::atomic<bool> play{false},audition{true},busy{false};
private:
 // Match the pinned VST3 converter; reserve up to 3*128 active note releases.
 static constexpr int maxMidiEvents=2048;int eventsWritten=0;std::atomic<bool> midiLimitReached{false};
 void emit(juce::MidiBuffer& midi,const juce::MidiMessage& message,int offset){midi.addEvent(message,offset);++eventsWritten;}
 const int requestTimeoutMs;
 struct Slot{Schedule data;std::optional<composer::PreparedSong> song;bool composer=false;double origin=0;std::atomic<int> state{0};};std::array<Slot,3> slots;
 bool haveLastBlock=false,lifecycleDrain=false;std::atomic<bool> cancelled{false};std::atomic<int> pending{-1};int active=0;double rate=44100,internalBeat=0,lastEnd=-1,lastStep=0;
 unsigned short noteCounts[16][128]{};float voices[16][128]{};double phases[16][128]{},increments[128]{};
 composer::Playback composerPlayback;bool renderComposer=false,composerMode=false,projectV2=false,hasComposerDocument=false;double internalSeconds=0,composerOrigin=0;std::string stateJson;
 uint64_t jobGeneration=0,requestRevision=0,revision=0,replacement=0;std::deque<juce::var> undo,redo;
 struct CommitGuard{uint64_t revision=0,job=0;bool mode=false,generated=false;};
 bool composerRequest=false,requestMode=false;uint32_t requestSeed=0;std::string requestDirection,requestBase;std::optional<std::string> draftRecipeJson;std::optional<size_t> canonicalStateBytes;uint64_t draftEpoch=0;composer::ManualOverrides requestManual;juce::String resolvedSummary;
 void handleAsyncUpdate()override{updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));}
 void notifyCommittedState()noexcept{try{triggerAsyncUpdate();}catch(const std::bad_alloc&){cancelPendingUpdate();}}
 static juce::var builtInIdea(){
  auto* metadata=new juce::DynamicObject;metadata->setProperty("bars",4);metadata->setProperty("bpm",100);
  auto* layers=new juce::DynamicObject;juce::Array<juce::var> bass,chords,melody;
  auto add=[](juce::Array<juce::var>& list,int pitch,double start,double duration,int velocity){auto* n=new juce::DynamicObject;n->setProperty("pitch",pitch);n->setProperty("start",start);n->setProperty("duration",duration);n->setProperty("velocity",velocity);list.add(juce::var(n));};
  const int roots[]{48,44,51,46};for(int bar=0;bar<4;++bar){int r=roots[bar];for(int pitch:{r,r+3,r+7})add(chords,pitch,bar*4,3.8,70);
   for(int beat=0;beat<4;++beat){add(bass,r-12,bar*4+beat,0.7,95);add(melody,r+12+(beat%2?7:3),bar*4+beat+0.5,0.4,85);}}
  layers->setProperty("bass",bass);layers->setProperty("chords",chords);layers->setProperty("melody",melody);
  auto* idea=new juce::DynamicObject;idea->setProperty("metadata",juce::var(metadata));idea->setProperty("layers",juce::var(layers));return juce::var(idea);
 }
 static juce::String describe(const composer::ResolvedConfig& resolved){
  constexpr const char* names[]{"root","scale","style","bpm","duration","meter","seed","feel","humanization","swing","complexity","instruments","mode","darkness","energy","melancholy","tension","aggression","atmosphere","sections"};
  juce::String text;for(size_t i=0;i<composer::fieldCount;++i){if(i)text+=" | ";text+=juce::String(names[i])+"="+juce::String(composer::fieldValue(resolved.config,composer::Field(i)))+" ["+(resolved.origins[i]==composer::Origin::manual?"manual":resolved.origins[i]==composer::Origin::text?"text":"default")+"]";}
  for(const auto& diagnostic:resolved.diagnostics)text+="; "+juce::String(diagnostic.message);return text.substring(0,4096);
 }
 int selectedIndex=0;std::mutex mutex;juce::String requestBody,status="Built-in demo ready. Generate offline ideas without a service.";juce::var ideas,saved;
 void resetNotes(){for(auto& ch:voices)for(auto& v:ch)v=0;for(auto& ch:noteCounts)for(auto& n:ch)n=0;}
 void flush(juce::MidiBuffer& midi,int offset){for(int ch=0;ch<16;++ch)for(int p=0;p<128;++p)if(voices[ch][p]>0)emit(midi,juce::MidiMessage::noteOff(ch+1,p),offset);resetNotes();}
 static void remember(std::deque<juce::var>& history,const juce::var& value){if(history.size()==64)history.pop_front();history.push_back(value);}
 void updateSelected(){if(auto* list=ideas.getArray())if(selectedIndex>=0&&selectedIndex<list->size())list->set(selectedIndex,saved["idea"]);}
 bool commitEdit(const juce::var& candidate){
  auto previous=saved["idea"];if(juce::JSON::toString(candidate)==juce::JSON::toString(previous))return false;
  if(!install(candidate,true)){status="Edit rejected: invalid notes or schedule unavailable";return false;}
  remember(undo,previous);redo.clear();updateSelected();status="Composition edited";return true;
 }
 bool travelHistory(bool backward){bool changed=false;{std::lock_guard lock(mutex);if(composerMode)return false;auto& from=backward?undo:redo;auto& to=backward?redo:undo;if(from.empty())return false;auto old=saved["idea"];if(!install(from.back(),true)){status="Schedule unavailable; try again";return false;}from.pop_back();remember(to,old);updateSelected();status=backward?"Edit undone":"Edit redone";changed=true;}if(changed)updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));return changed;}
 // All producer operations are serialized by mutex; callback only returns slot indexes.
 void reclaimReturned(){for(auto& slot:slots){int free=0;if(slot.state.compare_exchange_strong(free,1)){slot.song.reset();slot.state.store(0,std::memory_order_release);}}}
 int reserveSlot(size_t incoming=0){
  reclaimReturned();size_t retained=0;for(const auto& slot:slots)if(slot.song)retained+=slot.song->storageBytes();
  for(int i=0;i<3;++i){int free=0;if(slots[i].state.compare_exchange_strong(free,1)){
   const auto old=slots[i].song?slots[i].song->storageBytes():0;
   if(incoming>composer::limits::documentBytes-(retained-old)){slots[i].state=0;return -1;}
   slots[i].song.reset();return i;
  }}return -1;
 }
 static bool recipeFits(size_t base,const std::string& recipe)noexcept{return base<=composer::jsonDetail::maximumBytes-128&&recipe.size()<=(composer::jsonDetail::maximumBytes-base-128)/6;}
 void publishSlot(int index){slots[index].state.store(2,std::memory_order_release);int old=pending.exchange(index,std::memory_order_acq_rel);if(old>=0)slots[old].state.store(0,std::memory_order_release);}
 void cancelRestoredWork(){++jobGeneration;if(busy){cancelled=true;notify();}}
 bool restoreV2(composer::ProjectState candidate,std::string original,std::optional<CommitGuard> guard=std::nullopt,juce::String summary={}){
  const bool mode=candidate.activeMode==composer::ProjectMode::composer;
  std::optional<composer::PreparedSong> prepared;
  if(mode){auto ready=composer::prepareComposition(*candidate.composition,guard&&guard->generated?&cancelled:nullptr);if(!ready.song){std::lock_guard lock(mutex);status="Restore rejected: invalid prepared composition";return false;}prepared=std::move(ready.song);}
  juce::var legacyState,list;Schedule schedule;
  if(candidate.legacyStateJson){legacyState=wire::document(*candidate.legacyStateJson);list=juce::Array<juce::var>{legacyState["idea"]};if(!mode&&!wire::schedule(legacyState["idea"],schedule))return false;}
  std::optional<size_t> canonicalBytes=original.size();if(!guard){auto canonical=composer::encodeProjectJson(candidate);if(canonical)canonicalBytes=canonical.json.size();else canonicalBytes.reset();}
  const bool generated=guard&&guard->generated;juce::String restoredStatus=generated?"Native Composer ready":"Restored project";
  std::lock_guard lock(mutex);
  if(guard&&(revision!=guard->revision||jobGeneration!=guard->job||composerMode!=guard->mode||(guard->generated&&(cancelled||threadShouldExit()))))return false;
  if(guard&&draftRecipeJson&&draftRecipeJson!=candidate.generationRecipeJson&&(!canonicalBytes||!recipeFits(*canonicalBytes,*draftRecipeJson))){status="Restore rejected: recipe metadata budget exceeded";return false;}
  const int index=reserveSlot(prepared?prepared->storageBytes():0);
  if(index<0){status="Restore rejected: playback storage unavailable";return false;}
  auto& slot=slots[index];slot.data=schedule;slot.composer=mode;slot.origin=candidate.hostStartQuarterBeat;slot.song=std::move(prepared);
  saved=std::move(legacyState);ideas=std::move(list);selectedIndex=0;stateJson.swap(original);canonicalStateBytes=canonicalBytes;projectV2=true;composerMode=mode;hasComposerDocument=candidate.composition.has_value();composerOrigin=candidate.hostStartQuarterBeat;
  if(!guard){draftRecipeJson=std::move(candidate.generationRecipeJson);++draftEpoch;}else if(generated&&!draftRecipeJson)draftRecipeJson=std::move(candidate.generationRecipeJson);
  ++revision;++replacement;undo.clear();redo.clear();
  if(generated){resolvedSummary=std::move(summary);busy=false;}
  else{if(!guard)resolvedSummary.clear();cancelRestoredWork();}
  status=std::move(restoredStatus);publishSlot(index);
  if(generated)notifyCommittedState();return true;
 }
 bool install(const juce::var& idea,bool preserveHistory=false,const std::string* restored=nullptr){
  if(composerMode&&!restored)return false;
  Schedule schedule;if(!wire::schedule(idea,schedule))return false;
  juce::var nextSaved(new juce::DynamicObject);auto* object=nextSaved.getDynamicObject();object->setProperty("version",1);object->setProperty("idea",idea);
  std::string nextJson=restored?*restored:juce::JSON::toString(nextSaved).toStdString();
  const bool keepV2=projectV2&&!restored;
  if(keepV2){auto decoded=composer::decodeProjectJson(stateJson);if(!decoded.project)return false;decoded.project->activeMode=composer::ProjectMode::legacy;decoded.project->legacyStateJson=nextJson;auto encoded=composer::encodeProjectJson(*decoded.project);if(!encoded)return false;nextJson=std::move(encoded.json);if(draftRecipeJson&&draftRecipeJson!=decoded.project->generationRecipeJson&&!recipeFits(nextJson.size(),*draftRecipeJson))return false;}
  const int index=reserveSlot();if(index<0)return false;
  auto& slot=slots[index];slot.data=schedule;slot.composer=false;slot.origin=0;
  saved=std::move(nextSaved);stateJson.swap(nextJson);canonicalStateBytes=keepV2?std::optional<size_t>{stateJson.size()}:std::nullopt;projectV2=keepV2;composerMode=false;if(!keepV2){hasComposerDocument=false;resolvedSummary.clear();}++revision;if(!preserveHistory){++replacement;undo.clear();redo.clear();}
  publishSlot(index);return true;
 }
 void run()override{
  while(!threadShouldExit()){
   wait(200);juce::String body;uint64_t baseRevision=0,job=0;bool native=false,baseMode=false;std::string direction,base;uint32_t seed=0;composer::ManualOverrides manual;
   {std::lock_guard lock(mutex);reclaimReturned();if(!busy)continue;native=composerRequest;if(native){direction=std::move(requestDirection);base=std::move(requestBase);seed=requestSeed;manual=std::move(requestManual);baseMode=requestMode;}else body=requestBody;baseRevision=requestRevision;job=jobGeneration;status=native?"Resolving Composer direction...":"Generating...";}
   juce::String resultStatus;
   try{
    if(native){
     if(cancelled||threadShouldExit())throw std::runtime_error("Generation cancelled");
     auto resolved=composer::LocalPromptInterpreter{}.interpret(direction,seed,manual);
     if(!resolved.valid()){for(const auto& d:resolved.diagnostics)if(d.severity==composer::Severity::error)throw std::runtime_error("Composer rejected: "+d.message);}
     auto summary=describe(resolved);auto generated=composer::generateResolvedLoop(resolved);if(!generated.composition)throw std::runtime_error("Composer rejected: "+generated.error);
     composer::ProjectState candidate;auto decoded=composer::decodeProjectJson(base);
     if(decoded.project)candidate=std::move(*decoded.project);else candidate.legacyStateJson=std::move(base);
     candidate.activeMode=composer::ProjectMode::composer;candidate.composition=std::move(generated.composition);
     auto recipe=composer::encodeRecipeJson({direction,seed,manual});if(recipe)candidate.generationRecipeJson=std::move(recipe.json);else candidate.generationRecipeJson.reset(); // Preserve non-UI manual callers without promising a portable recipe.
     auto encoded=composer::encodeProjectJson(candidate);if(!encoded)throw std::runtime_error("Composer project encoding rejected");
     if(!restoreV2(std::move(candidate),std::move(encoded.json),CommitGuard{baseRevision,job,baseMode,true},std::move(summary)))throw std::runtime_error("Generation cancelled or Composer publication rejected");
     continue;
    }
    auto params=wire::document(std::string_view(body.toRawUTF8(),body.getNumBytesAsUTF8()));if(!params.isObject())throw std::runtime_error("Invalid composition request");bool probe=params["action"].toString()=="probe";
    if(cancelled||threadShouldExit())throw std::runtime_error("Generation cancelled");
    if(params["provider"].toString()=="offline"||params["provider"].isVoid()){
     auto generated=offline::generate(params);std::lock_guard lock(mutex);
     if(cancelled||threadShouldExit()||revision!=baseRevision||jobGeneration!=job||composerMode)throw std::runtime_error("Generation cancelled");
     if(probe){status="Offline generator ready - no service required.";busy=false;continue;}
     if(!install(generated[0]))throw std::runtime_error("Invalid offline composition");
     ideas=generated;selectedIndex=0;status="Offline ideas ready - no service required.";busy=false;continue;
    }
    // Use a direct loopback socket: native JUCE HTTP may ignore NO_PROXY on Linux.
    const auto deadline=juce::Time::getMillisecondCounterHiRes()+requestTimeoutMs;
    juce::StreamingSocket socket;
    if(!socket.connect("127.0.0.1",4177,juce::jmin(2000,requestTimeoutMs)))throw std::runtime_error("Local API unavailable. Run npm start in LoopHorge.");
    params.getDynamicObject()->removeProperty("action");body=juce::JSON::toString(params);
    auto request=juce::String(probe?"POST /api/v1/probe HTTP/1.1\r\nHost:":"POST /api/v1/generate HTTP/1.1\r\nHost:")+juce::String(" 127.0.0.1:4177\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: ")+juce::String(body.getNumBytesAsUTF8())+"\r\n\r\n"+body;
    const char* data=request.toRawUTF8();int remaining=int(request.getNumBytesAsUTF8());
    while(remaining>0){if(cancelled||threadShouldExit())throw std::runtime_error("Generation cancelled");if(juce::Time::getMillisecondCounterHiRes()>deadline)throw std::runtime_error("Generation timed out");int sent=socket.write(data,remaining);if(sent<=0)throw std::runtime_error("API request could not be sent");data+=sent;remaining-=sent;}
    juce::MemoryOutputStream bytes;char buffer[8192];
    while(true){
     if(cancelled||threadShouldExit()||juce::Time::getMillisecondCounterHiRes()>deadline)throw std::runtime_error("Generation cancelled or timed out");
     int ready=socket.waitUntilReady(true,100);if(ready<0)break;if(!ready)continue;
     int n=socket.read(buffer,sizeof(buffer),false);if(n<=0)break;
     if(bytes.getDataSize()+size_t(n)>4*1024*1024+8192)throw std::runtime_error("API response exceeded limit");bytes.write(buffer,size_t(n));
    }
    std::string raw(static_cast<const char*>(bytes.getData()),bytes.getDataSize());auto boundary=raw.find("\r\n\r\n");
    if(boundary==std::string::npos||boundary>8192||raw.substr(0,boundary).find('\0')!=std::string::npos||!juce::CharPointer_UTF8::isValidString(raw.data(),int(boundary)))throw std::runtime_error("Invalid local API headers");
    auto headers=juce::String::fromUTF8(raw.data(),int(boundary)).toLowerCase();auto payload=std::string_view(raw).substr(boundary+4);
    if(payload.size()>4*1024*1024)throw std::runtime_error("API response exceeded limit");
    if(headers.contains("transfer-encoding:"))throw std::runtime_error("Invalid local API transfer encoding");
    juce::String length;int lengthHeaders=0;for(auto line:juce::StringArray::fromLines(headers))if(line.startsWith("content-length:")){length=line.fromFirstOccurrenceOf(":",false,false).trim();++lengthHeaders;}
    auto lengthText=length.toStdString();uint64_t expected=0;auto parsedLength=std::from_chars(lengthText.data(),lengthText.data()+lengthText.size(),expected);
    if(lengthHeaders!=1||parsedLength.ec!=std::errc{}||parsedLength.ptr!=lengthText.data()+lengthText.size()||expected!=payload.size())throw std::runtime_error("API response length mismatch");
    auto response=wire::document(payload);
    if(raw.rfind("HTTP/1.1 200 ",0)!=0){
     auto error=response["error"]["message"].toString().substring(0,512);if(!wire::integer(response["protocolVersion"],1,1))error="check provider configuration and bridge version";
     throw std::runtime_error(("Local API rejected request: "+error).toStdString());
    }
    if(!response.isObject())throw std::runtime_error("Invalid bridge JSON");auto list=response["ideas"];
    if(!wire::integer(response["protocolVersion"],1,1))throw std::runtime_error("Unsupported bridge protocol; update the local service");
    if(!wire::reply(response,probe))throw std::runtime_error("Invalid bridge response");
    if(!probe){if(!list.isArray()||list.size()<1||list.size()>8)throw std::runtime_error("Invalid composition received");for(const auto& idea:*list.getArray()){Schedule checked;if(!wire::schedule(idea,checked,true))throw std::runtime_error("Invalid composition received");}}
    std::lock_guard lock(mutex);
    if(cancelled||threadShouldExit()||revision!=baseRevision||jobGeneration!=job||composerMode)throw std::runtime_error("Generation cancelled");
    if(probe){if(!bool(response["ok"]))throw std::runtime_error("Invalid provider probe response");status="Connected: "+response["provider"].toString()+" / "+response["model"].toString();busy=false;continue;}
    if(!list.isArray()||list.size()<1||list.size()>8||!install(list[0]))throw std::runtime_error("Invalid composition received");
    ideas=list;selectedIndex=0;status="Ideas ready - choose a variation, then play or export.";busy=false;continue;
   }catch(const std::exception& e){resultStatus=e.what();}
   {std::lock_guard lock(mutex);status=cancelled?juce::String("Generation cancelled"):resultStatus;busy=false;}
  }
 }
};
