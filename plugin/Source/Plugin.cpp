#include "Processor.h"
#include "MidiExport.h"
class Editor final: public juce::AudioProcessorEditor,private juce::Timer {
public:
 Editor(Processor& p):AudioProcessorEditor(p),processor(p){
  setSize(1000,740);
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
  exportButton.setButtonText("EXPORT MIDI");addAndMakeVisible(exportButton);exportButton.onClick=[this]{exportMidi();};
  addAndMakeVisible(status);status.setColour(juce::Label::textColourId,juce::Colours::lightgrey);timerCallback();startTimerHz(5);
 }
 void resized()override{
  title.setBounds(24,16,950,44);prompt.setBounds(24,76,950,70);
  provider.setBounds(24,164,160,32);key.setBounds(198,164,70,32);scale.setBounds(282,164,110,32);style.setBounds(406,164,140,32);bars.setBounds(560,164,70,32);bpm.setBounds(644,164,330,32);
  generate.setBounds(24,214,220,40);variation.setBounds(260,214,130,40);play.setBounds(406,214,140,40);sound.setBounds(560,214,160,40);exportButton.setBounds(744,214,230,40);model.setBounds(24,274,460,32);testConnection.setBounds(500,274,220,32);cancel.setBounds(744,274,230,32);status.setBounds(24,680,950,40);
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff151820));g.setColour(juce::Colour(0xff242936));g.fillRoundedRectangle(24,338,950,330,8);
  for(int i=0;i<=16;++i){g.setColour(juce::Colour(i%4?0xff303646:0xff4c566a));g.drawVerticalLine(24+i*950/16,338,668);}
  auto idea=processor.getIdea();auto layers=idea["layers"];int ch=0;const juce::Colour colours[]={juce::Colour(0xffb494ff),juce::Colour(0xff53d9ba),juce::Colour(0xffffbc70)};
  double beats=double(idea["metadata"]["bars"])*4;if(beats<=0){g.setColour(juce::Colours::white);g.drawText("Built-in demo ready. Generate ideas to view the piano roll.",40,420,900,30,juce::Justification::centred);return;}
  for(const char* name:{"bass","chords","melody"}){auto list=layers[name];if(auto* notes=list.getArray())for(const auto& n:*notes){g.setColour(colours[ch]);float x=24+float(double(n["start"])/beats*950),w=juce::jmax(2.0f,float(double(n["duration"])/beats*950));float y=650-float(int(n["pitch"])-24)*4;g.fillRoundedRectangle(x,juce::jlimit(340.0f,662.0f,y),w,4,1);}++ch;}
 }
private:
 Processor& processor;juce::Label title,status;juce::TextEditor prompt,model;juce::ComboBox provider,key,scale,style,bars,variation;juce::Slider bpm;juce::TextButton generate,play,exportButton,testConnection,cancel;juce::ToggleButton sound;std::unique_ptr<juce::FileChooser> chooser;
 juce::String requestBody(bool probe){auto* o=new juce::DynamicObject;
  o->setProperty("provider",provider.getText());o->setProperty("key",key.getText());o->setProperty("mode",scale.getText());o->setProperty("style",style.getText());o->setProperty("bars",bars.getText().getIntValue());o->setProperty("bpm",int(bpm.getValue()));o->setProperty("description",prompt.getText());o->setProperty("count",8);
  if(model.getText().isNotEmpty())o->setProperty("model",model.getText().trim());if(probe)o->setProperty("action","probe");return juce::JSON::toString(juce::var(o));
 }
 void setup(juce::ComboBox& box,std::initializer_list<const char*> values){int i=1;for(auto v:values)box.addItem(v,i++);box.setSelectedId(1);addAndMakeVisible(box);}
 void timerCallback()override{status.setText(processor.getStatus(),juce::dontSendNotification);generate.setEnabled(!processor.busy);testConnection.setEnabled(!processor.busy);cancel.setEnabled(processor.busy);int count=processor.ideaCount();if(count!=variation.getNumItems()){variation.clear();for(int i=0;i<count;++i)variation.addItem("IDEA "+juce::String(i+1),i+1);variation.setSelectedId(1,juce::dontSendNotification);}if(count>0)variation.setSelectedItemIndex(processor.selectedIdeaIndex(),juce::dontSendNotification);repaint();}
 void exportMidi(){auto idea=processor.getIdea();if(!idea.isObject()){status.setText("Generate an idea before export",juce::dontSendNotification);return;}
  chooser=std::make_unique<juce::FileChooser>("Export MIDI",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("loopforge.mid"),"*.mid");
  chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe=juce::Component::SafePointer<Editor>(this),idea](const juce::FileChooser& fc){if(!safe||fc.getResult()==juce::File{})return;
   auto bytes=encodeMidi(idea);bool ok=bytes.getSize()>0&&fc.getResult().replaceWithData(bytes.getData(),bytes.getSize());safe->status.setText(ok?"MIDI exported":"MIDI export failed",juce::dontSendNotification);
  });
 }
};
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor();}
