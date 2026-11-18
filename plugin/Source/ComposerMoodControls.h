#pragma once
#include "engine/Direction.h"
#include "StudioLook.h"

// Editor-local next-request settings, captured by value when Compose is pressed.
class ComposerMoodControls final:public juce::Component {
public:
 ComposerMoodControls(){
  setName("Composer mood controls");
  for(size_t i=0;i<7;++i){
   labels[i].setText(names[i],juce::dontSendNotification);labels[i].setJustificationType(juce::Justification::centred);labels[i].setFont(studio::font(14,juce::Font::bold));addAndMakeVisible(labels[i]);
   automatic[i].setButtonText("Auto");automatic[i].setToggleState(true,juce::dontSendNotification);automatic[i].setTitle(juce::String(names[i])+" Auto");addAndMakeVisible(automatic[i]);
   values[i].setRange(0,100,1);values[i].setValue(50);values[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);values[i].setTextBoxStyle(juce::Slider::TextBoxBelow,false,50,22);values[i].setTitle(names[i]);values[i].setEnabled(false);values[i].setTooltip("Next request only. Disable Auto to override direction text; use SAVE RECIPE to keep settings.");addAndMakeVisible(values[i]);
   automatic[i].onStateChange=[this,i]{values[i].setEnabled(!automatic[i].getToggleState());if(onChange&&!applying)onChange();};
   values[i].onValueChange=[this]{if(onChange&&!applying)onChange();};
  }
 }
 std::function<void()> onChange;
 composer::ManualOverrides overrides()const{
  composer::ManualOverrides result;auto& m=result.values.mood;
  int* fields[]{&m.darkness,&m.energy,&m.melancholy,&m.tension,&m.aggression,&m.atmosphere,&m.complexity};
  for(size_t i=0;i<7;++i){*fields[i]=int(values[i].getValue());result.pinned.set(size_t(keys[i]),!automatic[i].getToggleState());}return result;
 }
 void apply(const composer::ManualOverrides& manual){
  juce::ScopedValueSetter<bool> guard(applying,true);
  const auto& m=manual.values.mood;const int* fields[]{&m.darkness,&m.energy,&m.melancholy,&m.tension,&m.aggression,&m.atmosphere,&m.complexity};
  for(size_t i=0;i<7;++i){values[i].setValue(*fields[i],juce::dontSendNotification);automatic[i].setToggleState(!manual.pinned[size_t(keys[i])],juce::dontSendNotification);values[i].setEnabled(!automatic[i].getToggleState());}
 }
 void resized() override {
  for (size_t i=0; i<labels.size(); ++i) {
   auto tile = controlTile(int(i)).reduced(8, 8);
   labels[i].setBounds(tile.removeFromTop(24));
   automatic[i].setBounds(tile.removeFromBottom(24).withSizeKeepingCentre(62,24));
   values[i].setBounds(tile.reduced(1, 5));
  }
 }
 void paint(juce::Graphics& g) override {
  g.setFont(studio::font(18,juce::Font::bold));
  g.setColour(studio::text);
  g.drawText("Mood", 0, 5, 150, 28, juce::Justification::centredLeft);
  g.setFont(studio::font(13));
  g.setColour(studio::muted);
  g.drawText("Turn off Auto to set a value for the next loop.",160,5,getWidth()-160,28,juce::Justification::centredRight);
  for (int i=0; i<int(labels.size()); ++i) {
   auto frame = controlTile(i).toFloat().reduced(0.5f);
   g.setGradientFill(juce::ColourGradient(studio::panel, frame.getTopLeft(),
                                         studio::background, frame.getBottomLeft(), false));
   g.fillRoundedRectangle(frame, 8.0f);
   g.setColour(studio::border);
   g.drawRoundedRectangle(frame, 8.0f, 1.0f);
  }
 }
private:
 juce::Rectangle<int> controlTile(int index) const {
  const int columns = int(labels.size());
  const int left = getWidth()*index/columns;
  const int next = getWidth()*(index+1)/columns;
  return {left+3, 43, std::max(0,next-left-7), std::max(0,getHeight()-50)};
 }
 bool applying=false;
 static constexpr const char* names[]{"Darkness","Energy","Melancholy","Tension","Aggression","Atmosphere","Complexity"};
 static constexpr composer::Field keys[]{composer::Field::darkness,composer::Field::energy,composer::Field::melancholy,composer::Field::tension,composer::Field::aggression,composer::Field::atmosphere,composer::Field::complexity};
 std::array<juce::Label,7> labels;std::array<juce::ToggleButton,7> automatic;std::array<juce::Slider,7> values;
};
