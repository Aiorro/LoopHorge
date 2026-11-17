#include "Processor.h"
#include "MidiExport.h"
#include "PianoRoll.h"
#include "MidiDragSource.h"
#include "ComposerOverview.h"
#include "ProjectFiles.h"
#include "ComposerMidiExport.h"
#include "ComposerMoodControls.h"
#include "engine/RecipeJson.h"
class Editor final: public juce::AudioProcessorEditor,private juce::Timer {
public:
 Editor(Processor& p):AudioProcessorEditor(p),processor(p),roll(p),dragMidi([&p]{return p.snapshot().idea;},DragMidiCache(DragMidiCache::defaultRoot()),MidiDragSource::nativeDispatch,[safe=juce::Component::SafePointer<Editor>(this)](juce::String message){if(safe)safe->showDragStatus(std::move(message));}){
  setSize(1000,740);setResizable(true,true);setResizeLimits(1000,740,1600,1100);
  composerNotice.setText("Read-only project - MIDI output only.\nHost PPQ/BPM controls playback; standalone follows the tempo map.",juce::dontSendNotification);composerNotice.setReadOnly(true);composerNotice.setMultiLine(true);composerNotice.setScrollbarsShown(true);composerNotice.setFont(juce::FontOptions(12.0f));composerNotice.setName("Resolved Composer settings");composerNotice.setTitle("Committed Composer settings and origins; scroll for details");addChildComponent(composerNotice);
  title.setText("LOOPFORGE  /  AI MIDI",juce::dontSendNotification);title.setFont(juce::FontOptions(26.0f));addAndMakeVisible(title);
  openProject.setButtonText("OPEN PROJECT");saveProject.setButtonText("SAVE PROJECT");addAndMakeVisible(openProject);addAndMakeVisible(saveProject);
  openProject.setTitle("Open project JSON");saveProject.setTitle("Save project JSON");openProject.setTooltip("Open a full LoopForge v1/v2 project JSON. This replaces the committed project only after validation.");saveProject.setTooltip("Save the committed project captured when clicked. Composer v2 requires a current plugin version.");openProject.onClick=[this]{chooseProject(false);};saveProject.onClick=[this]{chooseProject(true);};
  mode.setName("Composer mode");compose.setName("Compose offline loop");seed.setName("Composer fallback seed");prompt.setName("Musical direction");
  openRecipe.setButtonText("OPEN RECIPE");saveRecipe.setButtonText("SAVE RECIPE");addChildComponent(openRecipe);addChildComponent(saveRecipe);openRecipe.onClick=[this]{chooseRecipe(false);};saveRecipe.onClick=[this]{chooseRecipe(true);};
  moodSettings.setName("Composer mood settings");moodSettings.setButtonText("MOOD / AUTO");moodSettings.setTooltip("Seven next-request mood settings; Auto follows text/defaults. Save/load separately with recipe files.");addChildComponent(moodSettings);addChildComponent(moodControls);moodSettings.onClick=[this]{showMood=!showMood;timerCallback();};
  compose.setButtonText("COMPOSE OFFLINE LOOP");compose.setTooltip("Native deterministic4/8/16bar loop. Text/defaults choose Composer parameters; no service. MIDI output only.");addChildComponent(compose);compose.onClick=[this]{composeLoop();};
  addAndMakeVisible(mode);mode.onClick=[this]{if(processor.canSwitchMode()){processor.selectMode(processor.isComposerMode()?composer::ProjectMode::legacy:composer::ProjectMode::composer);timerCallback();}else composeLoop();};mode.setTooltip("Create a native Composer loop or switch between both preserved source documents. Switching cancels an active generation.");
  // Preserve complete insertions: filtering characters/length can change the requested seed.
  seed.setText("42");seed.setTooltip("Fallback unsigned32bit seed (0-4294967295). An explicit seed in direction text wins. Next request only; SAVE RECIPE stores this value separately.");addAndMakeVisible(seed);caption(seedLabel,"Composer seed",seed);
  prompt.setTooltip("Composer rejects directions above4096UTF8bytes without truncation. Composer uses this text, fallback seed and manual mood pins; Auto follows text/defaults. Legacy uses the controls below. SAVE RECIPE stores next-request text, seed and mood pins separately.");
  prompt.setMultiLine(true);prompt.setText("Dark atmospheric nu-metal, coherent bass and melody");addAndMakeVisible(prompt);
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
  exportButton.setButtonText("EXPORT MIDI");exportButton.setTooltip("Export the composition captured when clicked. Composer MIDI is prepared in the background with its tempo, meter, tracks and controllers.");addAndMakeVisible(exportButton);exportButton.onClick=[this]{exportMidi();};
  setup(layer,{"bass","chords","melody"});layer.onChange=[this]{roll.selectLayer(layer.getText());viewport.setViewPosition(0,roll.preferredScrollY());};
  quantize.setButtonText("QUANTIZE 1/4 BEAT");undo.setButtonText("UNDO");redo.setButtonText("REDO");
  for(auto* button:{&quantize,&undo,&redo})addAndMakeVisible(*button);
  quantize.onClick=[this]{processor.quantize(processor.snapshot().revision,layer.getText());roll.refresh();};undo.onClick=[this]{processor.undoEdit();roll.refresh();};redo.onClick=[this]{processor.redoEdit();roll.refresh();};
  addNote.setButtonText("ADD NOTE");deleteNote.setButtonText("DELETE NOTE");addAndMakeVisible(addNote);addAndMakeVisible(deleteNote);addNote.onClick=[this]{roll.addNote();};deleteNote.onClick=[this]{roll.deleteSelected();};
  caption(promptLabel,"MUSICAL DIRECTION",prompt);caption(providerLabel,"Provider",provider);caption(keyLabel,"Root note",key);caption(scaleLabel,"Scale",scale);caption(styleLabel,"Style",style);caption(barsLabel,"Length (bars)",bars);caption(tempoLabel,"Tempo",bpm);caption(modelLabel,"Model (optional)",model);variation.setTitle("Idea");layer.setTitle("Edit layer");
  help.setText("Brackets select notes | Arrows move | Shift + Left/Right resize | Delete removes | Double-click adds",juce::dontSendNotification);help.setFont(juce::FontOptions(11.0f));help.setColour(juce::Label::textColourId,juce::Colours::lightgrey);addAndMakeVisible(help);
  int focusOrder=1;for(juce::Component* component:std::initializer_list<juce::Component*>{&mode,&openProject,&saveProject,&seed,&prompt,&compose,&provider,&key,&scale,&style,&bars,&bpm,&generate,&variation,&play,&sound,&exportButton,&dragMidi,&model,&testConnection,&cancel,&layer,&quantize,&undo,&redo,&addNote,&deleteNote,&composerNotice,&roll,&overview})component->setExplicitFocusOrder(focusOrder++);
  viewport.setViewedComponent(&roll,false);viewport.setScrollBarsShown(true,false);addAndMakeVisible(viewport);resized();viewport.setViewPosition(0,roll.preferredScrollY());
  layer.setTooltip("Double-click to add; drag to move; drag right edge to resize; right-click to delete. Scroll for pitch.");
  refreshComposerDraft();prompt.onTextChange=[this]{syncComposerDraft();};seed.onTextChange=[this]{syncComposerDraft();};moodControls.onChange=[this]{syncComposerDraft();};
  addAndMakeVisible(status);status.setColour(juce::Label::textColourId,juce::Colours::lightgrey);timerCallback();startTimerHz(5);
 }
 ~Editor()override{stopTimer();try{syncComposerDraft();}catch(const std::bad_alloc&){}}
 void resized()override{
  const int width=getWidth(),height=getHeight(),content=width-48;
  composerNotice.setBounds(24,196,content,44);title.setBounds(24,16,width-650,40);seedLabel.setBounds(width-614,10,90,14);seed.setBounds(width-614,26,90,26);mode.setBounds(width-510,22,160,30);openProject.setBounds(width-336,22,148,30);saveProject.setBounds(width-176,22,150,30);promptLabel.setBounds(24,58,content,16);prompt.setBounds(24,76,content,70);
  provider.setBounds(24,164,160,32);key.setBounds(198,164,70,32);scale.setBounds(282,164,110,32);style.setBounds(406,164,140,32);bars.setBounds(560,164,70,32);bpm.setBounds(644,164,width-668,32);
  for(auto pair:{std::pair<juce::Label*,juce::Component*>{&providerLabel,&provider},{&keyLabel,&key},{&scaleLabel,&scale},{&styleLabel,&style},{&barsLabel,&bars},{&tempoLabel,&bpm}})pair.first->setBounds(pair.second->getX(),148,pair.second->getWidth(),15);
  generate.setBounds(24,214,220,40);variation.setBounds(260,214,130,40);play.setBounds(406,214,140,40);sound.setBounds(560,214,160,40);exportButton.setBounds(width-256,214,110,40);dragMidi.setBounds(width-136,214,110,40);
  cancel.setBounds(width-256,274,230,32);testConnection.setBounds(width-500,274,220,32);model.setBounds(24,274,width-538,32);modelLabel.setBounds(24,258,model.getWidth(),15);
  layer.setBounds(24,320,160,30);quantize.setBounds(200,320,190,30);undo.setBounds(406,320,140,30);redo.setBounds(560,320,140,30);addNote.setBounds(width-284,320,120,30);deleteNote.setBounds(width-146,320,120,30);
  const bool composer=viewport.getViewedComponent()==&overview;const int top=composer?246:360;
  if(composer){compose.setBounds(24,154,220,36);play.setBounds(260,154,140,36);exportButton.setBounds(416,154,150,36);moodSettings.setBounds(582,154,150,36);moodControls.setBounds(24,246,content,308);openRecipe.setBounds(24,570,160,32);saveRecipe.setBounds(200,570,160,32);cancel.setBounds(width-176,154,150,32);}
  viewport.setBounds(24,top,content,height-top-72);roll.setSize(viewport.getMaximumVisibleWidth(),128*12);overview.setSize(overview.getWidth(),overview.requiredHeight());overview.setSize(viewport.getMaximumVisibleWidth(),overview.requiredHeight());status.setBounds(24,height-60,content,32);help.setBounds(24,height-25,content,20);
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff151820));g.setColour(juce::Colour(0xff242936));g.fillRoundedRectangle(viewport.getBounds().toFloat(),8);

 }
private:
 Processor& processor;ComposerMoodControls moodControls;juce::TextButton moodSettings;juce::TextButton openRecipe,saveRecipe;bool showMood=false,applyingDraft=false;uint64_t draftEpoch=UINT64_MAX;std::string renderedDraftJson;ComposerMidiExport composerMidiExport;bool fileChoosing=false;PianoRoll roll;ComposerOverview overview;uint64_t viewRevision=UINT64_MAX;MidiDragSource dragMidi;juce::String dragStatus;juce::uint32 dragStatusUntil=0;juce::Viewport viewport;juce::ComboBox layer;juce::TextButton quantize,undo,redo,addNote,deleteNote;juce::Label seedLabel,title,status,help,promptLabel,providerLabel,keyLabel,scaleLabel,styleLabel,barsLabel,tempoLabel,modelLabel;juce::TextEditor prompt,model,seed,composerNotice;juce::ComboBox provider,key,scale,style,bars,variation;juce::Slider bpm;juce::TextButton mode,compose,generate,play,exportButton,testConnection,cancel,openProject,saveProject;juce::ToggleButton sound;std::unique_ptr<juce::FileChooser> chooser;
 void refreshComposerDraft(){
  if(auto state=processor.composerDraft(draftEpoch)){
   draftEpoch=state->epoch;juce::ScopedValueSetter<bool> guard(applyingDraft,true);
   composer::GenerationRecipe recipe;recipe.direction="Dark atmospheric nu-metal, coherent bass and melody";
   if(state->json){auto decoded=composer::decodeRecipeJson(*state->json);if(!decoded.recipe)return;recipe=std::move(*decoded.recipe);}
   prompt.setText(juce::String::fromUTF8(recipe.direction.data(),int(recipe.direction.size())),false);seed.setText(juce::String(recipe.seed),false);moodControls.apply(recipe.manual);auto encoded=composer::encodeRecipeJson(recipe);if(encoded)renderedDraftJson=std::move(encoded.json);
  }
 }
 void syncComposerDraft(){
  if(applyingDraft)return;
  const auto text=seed.getText().toStdString();uint32_t value=0;auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
  if(text.empty()||parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||prompt.getText().getNumBytesAsUTF8()>4096)return;
  composer::GenerationRecipe recipe{prompt.getText().toStdString(),value,moodControls.overrides()};auto encoded=composer::encodeRecipeJson(recipe);if(!encoded||encoded.json==renderedDraftJson)return;
  if(processor.setComposerDraft(recipe,draftEpoch)){renderedDraftJson=std::move(encoded.json);return;}
  {auto old=draftEpoch;refreshComposerDraft();if(old==draftEpoch)showDragStatus("Settings cannot be saved within this project metadata budget");}
 }
 void composeLoop(){
  refreshComposerDraft();
  const auto text=seed.getText().toStdString();uint32_t value=0;auto read=std::from_chars(text.data(),text.data()+text.size(),value);
  if(text.empty()||read.ec!=std::errc{}||read.ptr!=text.data()+text.size()){showDragStatus("Composer seed must be an integer0-4294967295");seed.grabKeyboardFocus();return;}
  syncComposerDraft();dragStatus.clear();processor.requestComposer(prompt.getText(),value,moodControls.overrides());timerCallback();
 }
 juce::String requestBody(bool probe){auto* o=new juce::DynamicObject;
  o->setProperty("provider",provider.getText());o->setProperty("key",key.getText());o->setProperty("mode",scale.getText());o->setProperty("style",style.getText());o->setProperty("bars",bars.getText().getIntValue());o->setProperty("bpm",int(bpm.getValue()));o->setProperty("description",prompt.getText());o->setProperty("count",8);
  if(model.getText().isNotEmpty())o->setProperty("model",model.getText().trim());if(probe)o->setProperty("action","probe");return juce::JSON::toString(juce::var(o));
 }
 void caption(juce::Label& label,const juce::String& text,juce::Component& control){label.setText(text,juce::dontSendNotification);label.setFont(juce::FontOptions(11.0f));label.setColour(juce::Label::textColourId,juce::Colours::lightgrey);control.setTitle(text);addAndMakeVisible(label);}
 void setup(juce::ComboBox& box,std::initializer_list<const char*> values){int i=1;for(auto v:values)box.addItem(v,i++);box.setSelectedId(1);addAndMakeVisible(box);}
 void showDragStatus(juce::String message){dragStatus=std::move(message);dragStatusUntil=juce::Time::getMillisecondCounter()+5000;status.setText(dragStatus,juce::dontSendNotification);}
 void refreshPresentation(){
  if(auto state=processor.presentation(viewRevision)){viewRevision=state->revision;saveProject.setEnabled(state->hasState);
   if(state->composer){auto project=composer::decodeProjectJson(state->json);if(project.project&&project.project->composition)overview.setDocument(*project.project->composition,project.project->hostStartQuarterBeat);}
   juce::Component* target=state->composer?static_cast<juce::Component*>(&overview):static_cast<juce::Component*>(&roll);
   const bool changed=viewport.getViewedComponent()!=target;if(changed)viewport.setViewedComponent(target,false);resized();if(changed)viewport.setViewPosition(0,state->composer?0:roll.preferredScrollY());
  }
 }
 void chooseRecipe(bool saving){
  if(fileChoosing)return;
  try{
   auto bytes=std::make_shared<std::string>();
   if(saving){const auto text=seed.getText().toStdString();uint32_t value=0;auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);if(text.empty()||parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()){showDragStatus("Recipe seed must be an unsigned32bit integer");return;}
    auto encoded=composer::encodeRecipeJson({prompt.getText().toStdString(),value,moodControls.overrides()});if(!encoded){showDragStatus(juce::String(encoded.error));return;}*bytes=std::move(encoded.json);
   }
   chooser=std::make_unique<juce::FileChooser>(saving?"Save Composer recipe":"Open Composer recipe",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("loopforge-recipe.json"),"*.json");
   fileChoosing=true;chooser->launchAsync((saving?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode)|juce::FileBrowserComponent::canSelectFiles,[safe=juce::Component::SafePointer<Editor>(this),saving,bytes](const juce::FileChooser& fc){
    if(!safe)return;safe->fileChoosing=false;if(fc.getResult()==juce::File{})return;
    try{if(saving){auto error=projectFiles::replaceChecked(fc.getResult(),*bytes);safe->showDragStatus(error.isEmpty()?"Recipe saved":error);return;}
     auto file=projectFiles::read(fc.getResult(),32768);if(file.error.isNotEmpty()){safe->showDragStatus(file.error);return;}auto decoded=composer::decodeRecipeJson(file.text);if(!decoded.recipe){safe->showDragStatus(juce::String(decoded.error));return;}
     safe->prompt.setText(juce::String::fromUTF8(decoded.recipe->direction.data(),int(decoded.recipe->direction.size())),false);safe->seed.setText(juce::String(decoded.recipe->seed),false);safe->moodControls.apply(decoded.recipe->manual);safe->syncComposerDraft();safe->showDragStatus("Recipe opened - Compose to generate; current notes unchanged");
    }catch(const std::bad_alloc&){safe->showDragStatus("Insufficient memory for recipe");}
   });
  }catch(const std::bad_alloc&){fileChoosing=false;showDragStatus("Insufficient memory for recipe");}
 }
 void chooseProject(bool saving){
  if(fileChoosing)return;
  try{
  auto captured=std::make_shared<juce::MemoryBlock>();if(saving)processor.getStateInformation(*captured);
  chooser=std::make_unique<juce::FileChooser>(saving?"Save LoopForge project":"Open LoopForge project",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("loopforge-project.json"),"*.json");
  const int flags=saving?(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting):(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles);
  fileChoosing=true;chooser->launchAsync(flags,[safe=juce::Component::SafePointer<Editor>(this),saving,captured](const juce::FileChooser& fc){
   if(!safe)return;safe->fileChoosing=false;if(fc.getResult()==juce::File{})return;
   if(saving){auto error=projectFiles::write(fc.getResult(),{static_cast<const char*>(captured->getData()),captured->getSize()});safe->showDragStatus(error.isEmpty()?"Project saved":error);}
   else{auto file=projectFiles::read(fc.getResult());if(file.error.isNotEmpty()){safe->showDragStatus(file.error);return;}const bool restored=safe->processor.restoreStateInformation(file.text.data(),int(file.text.size()));if(restored)safe->processor.updateHostDisplay(juce::AudioProcessorListener::ChangeDetails{}.withNonParameterStateChanged(true));safe->showDragStatus(restored?"Project opened":safe->processor.getStatus());safe->refreshComposerDraft();safe->refreshPresentation();}
  });
  }catch(const std::bad_alloc&){fileChoosing=false;showDragStatus("Insufficient memory to choose a project file");}
 }
 void timerCallback()override{refreshComposerDraft();pollComposerMidi();refreshPresentation();status.setText(dragStatus.isNotEmpty()&&int(juce::Time::getMillisecondCounter()-dragStatusUntil)<0?dragStatus:processor.getStatus(),juce::dontSendNotification);const bool legacy=viewport.getViewedComponent()!=&overview;saveProject.setEnabled(processor.hasSavableState());
  for(auto* component:std::initializer_list<juce::Component*>{&provider,&providerLabel,&key,&keyLabel,&scale,&scaleLabel,&style,&styleLabel,&bars,&barsLabel,&bpm,&tempoLabel,&generate,&variation,&sound,&dragMidi,&model,&modelLabel,&testConnection,&layer,&quantize,&undo,&redo,&addNote,&deleteNote})component->setVisible(legacy);
  prompt.setVisible(true);promptLabel.setVisible(true);compose.setVisible(!legacy);compose.setEnabled(!processor.busy);
  mode.setButtonText(legacy?(processor.canSwitchMode()?"VIEW COMPOSER":"COMPOSE LOOP"):"BACK TO LEGACY");mode.setEnabled(processor.canSwitchMode()||(legacy&&!processor.busy));
  auto summary=processor.composerSummary();composerNotice.setText(summary.isEmpty()?"MIDI output and export; read-only notes. Direction/seed describe the next loop, not the loaded project.":"Committed loop - "+summary,juce::dontSendNotification);composerNotice.setTooltip(summary.isEmpty()?"Native loops:4/8/16bars; unsupported song/sections/roles reject. MIDI export is available; editing and audition remain unavailable.":summary+". MIDI output and export; read-only notes. Direction/seed apply to the next request.");
  moodSettings.setVisible(!legacy);moodSettings.setButtonText(showMood?"SHOW OVERVIEW":"MOOD / AUTO");moodControls.setVisible(!legacy&&showMood);openRecipe.setVisible(!legacy&&showMood);saveRecipe.setVisible(!legacy&&showMood);viewport.setVisible(legacy||!showMood);
  composerNotice.setVisible(!legacy);cancel.setVisible(legacy||processor.busy);generate.setEnabled(legacy&&!processor.busy);testConnection.setEnabled(legacy&&!processor.busy);sound.setEnabled(legacy);exportButton.setVisible(true);exportButton.setEnabled(!composerMidiExport.busy()&&!fileChoosing);quantize.setEnabled(legacy);layer.setEnabled(legacy);roll.setEnabled(legacy);prompt.setEnabled(true);provider.setEnabled(legacy);key.setEnabled(legacy);scale.setEnabled(legacy);style.setEnabled(legacy);bars.setEnabled(legacy);bpm.setEnabled(legacy);model.setEnabled(legacy&&provider.getText()!="offline");title.setText(legacy?"LOOPFORGE  /  AI MIDI":"COMPOSER  /  MIDI ONLY",juce::dontSendNotification);status.setTooltip(processor.getStatus());help.setText(legacy?"Brackets select notes | Arrows move | Shift + Left/Right resize | Delete removes | Double-click adds":"Home/End and Page Up/Down scroll | Read-only MIDI overview | v2 requires a current plugin version",juce::dontSendNotification);cancel.setEnabled(processor.busy);int count=processor.ideaCount();dragMidi.setEnabled(count>0);addNote.setEnabled(count>0);if(count!=variation.getNumItems()){variation.clear();for(int i=0;i<count;++i)variation.addItem("IDEA "+juce::String(i+1),i+1);variation.setSelectedId(1,juce::dontSendNotification);}if(count>0)variation.setSelectedItemIndex(processor.selectedIdeaIndex(),juce::dontSendNotification);if(roll.refresh()&&legacy)viewport.setViewPosition(0,roll.preferredScrollY());deleteNote.setEnabled(roll.hasSelection());undo.setEnabled(processor.canUndo());redo.setEnabled(processor.canRedo());repaint();}
 void pollComposerMidi(){
  if(fileChoosing)return;auto result=composerMidiExport.takeResult();if(!result)return;
  if(!*result){showDragStatus(juce::String(result->error));return;}
  try{
   auto bytes=std::make_shared<const std::vector<uint8_t>>(std::move(result->bytes));
   chooser=std::make_unique<juce::FileChooser>("Export Composer MIDI",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("loopforge.mid"),"*.mid");
   fileChoosing=true;chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe=juce::Component::SafePointer<Editor>(this),bytes](const juce::FileChooser& fc){
    if(!safe)return;safe->fileChoosing=false;if(fc.getResult()==juce::File{})return;
    try{const auto error=projectFiles::replaceChecked(fc.getResult(),{reinterpret_cast<const char*>(bytes->data()),bytes->size()});safe->showDragStatus(error.isEmpty()?"MIDI exported":error);}
    catch(const std::bad_alloc&){safe->showDragStatus("Insufficient memory to save MIDI");}
   });
  }catch(const std::bad_alloc&){fileChoosing=false;showDragStatus("Insufficient memory for MIDI export");}
 }
 void exportMidi(){
  if(fileChoosing||composerMidiExport.busy())return;
  try{auto captured=processor.presentation(UINT64_MAX);if(captured&&captured->composer){
    if(composerMidiExport.request(std::move(captured->json))){exportButton.setEnabled(false);showDragStatus("Preparing Composer MIDI...");}
    else showDragStatus("Cannot start MIDI export");return;
   }
  }catch(const std::bad_alloc&){showDragStatus("Insufficient memory for MIDI export");return;}
  try{auto idea=processor.getIdea();if(!idea.isObject()){status.setText("Generate an idea before export",juce::dontSendNotification);return;}
  chooser=std::make_unique<juce::FileChooser>("Export MIDI",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("loopforge.mid"),"*.mid");
  fileChoosing=true;chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe=juce::Component::SafePointer<Editor>(this),idea](const juce::FileChooser& fc){if(!safe)return;safe->fileChoosing=false;if(fc.getResult()==juce::File{})return;
   try{auto bytes=encodeMidi(idea);bool ok=bytes.getSize()>0&&projectFiles::replaceChecked(fc.getResult(),{static_cast<const char*>(bytes.getData()),bytes.getSize()}).isEmpty();safe->status.setText(ok?"MIDI exported":"MIDI export failed",juce::dontSendNotification);}
   catch(const std::bad_alloc&){safe->showDragStatus("Insufficient memory to save MIDI");}
  });
  }catch(const std::bad_alloc&){fileChoosing=false;showDragStatus("Insufficient memory for MIDI export");}
 }
};
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor();}
