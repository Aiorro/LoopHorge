#pragma once

#include "StudioLook.h"
#include "engine/Composition.h"
#include <array>
#include <cmath>

// Fixed-resolution view of committed MIDI. It is built on the message thread
// when the project revision changes, never from audio samples or during paint.
class LoopSignal final : public juce::Component {
public:
 LoopSignal() {
  setComponentID("loop-signal");
  setTitle("MIDI signal map");
  setInterceptsMouseClicks(false, true);
  for (int i=0; i<lanes; ++i) {
   auto& button = trackButtons[size_t(i)];
   const auto colour = studio::trackColour(i);
   button.setComponentID("signal-track-"+juce::String(i));
   button.setTooltip("Open this track in the piano roll");
   button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff19221d));
   button.setColour(juce::TextButton::buttonOnColourId, colour.interpolatedWith(studio::background, 0.78f));
   button.setColour(juce::TextButton::textColourOnId, colour);
   button.onClick = [this, i] { if (onTrackSelected) onTrackSelected(i); };
   addChildComponent(button);
  }
  publish();
 }

 std::function<void(int)> onTrackSelected;

 void setActiveTrack(int index) {
  for (int i=0; i<lanes; ++i)
   trackButtons[size_t(i)].setToggleState(i==index, juce::dontSendNotification);
 }

 void resized() override {
  const auto area = displayArea().withTrimmedTop(24);
  const int rows = std::max(1, std::min(tracks, lanes));
  const int height = area.getHeight()/rows;
  for (int i=0; i<lanes; ++i)
   trackButtons[size_t(i)].setBounds(area.getX()+12, area.getY()+i*height+1, 88, std::max(0,height-2));
 }

 void setComposition(const composer::Composition& song, const std::vector<int64_t>& barStarts, bool canEdit) {
  clear();
  bars = barStarts.empty() ? 0 : int(barStarts.size()-1);
  editable = canEdit;
  tracks = int(song.tracks.size());
  if (!song.tempos.empty()) {
   tempo = 60000000.0 / song.tempos.front().microsPerQuarter;
   changingTempo = song.tempos.size() > 1;
  }
  const double end = double(song.endTick.value);
  if (end > 0) for (int i=0; i<=bars && i<int(barStarts.size()); ++i) marks[size_t(i)] = double(barStarts[size_t(i)])/end;
  for (int i = 0; i < tracks; ++i) {
   const auto& track = song.tracks[size_t(i)];
   if (i < lanes) names[size_t(i)] = juce::String::fromUTF8(track.name.c_str());
   for (const auto& note : track.notes)
    add(i, double(note.start.value), double(note.duration.value), end, note.pitch, note.velocity);
  }
  publish();
 }

 void setIdea(const juce::var& idea) {
  clear();
  bars = int(idea["metadata"]["bars"]);
  tempo = double(idea["metadata"]["bpm"]);
  if (bars > 0) {
   tracks = 3;
   editable = true;
   for (int i=0; i<=bars && i<int(marks.size()); ++i) marks[size_t(i)] = double(i)/bars;
   names = {"Bass", "Chords", "Melody", {}};
   constexpr const char* keys[] = {"bass", "chords", "melody"};
   for (int i = 0; i < tracks; ++i) {
    const auto notes = idea["layers"][keys[i]];
    for (int j = 0; j < notes.size(); ++j) {
     const auto note = notes[j];
     add(i, double(note["start"]), double(note["duration"]), bars*4.0, int(note["pitch"]), int(note["velocity"]));
    }
   }
  }
  publish();
 }

 std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
  return std::make_unique<juce::AccessibilityHandler>(*this, juce::AccessibilityRole::group);
 }

 void paint(juce::Graphics& g) override {
  const auto frame = getLocalBounds().toFloat().reduced(0.5f);
  juce::Path mask;
  mask.addRoundedRectangle(frame, 9.0f);
  {
   const juce::Graphics::ScopedSaveState clipped(g);
   g.reduceClipRegion(mask);
   g.setGradientFill(juce::ColourGradient(juce::Colour(0xff202c25), frame.getTopLeft(),
                                         juce::Colour(0xff10171c), frame.getBottomRight(), false));
   g.fillRect(frame);
   g.setGradientFill(juce::ColourGradient(studio::accent.withAlpha(0.12f), {86.0f, 66.0f},
                                         studio::accent.withAlpha(0.0f), {252.0f, 76.0f}, true));
   g.fillRect(frame);
   drawDial(g, {78.0f, frame.getCentreY()});
   drawTracks(g, displayArea());
  }
  g.setColour(studio::border.brighter(0.17f));
  g.strokePath(mask, juce::PathStrokeType(1.0f));
 }

private:
 static constexpr int bins = 128, lanes = 4;
 struct Bin { uint8_t peak = 0; uint8_t end = 0; };
 std::array<std::array<Bin, bins>, lanes> activity{};
 std::array<juce::TextButton, lanes> trackButtons{};
 std::array<int, 12> pitches{};
 std::array<double, 17> marks{};
 std::array<juce::String, lanes> names{};
 int bars = 0, tracks = 0, notes = 0;
 double tempo = 0;
 bool changingTempo = false, editable = false;

 juce::Rectangle<int> displayArea() const {
  return getLocalBounds().withTrimmedLeft(151).withTrimmedRight(18).reduced(0,11);
 }

 void clear() {
  activity = {};
  pitches = {};
  marks = {};
  names = {};
  bars = tracks = notes = 0;
  tempo = 0;
  changingTempo = editable = false;
 }

 void add(int track, double start, double duration, double end, int pitch, int velocity) {
  if (end <= 0 || !std::isfinite(start) || start < 0 || start >= end || pitch < 0 || pitch > 127) return;
  const int bin = juce::jlimit(0, bins-1, int(start/end*bins));
  auto& event = activity[size_t(std::min(track, lanes-1))][size_t(bin)];
  event.peak = std::max(event.peak, uint8_t(juce::jlimit(0, 127, velocity)));
  auto endBin = std::ceil((start+duration)/end*bins);
  if (std::isfinite(endBin))
   event.end = std::max(event.end, uint8_t(juce::jlimit(double(bin+1), double(bins), endBin)));
  ++pitches[size_t(pitch%12)];
  ++notes;
 }

 void publish() {
  if (tracks > lanes) names.back() = juce::String(tracks-lanes+1)+" more";
  const int buttons = tracks > lanes ? lanes-1 : tracks;
  for (int i=0; i<lanes; ++i) {
   auto& button = trackButtons[size_t(i)];
   button.setButtonText(names[size_t(i)]);
   button.setTitle(names[size_t(i)]+" - open piano roll");
   button.setVisible(editable && i<buttons);
  }
  resized();
  juce::StringArray facts;
  if (bars > 0) facts.add(juce::String(bars) + (bars == 1 ? " bar" : " bars"));
  if (tracks > 0) {
   facts.add(juce::String(tracks) + (tracks == 1 ? " track" : " tracks"));
   facts.add(juce::String(notes) + (notes == 1 ? " note" : " notes"));
  }
  summary = facts.joinIntoString("   /   ");
  const auto tempoInfo = tempo > 0 ? " " + juce::String(tempo, 1) + (changingTempo ? " starting BPM." : " BPM.") : juce::String{};
  setDescription(tracks > 0 ? summary + "." + tempoInfo + " Each lane shows MIDI note timing and velocity."
                            : "Compose a loop to see note activity.");
  repaint();
 }

 void drawDial(juce::Graphics& g, juce::Point<float> centre) const {
  const auto circle = [centre](float radius) { return juce::Rectangle<float>(radius*2, radius*2).withCentre(centre); };
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff334238), centre.translated(0,-46),
                                        juce::Colour(0xff101813), centre.translated(0,46), false));
  g.fillEllipse(circle(46));
  g.setColour(juce::Colour(0xff526353));
  g.drawEllipse(circle(46), 1.0f);
  g.setColour(juce::Colour(0xff111a14));
  g.fillEllipse(circle(34));
  g.setColour(juce::Colour(0xff3b4a3e));
  g.drawEllipse(circle(34), 1.0f);
  const int highest = std::max(1, *std::max_element(pitches.begin(), pitches.end()));
  for (int i = 0; i < 48; ++i) {
   const float angle = juce::MathConstants<float>::twoPi*float(i)/48.0f;
   const auto dir = juce::Point<float>(std::sin(angle), -std::cos(angle));
   const float strength = float(pitches[size_t(i/4)])/float(highest);
   const float length = 4.0f + strength*8.0f + (i%4==0 ? 2.0f : 0.0f);
   g.setColour(studio::accent.interpolatedWith(studio::border, 1.0f-strength*0.9f));
   g.drawLine({centre+dir*49.0f, centre+dir*(49.0f+length)}, i%4==0 ? 1.5f : 1.0f);
  }
  g.setFont(studio::font(24, juce::Font::bold));
  g.setColour(tempo > 0 ? studio::accent : studio::muted);
  const auto box = circle(32).toNearestInt();
  g.drawText(tempo > 0 ? juce::String(tempo, std::abs(tempo-std::round(tempo))<0.05?0:1) : juce::String::fromUTF8("\xe2\x80\x94"),
             box.withY(box.getY()+8).withHeight(29), juce::Justification::centred);
  g.setFont(studio::font(9, juce::Font::bold));
  g.setColour(studio::muted);
  g.drawText(changingTempo ? "START BPM" : "BPM", box.withY(box.getY()+37).withHeight(13), juce::Justification::centred);
 }

 void drawTracks(juce::Graphics& g, juce::Rectangle<int> area) const {
  auto heading = area.removeFromTop(19);
  g.setFont(studio::font(11, juce::Font::bold));
  g.setColour(studio::accent);
  g.drawText("MIDI SIGNAL", heading.removeFromLeft(130), juce::Justification::centredLeft);
  g.setColour(studio::muted);
  g.drawText(summary, heading, juce::Justification::centredRight);
  area.removeFromTop(5);
  auto chart = area.withTrimmedLeft(110);
  g.setColour(juce::Colour(0xff0f1618).withAlpha(0.83f));
  g.fillRoundedRectangle(chart.toFloat(), 4.0f);
  const bool haveBars = bars > 0 && bars < int(marks.size());
  const int divisions = haveBars ? bars : tracks==0 ? 8 : 1;
  for (int i = 0; i <= divisions; ++i) {
   g.setColour(studio::border.withAlpha(i%4==0 ? 0.8f : 0.41f));
   const double position = haveBars ? marks[size_t(i)] : double(i)/divisions;
   g.drawVerticalLine(chart.getX()+int(chart.getWidth()*position), float(chart.getY()), float(chart.getBottom()));
  }
  if (tracks == 0) {
   g.setColour(studio::muted);
   g.setFont(studio::font(13));
   g.drawText("Compose a loop to see note activity", chart, juce::Justification::centred);
   return;
  }
  const int rows = std::min(tracks, lanes);
  const int height = chart.getHeight()/rows;
  const float step = float(chart.getWidth()-2)/float(bins);
  for (int i=0; i<rows; ++i) {
   const auto tint = studio::trackColour(i);
   const int y = chart.getY()+i*height;
   g.setColour(tint);
   g.fillRoundedRectangle(float(area.getX()+1), float(y+height/2-3), 6.0f, 6.0f, 2.0f);
   g.setFont(studio::font(12, juce::Font::bold));
   g.setColour(studio::text);
   if (!trackButtons[size_t(i)].isVisible())
    g.drawText(names[size_t(i)], area.getX()+13, y, 83, height, juce::Justification::centredLeft, true);
   g.setColour(studio::border.withAlpha(0.48f));
   g.drawHorizontalLine(y+height-1, float(chart.getX()), float(chart.getRight()));
   const auto& row = activity[size_t(i)];
   const float base = float(y+height-2);
   for (int j=0; j<bins; ++j) {
    const auto event = row[size_t(j)];
    if (event.peak==0) continue;
    const float x = float(chart.getX()+1) + float(j)*step;
    const float length = float(std::max(1,int(event.end)-j))*step;
    const float trailHeight = float(event.peak)/127.0f * float(height-6);
    g.setColour(tint.withAlpha(0.12f));
    g.fillRect(x, base-trailHeight, length, trailHeight);
    g.setColour(tint.withAlpha(0.27f));
    g.fillRect(x, base-trailHeight, length, 1.0f);
   }
   for (int j=0; j<bins; ++j) {
    if (row[size_t(j)].peak==0) continue;
    const float intensity = float(row[size_t(j)].peak)/127.0f;
    const float x = float(chart.getX()+1) + float(j)*step;
    const float size = 3.0f + float(height-6)*intensity;
    g.setColour(tint.withAlpha(0.13f));
    g.fillRect(x, base-size, std::max(2.0f, step*1.2f), size);
    g.setColour(tint.withAlpha(0.65f+0.35f*intensity));
    g.fillRoundedRectangle(x, base-size, std::max(1.4f, step*0.60f), size, 1.0f);
   }
  }
 }
 juce::String summary;
};
