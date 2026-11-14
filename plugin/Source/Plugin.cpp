#include "Processor.h"
#include "MidiExport.h"
#include "PianoRoll.h"
#include "MidiDragSource.h"
class Editor final: public juce::AudioProcessorEditor,private juce::Timer {
public:
 Editor(Processor& p):AudioProcessorEditor(p),processor(p),roll(p),dragMidi([&p]{return p.snapshot().idea;},DragMidiCache(DragMidiCache::defaultRoot()),MidiDragSource::nativeDispatch,[safe=juce::Component::SafePointer<Editor>(this)](juce::String message){if(safe)safe->showDragStatus(std::move(message));}){
  setSize(1000,740);setResizable(true,true);setResizeLimits(1000,740,1600,1100);
  title.setText("LOOPFORGE  /  AI MIDI",juce::dontSendNotification);title.setFont(juce::FontOptions(26.0f));addAndMakeVisible(title);
  prompt.setMultiLine(true);prompt.setInputRestrictions(600);prompt.setText("Dark atmospheric nu-metal, coherent bass and melody");addAndMakeVisible(prompt);
  setup(provider,{"offline","openai","anthropic","openrouter","ollama","codex-cli","claude-cli"});
  setup(key,{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"});key.setSelectedId(3);
  setup(scale,{"minor","major","dorian","phrygian"});setup(style,{"nu-metal","trap","boom-bap","r&b","ambient"});setup(bars,{"4","8"});
  bpm.setRange(40,240,1);bpm.setValue(105);bpm.setTextValueSuffix(" BPM");addAndMakeVisible(bpm);
  model.setInputRestrictions(100);model.setTextToShowWhenEmpty("Default provider model",juce::Colours::grey);model.setTooltip("Optional model ID; sent only to the selected provider.");addAndMakeVisible(model);
  provider.onChange=[this]{model.clear();model.setEnabled(provider.getText()!="offline");};model.setEnabled(false);
  generate.setButtonText("GENERATE 8 IDEAS");addAndMakeVisible(generate);generate.onClick=[this]{processor.request(requestBody(false));};
  testConnection.setButtonText("TEST (1 IDEA)");testConnection.setTooltip("Generates one idea to check connection, login and model access. Cloud/CLI usage may incur charges. Keeps your current composition.");addAndMakeVisible(testConnection);testConnection.onClick=[this]{processor.request(requestBody(true));};
  cancel.setButtonText("CANCEL REQUEST");addAndMakeVisible(cancel);cancel.onClick=[this]{processor.cancel();};
  play.setButtonText("PLAY / STOP");addAndMakeVisible(play);play.onClick=[this]{processor.play=!processor.play.load();};
  sound.setButtonText("Audition synth");sound.setToggleState(true,juce::dontSendNotification);addAndMakeVisible(sound);sound.onClick=[this]{processor.audition=sound.getToggleState();};
  addAndMakeVisible(variation);variation.onChange=[this]{processor.choose(variation.getSelectedItemIndex());repaint();};
  addAndMakeVisible(dragMidi);
  exportButton.setButtonText("EXPORT MIDI");addAndMakeVisible(exportButton);exportButton.onClick=[this]{exportMidi();};
  setup(layer,{"bass","chords","melody"});layer.onChange=[this]{roll.selectLayer(layer.getText());viewport.setViewPosition(0,roll.preferredScrollY());};
  quantize.setButtonText("QUANTIZE 1/4 BEAT");undo.setButtonText("UNDO");redo.setButtonText("REDO");
  for(auto* button:{&quantize,&undo,&redo})addAndMakeVisible(*button);
  quantize.onClick=[this]{processor.quantize(processor.snapshot().revision,layer.getText());roll.refresh();};undo.onClick=[this]{processor.undoEdit();roll.refresh();};redo.onClick=[this]{processor.redoEdit();roll.refresh();};
  addNote.setButtonText("ADD NOTE");deleteNote.setButtonText("DELETE NOTE");addAndMakeVisible(addNote);addAndMakeVisible(deleteNote);addNote.onClick=[this]{roll.addNote();};deleteNote.onClick=[this]{roll.deleteSelected();};
  caption(promptLabel,"MUSICAL DIRECTION",prompt);caption(providerLabel,"Provider",provider);caption(keyLabel,"Root note",key);caption(scaleLabel,"Scale",scale);caption(styleLabel,"Style",style);caption(barsLabel,"Length (bars)",bars);caption(tempoLabel,"Tempo",bpm);caption(modelLabel,"Model (optional)",model);variation.setTitle("Idea");layer.setTitle("Edit layer");
  help.setText("Brackets select notes | Arrows move | Shift + Left/Right resize | Delete removes | Double-click adds",juce::dontSendNotification);help.setFont(juce::FontOptions(11.0f));help.setColour(juce::Label::textColourId,juce::Colours::lightgrey);addAndMakeVisible(help);
  int focusOrder=1;for(juce::Component* component:std::initializer_list<juce::Component*>{&prompt,&provider,&key,&scale,&style,&bars,&bpm,&generate,&variation,&play,&sound,&exportButton,&dragMidi,&model,&testConnection,&cancel,&layer,&quantize,&undo,&redo,&addNote,&deleteNote,&roll})component->setExplicitFocusOrder(focusOrder++);
  viewport.setViewedComponent(&roll,false);viewport.setScrollBarsShown(true,false);addAndMakeVisible(viewport);resized();viewport.setViewPosition(0,roll.preferredScrollY());
  layer.setTooltip("Double-click to add; drag to move; drag right edge to resize; right-click to delete. Scroll for pitch.");
  addAndMakeVisible(status);status.setColour(juce::Label::textColourId,juce::Colours::lightgrey);timerCallback();startTimerHz(5);
 }
 void resized()override{
  const int width=getWidth(),height=getHeight(),content=width-48;
  title.setBounds(24,16,content,40);promptLabel.setBounds(24,58,content,16);prompt.setBounds(24,76,content,70);
  provider.setBounds(24,164,160,32);key.setBounds(198,164,70,32);scale.setBounds(282,164,110,32);style.setBounds(406,164,140,32);bars.setBounds(560,164,70,32);bpm.setBounds(644,164,width-668,32);
  for(auto pair:{std::pair<juce::Label*,juce::Component*>{&providerLabel,&provider},{&keyLabel,&key},{&scaleLabel,&scale},{&styleLabel,&style},{&barsLabel,&bars},{&tempoLabel,&bpm}})pair.first->setBounds(pair.second->getX(),148,pair.second->getWidth(),15);
  generate.setBounds(24,214,220,40);variation.setBounds(260,214,130,40);play.setBounds(406,214,140,40);sound.setBounds(560,214,160,40);exportButton.setBounds(width-256,214,110,40);dragMidi.setBounds(width-136,214,110,40);
  cancel.setBounds(width-256,274,230,32);testConnection.setBounds(width-500,274,220,32);model.setBounds(24,274,width-538,32);modelLabel.setBounds(24,258,model.getWidth(),15);
  layer.setBounds(24,320,160,30);quantize.setBounds(200,320,190,30);undo.setBounds(406,320,140,30);redo.setBounds(560,320,140,30);addNote.setBounds(width-284,320,120,30);deleteNote.setBounds(width-146,320,120,30);
  viewport.setBounds(24,360,content,height-432);roll.setSize(viewport.getMaximumVisibleWidth(),128*12);status.setBounds(24,height-60,content,32);help.setBounds(24,height-25,content,20);
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff151820));g.setColour(juce::Colour(0xff242936));g.fillRoundedRectangle(viewport.getBounds().toFloat(),8);

 }
private:
 Processor& processor;PianoRoll roll;MidiDragSource dragMidi;juce::String dragStatus;juce::uint32 dragStatusUntil=0;juce::Viewport viewport;juce::ComboBox layer;juce::TextButton quantize,undo,redo,addNote,deleteNote;juce::Label title,status,help,promptLabel,providerLabel,keyLabel,scaleLabel,styleLabel,barsLabel,tempoLabel,modelLabel;juce::TextEditor prompt,model;juce::ComboBox provider,key,scale,style,bars,variation;juce::Slider bpm;juce::TextButton generate,play,exportButton,testConnection,cancel;juce::ToggleButton sound;std::unique_ptr<juce::FileChooser> chooser;
 juce::String requestBody(bool probe){auto* o=new juce::DynamicObject;
  o->setProperty("provider",provider.getText());o->setProperty("key",key.getText());o->setProperty("mode",scale.getText());o->setProperty("style",style.getText());o->setProperty("bars",bars.getText().getIntValue());o->setProperty("bpm",int(bpm.getValue()));o->setProperty("description",prompt.getText());o->setProperty("count",8);
  if(model.getText().isNotEmpty())o->setProperty("model",model.getText().trim());if(probe)o->setProperty("action","probe");return juce::JSON::toString(juce::var(o));
 }
 void caption(juce::Label& label,const juce::String& text,juce::Component& control){label.setText(text,juce::dontSendNotification);label.setFont(juce::FontOptions(11.0f));label.setColour(juce::Label::textColourId,juce::Colours::lightgrey);control.setTitle(text);addAndMakeVisible(label);}
 void setup(juce::ComboBox& box,std::initializer_list<const char*> values){int i=1;for(auto v:values)box.addItem(v,i++);box.setSelectedId(1);addAndMakeVisible(box);}
 void showDragStatus(juce::String message){dragStatus=std::move(message);dragStatusUntil=juce::Time::getMillisecondCounter()+5000;status.setText(dragStatus,juce::dontSendNotification);}
 void timerCallback()override{status.setText(dragStatus.isNotEmpty()&&int(juce::Time::getMillisecondCounter()-dragStatusUntil)<0?dragStatus:processor.getStatus(),juce::dontSendNotification);const bool legacy=!processor.isComposerMode();generate.setEnabled(legacy&&!processor.busy);testConnection.setEnabled(legacy&&!processor.busy);sound.setEnabled(legacy);exportButton.setEnabled(legacy);quantize.setEnabled(legacy);layer.setEnabled(legacy);roll.setEnabled(legacy);prompt.setEnabled(legacy);provider.setEnabled(legacy);key.setEnabled(legacy);scale.setEnabled(legacy);style.setEnabled(legacy);bars.setEnabled(legacy);bpm.setEnabled(legacy);model.setEnabled(legacy&&provider.getText()!="offline");title.setText(legacy?"LOOPFORGE  /  AI MIDI":"COMPOSER  /  MIDI ONLY",juce::dontSendNotification);status.setTooltip(processor.getStatus());help.setText(legacy?"Brackets select notes | Arrows move | Shift + Left/Right resize | Delete removes | Double-click adds":"MIDI only | Legacy controls disabled | v2 projects require a current plugin version",juce::dontSendNotification);cancel.setEnabled(processor.busy);int count=processor.ideaCount();dragMidi.setEnabled(count>0);addNote.setEnabled(count>0);if(count!=variation.getNumItems()){variation.clear();for(int i=0;i<count;++i)variation.addItem("IDEA "+juce::String(i+1),i+1);variation.setSelectedId(1,juce::dontSendNotification);}if(count>0)variation.setSelectedItemIndex(processor.selectedIdeaIndex(),juce::dontSendNotification);if(roll.refresh())viewport.setViewPosition(0,roll.preferredScrollY());deleteNote.setEnabled(roll.hasSelection());undo.setEnabled(processor.canUndo());redo.setEnabled(processor.canRedo());repaint();}
 void exportMidi(){auto idea=processor.getIdea();if(!idea.isObject()){status.setText("Generate an idea before export",juce::dontSendNotification);return;}
  chooser=std::make_unique<juce::FileChooser>("Export MIDI",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("loopforge.mid"),"*.mid");
  chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe=juce::Component::SafePointer<Editor>(this),idea](const juce::FileChooser& fc){if(!safe||fc.getResult()==juce::File{})return;
   auto bytes=encodeMidi(idea);bool ok=bytes.getSize()>0&&fc.getResult().replaceWithData(bytes.getData(),bytes.getSize());safe->status.setText(ok?"MIDI exported":"MIDI export failed",juce::dontSendNotification);
  });
 }
};
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor();}
