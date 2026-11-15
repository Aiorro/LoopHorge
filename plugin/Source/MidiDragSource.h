#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "DragMidi.h"
#include <atomic>
class MidiDragSource final:public juce::TextButton {
public:
 using Dispatch=std::function<bool(const juce::StringArray&,bool,juce::Component*,std::function<void()>)>;
 MidiDragSource(std::function<juce::var()> provider,DragMidiCache cache,Dispatch dispatch,std::function<void(juce::String)> status):juce::TextButton("DRAG MIDI"),provider(std::move(provider)),cache(std::move(cache)),dispatch(std::move(dispatch)),status(std::move(status)){setTooltip("Drag the committed idea into a MIDI-capable application. Copy/import only; use Export MIDI for a permanent file.");}
 static juce::StringArray platformFiles(const juce::StringArray& files){
#if JUCE_LINUX
  juce::StringArray urls;for(const auto& file:files)urls.add(juce::URL(juce::File(file)).toString(true));return urls;
#else
  return files;
#endif
 }
 static bool nativeDispatch(const juce::StringArray& files,bool move,juce::Component* source,std::function<void()> done){return juce::DragAndDropContainer::performExternalDragDropOfFiles(platformFiles(files),move,source,std::move(done));}
 void mouseDown(const juce::MouseEvent& e)override{
  juce::TextButton::mouseDown(e);armed=false;if(!isEnabled()||!e.mods.isLeftButtonDown())return;captured=provider().clone();Schedule checked;if(!wire::schedule(captured,checked)){status("Generate a valid idea before dragging");return;}anchor=e.position;armed=true;
 }
 void mouseDrag(const juce::MouseEvent& e)override{
  juce::TextButton::mouseDrag(e);if(!armed||!e.mods.isLeftButtonDown()||e.position.getDistanceFrom(anchor)<6)return;armed=false;
  auto prepared=cache.prepare(captured);auto sink=status;auto start=dispatch;
  if(!prepared.lease){sink(prepared.error);return;}
  auto lease=std::move(prepared.lease);
  // Callback copies share one thread-safe hold; a retained finished callback must not pin the file.
  auto completionLease=std::make_shared<std::atomic<std::shared_ptr<DragMidiLease>>>(lease);
  bool started=start({lease->file.getFullPathName()},false,this,[completionLease]{completionLease->store({});});
  if(!started)completionLease->store({});
  // A native modal operation may have destroyed the source; only copied values remain in use.
  sink(started?"MIDI drag started; import depends on target":"MIDI drag unavailable; use Export MIDI");
 }
 void mouseUp(const juce::MouseEvent& e)override{armed=false;captured=juce::var{};juce::TextButton::mouseUp(e);}
private:
 std::function<juce::var()> provider;DragMidiCache cache;Dispatch dispatch;std::function<void(juce::String)> status;juce::var captured;juce::Point<float> anchor;bool armed=false;
};
