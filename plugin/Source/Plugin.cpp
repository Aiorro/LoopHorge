#include "Processor.h"
#include "MidiExport.h"
#include "PianoRoll.h"
#include "MidiDragSource.h"
#include "ComposerOverview.h"
#include "ProjectFiles.h"
#include "ComposerMidiExport.h"
#include "ComposerMoodControls.h"
#include "engine/RecipeJson.h"
#include "StudioLook.h"
#include "LoopSignal.h"
class Editor final: public juce::AudioProcessorEditor,private juce::Timer {
public:
 Editor(Processor& p):AudioProcessorEditor(p),processor(p),roll(p),dragMidi([&p]{return p.snapshot().idea;},DragMidiCache(DragMidiCache::defaultRoot()),MidiDragSource::nativeDispatch,[safe=juce::Component::SafePointer<Editor>(this)](juce::String message){if(safe)safe->showDragStatus(std::move(message));}){
  setLookAndFeel(&look);setResizable(true,true);setResizeLimits(1000,740,1600,1100);
  composerNotice.setText("Read-only project - MIDI output only.\nHost PPQ/BPM controls playback; standalone follows the tempo map.",juce::dontSendNotification);composerNotice.setReadOnly(true);composerNotice.setMultiLine(true,true);composerNotice.setCaretVisible(false);composerNotice.setIndents(8,8);composerNotice.setScrollbarsShown(true);composerNotice.setFont(juce::FontOptions(12.0f));composerNotice.setName("Resolved Composer settings");composerNotice.setTitle("Committed Composer settings and origins; scroll for details");addChildComponent(composerNotice);
  title.setText("LoopForge",juce::dontSendNotification);title.setFont(studio::font(26.0f,juce::Font::bold));title.setBorderSize({0,0,0,0});addAndMakeVisible(title);
  identity.setText(p.wrapperType==juce::AudioProcessor::wrapperType_Standalone?"STANDALONE MIDI STUDIO":"MIDI LOOP STUDIO",juce::dontSendNotification);
  identity.setFont(studio::font(11.0f,juce::Font::bold));identity.setBorderSize({0,0,0,0});identity.setColour(juce::Label::textColourId,studio::muted);addAndMakeVisible(identity);
  openProject.setButtonText("OPEN PROJECT");saveProject.setButtonText("SAVE PROJECT");addAndMakeVisible(openProject);addAndMakeVisible(saveProject);
  openProject.setTitle("Open project JSON");saveProject.setTitle("Save project JSON");openProject.setTooltip("Open a full LoopForge v1/v2 project JSON. This replaces the committed project only after validation.");saveProject.setTooltip("Save the committed project captured when clicked. Composer v2 requires a current plugin version.");openProject.onClick=[this]{chooseProject(false);};saveProject.onClick=[this]{chooseProject(true);};
  addAndMakeVisible(signal);
  signal.onTrackSelected=[this](int index){
   if (processor.compositionRevision()!=viewRevision || index>=layer.getNumItems() ||
       (showComposer && !processor.canEditComposer())) return;
   layer.setSelectedItemIndex(index,juce::dontSendNotification);
   roll.selectLayer(activeTrack(),false);
   if (showComposer) setNoteView(true);
   else signal.setActiveTrack(index);
   viewport.setViewPosition(0,roll.preferredScrollY());
  };
  ideaSettings.setName("Idea bank settings");ideaSettings.setButtonText("IDEA SETTINGS");addAndMakeVisible(ideaSettings);
  ideaSettings.onClick=[this]{showIdeaSettings=!showIdeaSettings;timerCallback();repaint(signal.getBounds());};
  loopDetails.setName("Loop details");loopDetails.setButtonText("SHOW LOOP DETAILS");addChildComponent(loopDetails);
  loopDetails.onClick=[this]{showDetails=!showDetails;timerCallback();repaint(signal.getBounds());};
  mode.setName("Composer mode");compose.setName("Compose offline loop");seed.setName("Composer fallback seed");prompt.setName("Musical direction");
  openRecipe.setButtonText("OPEN RECIPE");saveRecipe.setButtonText("SAVE RECIPE");addChildComponent(openRecipe);addChildComponent(saveRecipe);openRecipe.onClick=[this]{chooseRecipe(false);};saveRecipe.onClick=[this]{chooseRecipe(true);};
  moodSettings.setName("Composer mood settings");moodSettings.setButtonText("MOOD SETTINGS");moodSettings.setTooltip("Seven next-request mood settings; Auto follows text/defaults. Save/load separately with recipe files.");addChildComponent(moodSettings);addChildComponent(moodControls);moodSettings.onClick=[this]{showMood=!showMood;timerCallback();repaint();};
  compose.setButtonText("COMPOSE OFFLINE LOOP");compose.setTooltip("Native deterministic4/8/16bar loop. Text/defaults choose Composer parameters; no service. MIDI output and basic preview.");addChildComponent(compose);compose.onClick=[this]{composeLoop();};
  addAndMakeVisible(mode);mode.onClick=[this]{if(processor.canSwitchMode()){processor.selectMode(processor.isComposerMode()?composer::ProjectMode::legacy:composer::ProjectMode::composer);timerCallback();}else composeLoop();};mode.setTooltip("Create a native Composer loop or switch between both preserved source documents. Switching cancels an active generation.");
  // Preserve complete insertions: filtering characters/length can change the requested seed.
  seed.setFont(studio::font(15.0f));seed.setIndents(9,8);seed.setText("42");seed.setTooltip("Fallback unsigned32bit seed (0-4294967295). An explicit seed in direction text wins. Next request only; SAVE RECIPE stores this value separately.");addAndMakeVisible(seed);caption(seedLabel,"Seed",seed);
  prompt.setTooltip("Composer rejects directions above4096UTF8bytes without truncation. Composer uses this text, fallback seed and manual mood pins; Auto follows text/defaults. Legacy uses the controls below. SAVE RECIPE stores next-request text, seed and mood pins separately.");
  prompt.setFont(studio::font(15.0f));prompt.setIndents(9,8);prompt.setMultiLine(true);prompt.setText(defaultDirection);addAndMakeVisible(prompt);
  setup(engine,{"Rule Composer","MIDI-LLM"});
  engine.setTitle("Composition engine");
  addAndMakeVisible(modelSetup);modelSetup.setButtonText("MODEL SETUP");
  modelSetup.setTooltip("Check this computer and install the local MIDI-LLM model");
  modelSetup.onClick=[]{
   const auto launcher=midiLlm::setupLauncher();
   if(!launcher.existsAsFile() || !launcher.startAsProcess())
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"MIDI-LLM setup",
       "Open Setup MIDI-LLM.cmd in the MIDI-LLM folder of the extracted Windows ZIP. Standalone and VST3 share the installed model.");
  };
  refreshModelChoice();
  setup(provider,{"offline","openai","anthropic","openrouter","ollama","codex-cli","claude-cli"});
  providerStatus.setComponentID("idea-provider");
  providerStatus.setFont(studio::font(11.0f,juce::Font::bold));
  providerStatus.setColour(juce::Label::textColourId,studio::muted);
  providerStatus.setTooltip("Provider for Generate 8 Ideas");
  addChildComponent(providerStatus);
  setup(key,{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"});key.setSelectedId(3);
  setup(scale,{"minor","major","dorian","phrygian"});setup(style,{"nu-metal","trap","boom-bap","r&b","ambient"});setup(bars,{"4","8"});
  bpm.setRange(40,240,1);bpm.setValue(105);bpm.setTextValueSuffix(" BPM");addAndMakeVisible(bpm);
  model.setInputRestrictions(100);model.setTextToShowWhenEmpty("Default provider model",studio::muted);model.setTooltip("Optional model ID; sent only to the selected provider.");addAndMakeVisible(model);
  provider.onChange=[this]{model.clear();model.setEnabled(provider.getText()!="offline");timerCallback();};model.setEnabled(false);
  generate.setButtonText("GENERATE 8 IDEAS");addAndMakeVisible(generate);generate.onClick=[this]{processor.request(requestBody(false));};
  testConnection.setButtonText("TEST (1 IDEA)");testConnection.setTooltip("Generates one idea to check connection, login and model access. Cloud/CLI usage may incur charges. Keeps your current composition.");addAndMakeVisible(testConnection);testConnection.onClick=[this]{processor.request(requestBody(true));};
  cancel.setButtonText("CANCEL");cancel.setTitle("Cancel generation request");addAndMakeVisible(cancel);cancel.onClick=[this]{processor.cancel();};
  play.setButtonText("PLAY / STOP");play.setTooltip("Start or stop the local preview. Plugin playback follows the host transport.");addAndMakeVisible(play);play.onClick=[this]{processor.play=!processor.play.load();};
  sound.setButtonText("SOUND");sound.setTitle("Audition synth");sound.setTooltip("Use the built-in preview instrument.");sound.setToggleState(processor.audition.load(),juce::dontSendNotification);addAndMakeVisible(sound);sound.onClick=[this]{processor.audition=sound.getToggleState();};
  variation.setTextWhenNothingSelected("No ideas yet");addAndMakeVisible(variation);variation.onChange=[this]{processor.choose(variation.getSelectedItemIndex());repaint();};
  addAndMakeVisible(dragMidi);
  exportButton.setButtonText("EXPORT MIDI");exportButton.setTooltip("Export the composition captured when clicked. Composer MIDI is prepared in the background with its tempo, meter, tracks and controllers.");addAndMakeVisible(exportButton);exportButton.onClick=[this]{exportMidi();};
  setup(layer,{"bass","chords","melody"});layer.onChange=[this]{roll.selectLayer(activeTrack());signal.setActiveTrack(layer.getSelectedItemIndex());viewport.setViewPosition(0,roll.preferredScrollY());};
  noteView.setName("Composer note view");noteView.setButtonText("EDIT NOTES");addChildComponent(noteView);noteView.onClick=[this]{setNoteView(!showEdit);};
  regenerate.setButtonText("REGENERATE BARS");
  regenerate.setTooltip("Create an offline variation of selected bars on the active track");
  addChildComponent(regenerate);
  regenerate.onClick=[this]{chooseRegion();};
  quantize.setButtonText("QUANTIZE 1/4 BEAT");undo.setButtonText("UNDO");redo.setButtonText("REDO");
  for(auto* button:{&quantize,&undo,&redo})addAndMakeVisible(*button);
  quantize.onClick=[this]{processor.quantize(processor.compositionRevision(),activeTrack());roll.refresh();};undo.onClick=[this]{processor.undoEdit();roll.refresh();};redo.onClick=[this]{processor.redoEdit();roll.refresh();};
  addNote.setButtonText("ADD NOTE");deleteNote.setButtonText("DELETE NOTE");addAndMakeVisible(addNote);addAndMakeVisible(deleteNote);addNote.onClick=[this]{roll.addNote();};deleteNote.onClick=[this]{roll.deleteSelected();};
  caption(promptLabel,"Musical direction",prompt);caption(providerLabel,"Provider",provider);caption(keyLabel,"Root note",key);caption(scaleLabel,"Scale",scale);caption(styleLabel,"Style",style);caption(barsLabel,"Length (bars)",bars);caption(tempoLabel,"Tempo",bpm);caption(modelLabel,"Model (optional)",model);variation.setTitle("Idea");layer.setTitle("Edit layer");
  help.setText("Brackets select notes | Arrows move | Shift + Left/Right resize | Delete removes | Double-click adds",juce::dontSendNotification);help.setFont(juce::FontOptions(11.0f));help.setColour(juce::Label::textColourId,studio::muted);addAndMakeVisible(help);
  int focusOrder=1;
  for (auto* component : std::initializer_list<juce::Component*>{&prompt,&seed,&mode,&compose,&moodSettings,&loopDetails,&composerNotice,
       &ideaSettings,&provider,&key,&scale,&style,&bars,&bpm,&model,&generate,&testConnection,&cancel,&play,&signal,&sound,&variation,&noteView,
       &exportButton,&dragMidi,&layer,&quantize,&regenerate,&undo,&redo,&addNote,&deleteNote,&viewport,&moodControls,&openRecipe,&saveRecipe,
       &openProject,&saveProject})
   component->setExplicitFocusOrder(focusOrder++);
  viewport.setViewedComponent(&roll,false);viewport.setScrollBarsShown(true,false);viewport.setScrollBarThickness(9);addAndMakeVisible(viewport);setSize(1000,740);viewport.setViewPosition(0,roll.preferredScrollY());
  layer.setTooltip("Double-click to add; drag to move; drag right edge to resize; right-click to delete. Scroll for pitch.");
  refreshComposerDraft();prompt.onTextChange=[this]{syncComposerDraft();};seed.onTextChange=[this]{syncComposerDraft();};moodControls.onChange=[this]{syncComposerDraft();};
  addAndMakeVisible(status);status.setColour(juce::Label::textColourId,studio::text);status.setFont(studio::font(12.0f));timerCallback();startTimerHz(5);
 }
 ~Editor() override {
  stopTimer();
  try { syncComposerDraft(); } catch (const std::bad_alloc&) {}
  setLookAndFeel(nullptr);
 }

 void resized() override {
  const auto area = getLocalBounds().reduced(22, 0);
  title.setBounds(area.getX()+43, 10, 162, 32);
  identity.setBounds(title.getRight()+8, 18, 168, 17);
  engine.setBounds(area.getRight()-485,13,143,30);
  modelSetup.setBounds(area.getRight()-333,13,110,30);
  auto files = area.withY(13).withHeight(30).withTrimmedLeft(area.getWidth()-214);
  openProject.setBounds(files.removeFromLeft(103));
  files.removeFromLeft(8);
  saveProject.setBounds(files);

  const bool composer = showComposer;
  auto command = area.withY(74).withHeight(36);
  play.setBounds(command.removeFromRight(106));
  command.removeFromRight(8);
  const auto composeSlot = command.removeFromRight(180);
  compose.setBounds(composeSlot);
  command.removeFromRight(8);
  seed.setBounds(command.removeFromRight(108));
  seedLabel.setBounds(seed.getX(), 54, seed.getWidth(), 17);
  command.removeFromRight(8);
  prompt.setBounds(command);
  promptLabel.setBounds(command.getX(), 54, command.getWidth(), 17);

  const auto upper = area.withY(125).withHeight(137);
  signal.setBounds(upper);
  composerNotice.setBounds(upper.reduced(12, 10));
  auto options = upper.reduced(12, 10);
  int column = 0;
  for (const auto& [label, control] : std::array<std::pair<juce::Label*, juce::Component*>, 6>{{
        {&providerLabel, &provider}, {&keyLabel, &key}, {&scaleLabel, &scale},
        {&styleLabel, &style}, {&barsLabel, &bars}, {&tempoLabel, &bpm}}}) {
   const int left = options.getX()+options.getWidth()*column/6;
   const int right = options.getX()+options.getWidth()*(column+1)/6;
   label->setBounds(left, options.getY(), right-left-8, 17);
   control->setBounds(left, options.getY()+20, right-left-8, 31);
   ++column;
  }
  modelLabel.setBounds(options.getX(), options.getY()+58, 390, 17);
  model.setBounds(options.getX(), options.getY()+77, 390, 31);
  testConnection.setBounds(model.getRight()+10, model.getY(), 144, 31);

  auto tools = area.withY(270).withHeight(34);
  const auto place = [](juce::Rectangle<int>& row, juce::Component& control, int width) {
   control.setBounds(row.removeFromLeft(width));
   row.removeFromLeft(7);
  };
  // Both toolbars share the same column edges at every supported width.
  const auto cell = [area](int y, int column, int span) {
   const int pitch = area.getWidth()+8;
   const int left = column*pitch/12;
   const int right = (column+span)*pitch/12-8;
   return juce::Rectangle<int>(area.getX()+left, y, right-left, 34);
  };
  if (composer) {
   mode.setBounds(cell(270,0,2));
   moodSettings.setBounds(cell(270,2,2));
   loopDetails.setBounds(cell(270,4,2));
   noteView.setBounds(cell(270,6,2));
   cancel.setBounds(cell(270,8,1));
   sound.setBounds(cell(270,9,1));
   exportButton.setBounds(cell(270,10,2));
   ideaSettings.setBounds({});
   generate.setBounds({});
   variation.setBounds({});
   dragMidi.setBounds({});
  } else {
   mode.setBounds(composeSlot);
   place(tools, ideaSettings, 131);
   place(tools, generate, 148);
   place(tools, variation, 114);
   place(tools, providerStatus, 105);
   dragMidi.setBounds(tools.removeFromRight(104));
   tools.removeFromRight(7);
   noteView.setBounds({});
   moodSettings.setBounds({});
   loopDetails.setBounds({});
   exportButton.setBounds(tools.removeFromRight(111));
   tools.removeFromRight(7);
   sound.setBounds(tools.removeFromRight(78));
   tools.removeFromRight(7);
   cancel.setBounds(tools.removeFromRight(122));
  }

  constexpr int editY = 312;
  layer.setBounds(cell(editY,0,2));
  quantize.setBounds(cell(editY,2,2));
  undo.setBounds(cell(editY,4,1));
  redo.setBounds(cell(editY,5,1));
  regenerate.setBounds(cell(editY,6,2));
  addNote.setBounds(cell(editY,8,2));
  deleteNote.setBounds(cell(editY,10,2));

  const bool editVisible = !composer || showEdit;
  const int top = editVisible ? 371 : 314;
  viewport.setBounds(area.withTop(top).withTrimmedBottom(55));
  auto contentWidth = [&](int height) {
   return viewport.getWidth() - (height > viewport.getHeight() ? viewport.getScrollBarThickness() : 0);
  };
  const auto rollHeight = roll.hasTimeline() ? 128*12 : viewport.getHeight();
  roll.setSize(contentWidth(rollHeight), rollHeight);
  overview.setSize(contentWidth(overview.requiredHeight()), overview.requiredHeight());
  help.setBounds(area.withY(getHeight()-50).withHeight(19));

  moodControls.setBounds(area.getX(), 317, area.getWidth(), 256);
  openRecipe.setBounds(area.getX(), 587, 138, 32);
  saveRecipe.setBounds(area.getX()+148, 587, 138, 32);
  status.setBounds(area.getX()+16, getHeight()-27, area.getWidth()-16, 21);
 }

 void paint(juce::Graphics& g) override {
  g.fillAll(studio::background);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff242b2e), 0.0f, 0.0f,
                                        studio::background, 30.0f, 290.0f, false));
  g.fillRect(getLocalBounds().withHeight(300));
  const auto mark = juce::Rectangle<float>(22, 13, 31, 31);
  g.setGradientFill(juce::ColourGradient(studio::accent.brighter(0.18f), mark.getTopLeft(),
                                        juce::Colour(0xffa0cd4c), mark.getBottomRight(), false));
  g.fillRoundedRectangle(mark, 7.0f);
  g.setColour(studio::accentInk);
  for (int i=0; i<3; ++i) g.fillRoundedRectangle(29.0f+4.5f*i, 33.0f-5.0f*i, 8.0f, 3.0f, 1.5f);
  g.setColour(studio::border.withAlpha(0.8f));
  g.drawHorizontalLine(49, 22.0f, float(getWidth()-22));

  if (!signal.isVisible()) {
   const auto frame = signal.getBounds().toFloat().reduced(0.5f);
   g.setColour(studio::panel);
   g.fillRoundedRectangle(frame, 9.0f);
   g.setColour(studio::border);
   g.drawRoundedRectangle(frame, 9.0f, 1.0f);
  }
  if (viewport.isVisible()) {
   auto frame = viewport.getBounds();
   if (viewport.getViewedComponent() == &roll && roll.hasTimeline()) {
    roll.paintRuler(g, {frame.getX(), frame.getY()-20, roll.getWidth(), 20});
    frame = frame.withTop(frame.getY()-20);
   }
   g.setColour(studio::border);
   g.drawRect(frame.expanded(1), 1);
  }
  const float statusY = float(getHeight()-17);
  g.setColour(busyLayout ? studio::accent : studio::muted);
  g.fillEllipse(23.0f, statusY, 5.0f, 5.0f);
 }
private:
 studio::Look look;
 juce::Label identity, providerStatus;
 LoopSignal signal;
 juce::ComboBox engine;
 juce::TextButton modelSetup,regenerate;
 int regionBars=0;
 juce::uint32 modelCheckedAt=0;
 static constexpr auto defaultDirection = "8 bars D minor 105 bpm, dark atmospheric nu-metal with bass, chords and lead";
 bool busyLayout = false, showComposer = false, showDetails = false, showIdeaSettings = false;
 juce::TextButton loopDetails, ideaSettings;
 Processor& processor;ComposerMoodControls moodControls;juce::TextButton moodSettings;juce::TextButton openRecipe,saveRecipe;bool showMood=false,showEdit=false,applyingDraft=false;juce::TextButton noteView;juce::StringArray trackKeys;uint64_t draftEpoch=UINT64_MAX;std::string renderedDraftJson;ComposerMidiExport composerMidiExport;bool fileChoosing=false;PianoRoll roll;ComposerOverview overview;uint64_t viewRevision=UINT64_MAX;MidiDragSource dragMidi;juce::String dragStatus;juce::uint32 dragStatusUntil=0;juce::Viewport viewport;juce::ComboBox layer;juce::TextButton quantize,undo,redo,addNote,deleteNote;juce::Label seedLabel,title,status,help,promptLabel,providerLabel,keyLabel,scaleLabel,styleLabel,barsLabel,tempoLabel,modelLabel;juce::TextEditor prompt,model,seed,composerNotice;juce::ComboBox provider,key,scale,style,bars,variation;juce::Slider bpm;juce::TextButton mode,compose,generate,play,exportButton,testConnection,cancel,openProject,saveProject;juce::ToggleButton sound;std::unique_ptr<juce::FileChooser> chooser;
 void refreshComposerDraft(){
  if(auto state=processor.composerDraft(draftEpoch)){
   draftEpoch=state->epoch;juce::ScopedValueSetter<bool> guard(applyingDraft,true);
   composer::GenerationRecipe recipe;recipe.direction=defaultDirection;
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
 void refreshModelChoice(){
  modelCheckedAt=juce::Time::getMillisecondCounter();
  const bool ready=midiLlm::installed();
  engine.setItemEnabled(2,ready);
  if(!ready&&engine.getSelectedId()==2)engine.setSelectedId(1,juce::dontSendNotification);
 }
 void composeLoop(){
  refreshComposerDraft();
  const auto text=seed.getText().toStdString();uint32_t value=0;auto read=std::from_chars(text.data(),text.data()+text.size(),value);
  if(text.empty()||read.ec!=std::errc{}||read.ptr!=text.data()+text.size()){showDragStatus("Composer seed must be an integer0-4294967295");seed.grabKeyboardFocus();return;}
  syncComposerDraft();dragStatus.clear();processor.requestComposer(prompt.getText(),value,moodControls.overrides(),engine.getSelectedId()==2);timerCallback();
 }
 juce::String requestBody(bool probe){auto* o=new juce::DynamicObject;
  o->setProperty("provider",provider.getText());o->setProperty("key",key.getText());o->setProperty("mode",scale.getText());o->setProperty("style",style.getText());o->setProperty("bars",bars.getText().getIntValue());o->setProperty("bpm",int(bpm.getValue()));o->setProperty("description",prompt.getText());o->setProperty("count",8);
  if(model.getText().isNotEmpty())o->setProperty("model",model.getText().trim());if(probe)o->setProperty("action","probe");return juce::JSON::toString(juce::var(o));
 }
 void caption(juce::Label& label,const juce::String& text,juce::Component& control){label.setText(text,juce::dontSendNotification);label.setFont(studio::font(12.0f,juce::Font::bold));label.setBorderSize({0,0,0,0});label.setColour(juce::Label::textColourId,studio::muted);control.setTitle(text);addAndMakeVisible(label);}
 void setup(juce::ComboBox& box,std::initializer_list<const char*> values){int i=1;for(auto v:values)box.addItem(v,i++);box.setSelectedId(1);addAndMakeVisible(box);}
 void showDragStatus(juce::String message){dragStatus=std::move(message);dragStatusUntil=juce::Time::getMillisecondCounter()+5000;status.setText(dragStatus,juce::dontSendNotification);}
 juce::String activeTrack(){auto index=layer.getSelectedItemIndex();return index>=0&&index<trackKeys.size()?trackKeys[index]:layer.getText();}
 void refreshPresentation(){
  if(auto state=processor.presentation(viewRevision)){
   auto snapshot=processor.snapshot();
   if (state->revision != snapshot.revision) return;
   viewRevision=state->revision;
   if (state->composer != showComposer) {
    showComposer = state->composer;
    showDetails = showMood = showIdeaSettings = false;
    showEdit = showComposer && processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone;
   }
   saveProject.setEnabled(state->hasState);
   regionBars=0;
   if (state->composer) {
    auto project=composer::decodeProjectJson(state->json);
    if(project.project&&project.project->composition) {
     overview.setDocument(*project.project->composition,project.project->hostStartQuarterBeat);
     const auto& document = *project.project->composition;
     const auto starts=composerView::barStarts(document);
     signal.setComposition(document,starts,bool(snapshot.idea["metadata"]["editable"]));
     if(document.source=="native-motif-rules-v5"&&document.meters.size()==1&&!document.chords.empty()&&!starts.empty())
      regionBars=int(starts.size())-1;
    }
   } else {
    signal.setIdea(snapshot.idea);
   }
   if(state->composer&&!bool(snapshot.idea["metadata"]["editable"]))showEdit=false;auto current=activeTrack();juce::StringArray keys,names;if(state->composer){auto tracks=snapshot.idea["tracks"];for(int i=0;i<tracks.size();++i){keys.add(tracks[i]["key"].toString());names.add(tracks[i]["name"].toString());}}else{keys={"bass","chords","melody"};names=keys;}
   const bool replaced=roll.applySnapshot(std::move(snapshot));
   bool same=keys==trackKeys&&names.size()==layer.getNumItems();for(int i=0;same&&i<names.size();++i)same=names[i]==layer.getItemText(i);
   if(!same){trackKeys=keys;layer.clear(juce::dontSendNotification);for(int i=0;i<names.size();++i)layer.addItem(names[i],i+1);auto index=keys.indexOf(current);layer.setSelectedItemIndex(index<0?0:index,juce::dontSendNotification);roll.selectLayer(activeTrack(),false);}layer.setTitle(state->composer?"Edit track":"Edit layer");
   layoutWorkspace(replaced);
  }
 }
 void setNoteView(bool edit){
  showEdit=edit;showMood=false;layoutWorkspace();timerCallback();
 }
 void layoutWorkspace(bool recenter=false){
  auto* target=showComposer&&!showEdit?static_cast<juce::Component*>(&overview):static_cast<juce::Component*>(&roll);
  const bool changed=viewport.getViewedComponent()!=target;
  if(changed)viewport.setViewedComponent(target,false);
  resized();
  repaint();
  if(changed||(recenter&&target==&roll))viewport.setViewPosition(0,target==&roll?roll.preferredScrollY():0);
 }
 void chooseRegion(){
  juce::PopupMenu menu;
  for(int first=1;first<=regionBars;++first){
   juce::PopupMenu endings;
   for(int last=first;last<=regionBars;++last)
    endings.addItem(first*17+last, last==first?"This bar":"Through bar "+juce::String(last));
   menu.addSubMenu("From bar "+juce::String(first),endings);
  }
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&regenerate),
   [safe=juce::Component::SafePointer<Editor>(this),revision=viewRevision,track=activeTrack()](int chosen){
    if(safe&&chosen!=0&&!safe->processor.requestRegion(revision,track,chosen/17,chosen%17))
     safe->showDragStatus("Region unavailable. Select bars again.");
   });
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
   else{auto file=projectFiles::read(fc.getResult());if(file.error.isNotEmpty()){safe->showDragStatus(file.error);return;}const bool restored=safe->processor.restoreStateInformation(file.text.data(),int(file.text.size()));if(restored)safe->processor.updateHostDisplay(juce::AudioProcessorListener::ChangeDetails{}.withNonParameterStateChanged(true));safe->showDragStatus(restored?"Project opened":safe->processor.getStatus());safe->timerCallback();}
  });
  }catch(const std::bad_alloc&){fileChoosing=false;showDragStatus("Insufficient memory to choose a project file");}
 }
 void timerCallback() override {
  if(juce::Time::getMillisecondCounter()-modelCheckedAt>=2000)refreshModelChoice();
  refreshComposerDraft();
  pollComposerMidi();
  refreshPresentation();

  const bool legacy = !showComposer;
  const bool ideaControls = legacy && showIdeaSettings;
  const bool busy = processor.busy;
  const bool canSwitch = processor.canSwitchMode();
  const bool resetView = legacy && (showMood || showEdit);
  if (resetView) { showMood = false; showEdit = false; }
  const bool editable = legacy || processor.canEditComposer();
  const bool editing = legacy || (!showMood && showEdit && editable);

  const bool showActionStatus = dragStatus.isNotEmpty() && int(juce::Time::getMillisecondCounter()-dragStatusUntil)<0;
  const auto message = showActionStatus ? dragStatus : processor.getStatus();
  status.setText(message, juce::dontSendNotification);
  status.setTooltip(message);

  for (auto* control : std::initializer_list<juce::Component*>{&provider,&providerLabel,&key,&keyLabel,
       &scale,&scaleLabel,&style,&styleLabel,&bars,&barsLabel,&bpm,&tempoLabel,&model,&modelLabel,&testConnection})
   control->setVisible(ideaControls);
  for (auto* control : std::initializer_list<juce::Component*>{&generate,&variation,&dragMidi,&ideaSettings,&providerStatus})
   control->setVisible(legacy);
  ideaSettings.setButtonText(ideaControls ? "CLOSE SETTINGS" : "IDEA SETTINGS");
  providerStatus.setText("VIA " + provider.getText().toUpperCase(),juce::dontSendNotification);
  signal.setVisible(!ideaControls && !showDetails);
  for (auto* control : std::initializer_list<juce::Component*>{&layer,&quantize,&undo,&redo,&addNote,&deleteNote})
   control->setVisible(editing);
  regenerate.setVisible(!legacy&&editing&&regionBars>0);
  regenerate.setEnabled(!busy);

  mode.setButtonText(legacy ? (canSwitch ? "VIEW LOOP" : "COMPOSE LOOP") : "IDEA BANK");
  mode.setEnabled(canSwitch || (legacy && !busy));
  compose.setVisible(!legacy);
  compose.setEnabled(!busy);
  studio::primary(mode, legacy && !canSwitch && !busy);
  compose.setButtonText(engine.getSelectedId()==2?"COMPOSE WITH MIDI-LLM":"COMPOSE OFFLINE LOOP");
  studio::primary(compose, !busy);
  cancel.setVisible(busy);
  cancel.setEnabled(busy);
  generate.setEnabled(legacy && !busy);
  testConnection.setEnabled(legacy && !busy);
  model.setEnabled(legacy && provider.getText() != "offline");

  saveProject.setEnabled(processor.hasSavableState());
  sound.setToggleState(processor.audition.load(), juce::dontSendNotification);


  noteView.setVisible(!legacy && !showMood);
  noteView.setEnabled(editable);
  noteView.setButtonText(showEdit ? "TRACK OVERVIEW" : "PIANO ROLL");
  moodSettings.setVisible(!legacy);
  moodSettings.setButtonText(showMood ? "BACK TO LOOP" : "MOOD SETTINGS");
  moodControls.setVisible(!legacy && showMood);
  openRecipe.setVisible(!legacy && showMood);
  saveRecipe.setVisible(!legacy && showMood);
  viewport.setVisible(!showMood);
  help.setVisible(!showMood);
  help.setText(editing ? "Brackets select notes | Arrows move | Shift + Left/Right resize | Delete removes"
                      : "Home / End and Page Up / Down scroll through the overview",
               juce::dontSendNotification);

  loopDetails.setVisible(!legacy);
  loopDetails.setButtonText(showDetails ? "CLOSE DETAILS" : "LOOP DETAILS");
  composerNotice.setVisible(!legacy && showDetails);
  if (!legacy && showDetails) {
   const auto summary = processor.composerSummary();
   const auto settings = summary.isEmpty() ? juce::String("Direction and seed apply to the next loop.") : summary;
   if (composerNotice.getText() != settings) {
    composerNotice.setText(settings, juce::dontSendNotification);
    composerNotice.setCaretPosition(0);
   }
   composerNotice.setTooltip(settings);
  }
  signal.setActiveTrack(editing ? layer.getSelectedItemIndex() : -1);
  layer.setEnabled(editing);
  roll.setEnabled(editable);
  const int count = processor.ideaCount();
  dragMidi.setEnabled(count > 0);
  variation.setEnabled(count > 0);
  quantize.setEnabled(editing && (!legacy || count > 0));
  exportButton.setEnabled((!legacy || count > 0) && !composerMidiExport.busy() && !fileChoosing);
  addNote.setEnabled(editing && (!legacy || count > 0));
  if (count != variation.getNumItems()) {
   variation.clear();
   for (int i = 0; i < count; ++i) variation.addItem("IDEA " + juce::String(i+1), i+1);
  }
  if (count > 0) variation.setSelectedItemIndex(processor.selectedIdeaIndex(), juce::dontSendNotification);
  deleteNote.setEnabled(roll.hasSelection());
  undo.setEnabled(processor.canUndo());
  redo.setEnabled(processor.canRedo());

  if (resetView) resized();
  if (busy != busyLayout) {
   busyLayout = busy;
   repaint(22, getHeight()-19, 8, 8);
  }
 }
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
