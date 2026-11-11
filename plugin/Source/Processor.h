#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Schedule.h"
#include "Offline.h"
#include <atomic>
#include <mutex>

static_assert(std::atomic<int>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);
class Processor final : public juce::AudioProcessor, private juce::Thread {
public:
 Processor():AudioProcessor(BusesProperties().withOutput("Audition",juce::AudioChannelSet::stereo(),true)),Thread("LoopForge composition") {
  slots[0].state=3; auto& s=slots[0].data;
  const int roots[]={48,44,51,46};
  for(int bar=0;bar<4;++bar){int r=roots[bar];
   for(int p:{r,r+3,r+7})s.add(p,bar*4,3.8,70,2);
   for(int beat=0;beat<4;++beat){s.add(r-12,bar*4+beat,0.7,95,1);s.add(r+12+(beat%2?7:3),bar*4+beat+0.5,0.4,85,3);}
  }s.sort();startThread();
 }
 ~Processor() override {signalThreadShouldExit();notify();stopThread(-1);}
 const juce::String getName()const override{return "LoopForge AI MIDI";}
 bool acceptsMidi()const override{return true;} bool producesMidi()const override{return true;} bool isMidiEffect()const override{return false;}
 double getTailLengthSeconds()const override{return 0;} int getNumPrograms()override{return 1;} int getCurrentProgram()override{return 0;}
 void setCurrentProgram(int)override{} const juce::String getProgramName(int)override{return "Default";} void changeProgramName(int,const juce::String&)override{}
 bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 void prepareToPlay(double sr,int)override {rate=std::isfinite(sr)&&sr>=1&&sr<=768000?sr:44100;midiLimitReached=false;for(int p=0;p<128;++p)increments[p]=juce::MathConstants<double>::twoPi*juce::MidiMessage::getMidiNoteInHertz(p)/rate;resetNotes();internalBeat=0;lastEnd=-1;lastStep=0;}
 void releaseResources()override{resetNotes();}
 void processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi)override {
  juce::ScopedNoDenormals noDenormals;audio.clear();midi.clear();eventsWritten=0;
  if(audio.getNumSamples()==0)return;
  bool changed=false;int next=pending.exchange(-1);
  if(next>=0){slots[next].state=3;int old=active;active=next;slots[old].state=0;changed=true;}
  const auto& s=slots[active].data;double beat=internalBeat,bpm=s.bpm;bool playing=play.load();bool host=false;
  if(auto* ph=getPlayHead())if(auto pos=ph->getPosition()){
   if(auto ppq=pos->getPpqPosition()){host=true;beat=*ppq;playing=pos->getIsPlaying();if(auto tempo=pos->getBpm())bpm=*tempo;}
  }
  if(!std::isfinite(beat)){beat=0;playing=false;}
  if(!std::isfinite(bpm)||bpm<1||bpm>1000)bpm=s.bpm;
  const double step=bpm/(60.0*rate);const int samples=audio.getNumSamples();
  const bool discontinuity=lastEnd>=0&&std::abs(beat-lastEnd)>juce::jmax(step,lastStep)*2;
  const double firstStep=lastEnd>=0&&!discontinuity&&!changed?lastStep:step;
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
  lastStep=step;lastEnd=playing?beat+samples*step:-1;if(!host)internalBeat=playing?beat+samples*step:0;
 }
 bool hasEditor()const override{return true;} juce::AudioProcessorEditor* createEditor()override;
 void getStateInformation(juce::MemoryBlock& dest)override {std::lock_guard lock(mutex);auto text=juce::JSON::toString(saved);dest.replaceAll(text.toRawUTF8(),text.getNumBytesAsUTF8());}
 void setStateInformation(const void* data,int size)override {
  if(size<=0||size>4*1024*1024)return;
  auto value=juce::JSON::parse(juce::String::fromUTF8((const char*)data,size));
  std::lock_guard lock(mutex);if(int(value["version"])==1 && install(value["idea"])){saved=value;status="Restored composition";}
 }
 void request(const juce::String& body){std::lock_guard lock(mutex);if(busy)return;requestBody=body;cancelled=false;status="Queued...";busy=true;notify();}
 void cancel(){std::lock_guard lock(mutex);if(busy){cancelled=true;status="Cancelling...";notify();}}
 void choose(int index){std::lock_guard lock(mutex);if(auto* list=ideas.getArray())if(index>=0&&index<list->size())install((*list)[index]);}
 juce::String getStatus(){std::lock_guard lock(mutex);return status+(midiLimitReached?" MIDI block limit reached; playback paused for this block.":"");}
 juce::var getIdea(){std::lock_guard lock(mutex);return saved["idea"];}
 int ideaCount(){std::lock_guard lock(mutex);return ideas.isArray()?ideas.size():0;}
 std::atomic<bool> play{false},audition{true},busy{false};
private:
 // Match the pinned VST3 converter; reserve up to 3*128 active note releases.
 static constexpr int maxMidiEvents=2048;int eventsWritten=0;std::atomic<bool> midiLimitReached{false};
 void emit(juce::MidiBuffer& midi,const juce::MidiMessage& message,int offset){midi.addEvent(message,offset);++eventsWritten;}
 struct Slot{Schedule data;std::atomic<int> state{0};};std::array<Slot,3> slots;
 std::atomic<bool> cancelled{false};std::atomic<int> pending{-1};int active=0;double rate=44100,internalBeat=0,lastEnd=-1,lastStep=0;
 unsigned short noteCounts[16][128]{};float voices[16][128]{};double phases[16][128]{},increments[128]{};
 std::mutex mutex;juce::String requestBody,status="Built-in demo ready. Generate offline ideas without a service.";juce::var ideas,saved;
 void resetNotes(){for(auto& ch:voices)for(auto& v:ch)v=0;for(auto& ch:noteCounts)for(auto& n:ch)n=0;}
 void flush(juce::MidiBuffer& midi,int offset){for(int ch=0;ch<16;++ch)for(int p=0;p<128;++p)if(voices[ch][p]>0)emit(midi,juce::MidiMessage::noteOff(ch+1,p),offset);resetNotes();}
 bool install(const juce::var& idea){
  Schedule s;auto metadata=idea["metadata"];s.beats=double(metadata["bars"])*4;s.bpm=double(metadata["bpm"]);
  if((s.beats!=16&&s.beats!=32)||!std::isfinite(s.bpm)||s.bpm<40||s.bpm>240)return false;
  auto layers=idea["layers"];int channel=1;
  for(const char* name:{"bass","chords","melody"}){
   auto list=layers[name];if(!list.isArray()||list.size()>800)return false;
   for(const auto& n:*list.getArray()){
    for(const char* field:{"pitch","start","duration","velocity"})if(!n[field].isInt()&&!n[field].isInt64()&&!n[field].isDouble())return false;
    double pitch=n["pitch"],velocity=n["velocity"];
    if(!std::isfinite(pitch)||!std::isfinite(velocity)||pitch<0||pitch>127||velocity<1||velocity>127||pitch!=std::floor(pitch)||velocity!=std::floor(velocity))return false;
    if(!s.add(int(pitch),double(n["start"]),double(n["duration"]),int(velocity),channel))return false;
   }++channel;
  }
  if(!s.count)return false;s.sort();
  for(int i=0;i<3;++i){int free=0;if(slots[i].state.compare_exchange_strong(free,1)){
   slots[i].data=s;slots[i].state=2;int old=pending.exchange(i);if(old>=0)slots[old].state=0;
   auto* object=new juce::DynamicObject;object->setProperty("version",1);object->setProperty("idea",idea);saved=juce::var(object);return true;
  }}return false;
 }
 void run()override{
  while(!threadShouldExit()){
   wait(200);juce::String body;
   {std::lock_guard lock(mutex);if(!busy)continue;body=requestBody;status="Generating...";}
   juce::String resultStatus;
   try{
    auto params=juce::JSON::parse(body);bool probe=params["action"].toString()=="probe";
    if(cancelled||threadShouldExit())throw std::runtime_error("Generation cancelled");
    if(params["provider"].toString()=="offline"||params["provider"].isVoid()){
     auto generated=offline::generate(params);std::lock_guard lock(mutex);
     if(cancelled||threadShouldExit())throw std::runtime_error("Generation cancelled");
     if(probe){status="Offline generator ready - no service required.";busy=false;continue;}
     if(!install(generated[0]))throw std::runtime_error("Invalid offline composition");
     ideas=generated;status="Offline ideas ready - no service required.";busy=false;continue;
    }
    // Use a direct loopback socket: native JUCE HTTP may ignore NO_PROXY on Linux.
    juce::StreamingSocket socket;
    if(!socket.connect("127.0.0.1",4177,2000))throw std::runtime_error("Local API unavailable. Run npm start in LoopHorge.");
    auto request=juce::String(probe?"POST /api/probe HTTP/1.1\r\nHost:":"POST /api/generate HTTP/1.1\r\nHost:")+juce::String(" 127.0.0.1:4177\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: ")+juce::String(body.getNumBytesAsUTF8())+"\r\n\r\n"+body;
    const char* data=request.toRawUTF8();int remaining=int(request.getNumBytesAsUTF8());
    while(remaining>0){int sent=socket.write(data,remaining);if(sent<=0)throw std::runtime_error("API request could not be sent");data+=sent;remaining-=sent;}
    juce::MemoryOutputStream bytes;char buffer[8192];const auto deadline=juce::Time::getMillisecondCounterHiRes()+95000;
    while(true){
     if(cancelled||threadShouldExit()||juce::Time::getMillisecondCounterHiRes()>deadline)throw std::runtime_error("Generation cancelled or timed out");
     int ready=socket.waitUntilReady(true,100);if(ready<0)break;if(!ready)continue;
     int n=socket.read(buffer,sizeof(buffer),false);if(n<=0)break;
     if(bytes.getDataSize()+size_t(n)>4*1024*1024+8192)throw std::runtime_error("API response exceeded limit");bytes.write(buffer,size_t(n));
    }
    auto raw=bytes.toString();int boundary=raw.indexOf("\r\n\r\n");
    if(boundary<0)throw std::runtime_error("Invalid local API response");
    if(!raw.startsWith("HTTP/1.1 200 ")){
     auto error=juce::JSON::parse(raw.substring(boundary+4))["error"].toString().substring(0,512);
     throw std::runtime_error(("Local API rejected request: "+(error.isEmpty()?juce::String("check server provider configuration"):error)).toStdString());
    }
    auto headers=raw.substring(0,boundary).toLowerCase();auto payload=raw.substring(boundary+4);
    if(headers.contains("transfer-encoding:"))throw std::runtime_error("Restart the updated LoopForge server (Content-Length required).");
    if(payload.getNumBytesAsUTF8()>4*1024*1024)throw std::runtime_error("API response exceeded limit");
    auto response=juce::JSON::parse(payload);auto list=response["ideas"];
    std::lock_guard lock(mutex);
    if(cancelled||threadShouldExit())throw std::runtime_error("Generation cancelled");
    if(probe){if(!bool(response["ok"]))throw std::runtime_error("Invalid provider probe response");status="Connected: "+response["provider"].toString()+" / "+response["model"].toString();busy=false;continue;}
    if(!list.isArray()||list.size()<1||list.size()>8||!install(list[0]))throw std::runtime_error("Invalid composition received");
    ideas=list;resultStatus="Ideas ready - choose a variation, then play or export.";
   }catch(const std::exception& e){resultStatus=e.what();}
   {std::lock_guard lock(mutex);status=cancelled?juce::String("Generation cancelled"):resultStatus;busy=false;}
  }
 }
};

