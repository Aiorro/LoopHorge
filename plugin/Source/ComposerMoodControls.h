#pragma once
#include "engine/Direction.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Editor-local next-request settings, captured by value when Compose is pressed.
class ComposerMoodControls final:public juce::Component {
public:
 ComposerMoodControls(){
  setName("Composer mood controls");
  for(size_t i=0;i<7;++i){
   labels[i].setText(names[i],juce::dontSendNotification);addAndMakeVisible(labels[i]);
   automatic[i].setButtonText("Auto");automatic[i].setToggleState(true,juce::dontSendNotification);automatic[i].setTitle(juce::String(names[i])+" Auto");addAndMakeVisible(automatic[i]);
   values[i].setRange(0,100,1);values[i].setValue(50);values[i].setSliderStyle(juce::Slider::LinearHorizontal);values[i].setTextBoxStyle(juce::Slider::TextBoxRight,false,56,24);values[i].setTitle(names[i]);values[i].setEnabled(false);values[i].setTooltip("Next request only. Disable Auto to override direction text; use SAVE RECIPE to keep settings.");addAndMakeVisible(values[i]);
   automatic[i].onClick=[this,i]{values[i].setEnabled(!automatic[i].getToggleState());};
   automatic[i].onStateChange=[this,i]{values[i].setEnabled(!automatic[i].getToggleState());};
  }
 }
 composer::ManualOverrides overrides()const{
  composer::ManualOverrides result;auto& m=result.values.mood;
  int* fields[]{&m.darkness,&m.energy,&m.melancholy,&m.tension,&m.aggression,&m.atmosphere,&m.complexity};
  for(size_t i=0;i<7;++i){*fields[i]=int(values[i].getValue());result.pinned.set(size_t(keys[i]),!automatic[i].getToggleState());}return result;
 }
 void apply(const composer::ManualOverrides& manual){
  const auto& m=manual.values.mood;const int* fields[]{&m.darkness,&m.energy,&m.melancholy,&m.tension,&m.aggression,&m.atmosphere,&m.complexity};
  for(size_t i=0;i<7;++i){values[i].setValue(*fields[i],juce::dontSendNotification);automatic[i].setToggleState(!manual.pinned[size_t(keys[i])],juce::dontSendNotification);values[i].setEnabled(!automatic[i].getToggleState());}
 }
 void resized()override{for(int i=0;i<7;++i){int y=i*44;labels[size_t(i)].setBounds(8,y,120,36);automatic[size_t(i)].setBounds(136,y,76,36);values[size_t(i)].setBounds(220,y,std::max(0,getWidth()-228),36);}}
private:
 static constexpr const char* names[]{"Darkness","Energy","Melancholy","Tension","Aggression","Atmosphere","Complexity"};
 static constexpr composer::Field keys[]{composer::Field::darkness,composer::Field::energy,composer::Field::melancholy,composer::Field::tension,composer::Field::aggression,composer::Field::atmosphere,composer::Field::complexity};
 std::array<juce::Label,7> labels;std::array<juce::ToggleButton,7> automatic;std::array<juce::Slider,7> values;
};
