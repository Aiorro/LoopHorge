#include "Processor.h"
class Editor final: public juce::AudioProcessorEditor,private juce::Timer {
public:
 Editor(Processor& p):AudioProcessorEditor(p),processor(p){
  setSize(1000,680);
  title.setText("LOOPFORGE  /  AI MIDI",juce::dontSendNotification);title.setFont(juce::FontOptions(26.0f));addAndMakeVisible(title);
  prompt.setMultiLine(true);prompt.setInputRestrictions(600);prompt.setText("Dark atmospheric nu-metal, coherent bass and melody");addAndMakeVisible(prompt);
  setup(provider,{"offline","openai","anthropic","openrouter","ollama","codex-cli","claude-cli"});
  setup(key,{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"});key.setSelectedId(3);
  setup(scale,{"minor","major","dorian","phrygian"});setup(style,{"nu-metal","trap","boom-bap","r&b","ambient"});setup(bars,{"4","8"});
  bpm.setRange(40,240,1);bpm.setValue(105);bpm.setTextValueSuffix(" BPM");addAndMakeVisible(bpm);
  generate.setButtonText("GENERATE 8 IDEAS");addAndMakeVisible(generate);generate.onClick=[this]{auto* o=new juce::DynamicObject;
   o->setProperty("provider",provider.getText());o->setProperty("key",key.getText());o->setProperty("mode",scale.getText());o->setProperty("style",style.getText());o->setProperty("bars",bars.getText().getIntValue());o->setProperty("bpm",int(bpm.getValue()));o->setProperty("description",prompt.getText());o->setProperty("count",8);processor.request(juce::JSON::toString(juce::var(o)));};
  play.setButtonText("PLAY / STOP");addAndMakeVisible(play);play.onClick=[this]{processor.play=!processor.play.load();};
  sound.setButtonText("Audition synth");sound.setToggleState(true,juce::dontSendNotification);addAndMakeVisible(sound);sound.onClick=[this]{processor.audition=sound.getToggleState();};
  addAndMakeVisible(variation);variation.onChange=[this]{processor.choose(variation.getSelectedItemIndex());repaint();};
  exportButton.setButtonText("EXPORT MIDI");addAndMakeVisible(exportButton);exportButton.onClick=[this]{exportMidi();};
  addAndMakeVisible(status);status.setColour(juce::Label::textColourId,juce::Colours::lightgrey);timerCallback();startTimerHz(5);
 }
 void resized()override{
  title.setBounds(24,16,950,44);prompt.setBounds(24,76,950,70);
  provider.setBounds(24,164,160,32);key.setBounds(198,164,70,32);scale.setBounds(282,164,110,32);style.setBounds(406,164,140,32);bars.setBounds(560,164,70,32);bpm.setBounds(644,164,330,32);
  generate.setBounds(24,214,220,40);variation.setBounds(260,214,130,40);play.setBounds(406,214,140,40);sound.setBounds(560,214,160,40);exportButton.setBounds(744,214,230,40);status.setBounds(24,620,950,40);
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff151820));g.setColour(juce::Colour(0xff242936));g.fillRoundedRectangle(24,278,950,330,8);
  for(int i=0;i<=16;++i){g.setColour(juce::Colour(i%4?0xff303646:0xff4c566a));g.drawVerticalLine(24+i*950/16,278,608);}
  auto idea=processor.getIdea();auto layers=idea["layers"];int ch=0;const juce::Colour colours[]={juce::Colour(0xffb494ff),juce::Colour(0xff53d9ba),juce::Colour(0xffffbc70)};
  double beats=double(idea["metadata"]["bars"])*4;if(beats<=0){g.setColour(juce::Colours::white);g.drawText("Built-in demo ready. Generate ideas to view the piano roll.",40,420,900,30,juce::Justification::centred);return;}
  for(const char* name:{"bass","chords","melody"}){auto list=layers[name];if(auto* notes=list.getArray())for(const auto& n:*notes){g.setColour(colours[ch]);float x=24+float(double(n["start"])/beats*950),w=juce::jmax(2.0f,float(double(n["duration"])/beats*950));float y=590-float(int(n["pitch"])-24)*4;g.fillRoundedRectangle(x,juce::jlimit(280.0f,602.0f,y),w,4,1);}++ch;}
 }
private:
 Processor& processor;juce::Label title,status;juce::TextEditor prompt;juce::ComboBox provider,key,scale,style,bars,variation;juce::Slider bpm;juce::TextButton generate,play,exportButton;juce::ToggleButton sound;std::unique_ptr<juce::FileChooser> chooser;
 void setup(juce::ComboBox& box,std::initializer_list<const char*> values){int i=1;for(auto v:values)box.addItem(v,i++);box.setSelectedId(1);addAndMakeVisible(box);}
 void timerCallback()override{status.setText(processor.getStatus(),juce::dontSendNotification);generate.setEnabled(!processor.busy);int count=processor.ideaCount();if(count!=variation.getNumItems()){variation.clear();for(int i=0;i<count;++i)variation.addItem("IDEA "+juce::String(i+1),i+1);variation.setSelectedId(1,juce::dontSendNotification);}repaint();}
 void exportMidi(){auto idea=processor.getIdea();if(!idea.isObject()){status.setText("Generate an idea before export",juce::dontSendNotification);return;}
  chooser=std::make_unique<juce::FileChooser>("Export MIDI",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("loopforge.mid"),"*.mid");
  chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe=juce::Component::SafePointer<Editor>(this),idea](const juce::FileChooser& fc){if(!safe||fc.getResult()==juce::File{})return;
   juce::MidiFile file;file.setTicksPerQuarterNote(480);juce::MidiMessageSequence tempo;tempo.addEvent(juce::MidiMessage::tempoMetaEvent(int(60000000.0/double(idea["metadata"]["bpm"]))));file.addTrack(tempo);int ch=1;
   for(const char* name:{"bass","chords","melody"}){juce::MidiMessageSequence track;track.addEvent(juce::MidiMessage::textMetaEvent(3,name));auto list=idea["layers"][name];if(auto* notes=list.getArray())for(const auto& n:*notes){auto on=juce::MidiMessage::noteOn(ch,int(n["pitch"]),(juce::uint8)int(n["velocity"]));on.setTimeStamp(double(n["start"])*480);track.addEvent(on);auto off=juce::MidiMessage::noteOff(ch,int(n["pitch"]));off.setTimeStamp((double(n["start"])+double(n["duration"]))*480);track.addEvent(off);}file.addTrack(track);++ch;}
   juce::MemoryOutputStream bytes;bool ok=file.writeTo(bytes,1);if(ok)ok=fc.getResult().replaceWithData(bytes.getData(),bytes.getDataSize());safe->status.setText(ok?"MIDI exported":"MIDI export failed",juce::dontSendNotification);
  });
 }
};
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor();}
