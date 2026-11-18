#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace studio {
inline const juce::Colour background{0xff0c1013}, panel{0xff141a1d}, field{0xff1f272b};
inline const juce::Colour border{0xff343f44}, text{0xffeff3ee}, muted{0xffa5b2ad};
inline const juce::Colour accent{0xffd7fb66}, accentInk{0xff1a2510}, active{0xff283723};

inline juce::Colour trackColour(int track) {
 constexpr juce::uint32 colours[]{0xffd7fb66, 0xff66e5c0, 0xffc6a9ff, 0xffffc37f};
 return juce::Colour(colours[track%4]);
}

inline juce::Font font(float height, int style = juce::Font::plain) {
 return juce::FontOptions(height, style);
}

class Look final : public juce::LookAndFeel_V4 {
public:
 Look() {
  setColourScheme(ColourScheme{background, field, panel, border, text, accent, accentInk, accent, text});
  setColour(juce::ResizableWindow::backgroundColourId, background);
  setColour(juce::Label::textColourId, text);
  setColour(juce::TextButton::buttonColourId, field);
  setColour(juce::TextButton::buttonOnColourId, active);
  setColour(juce::TextButton::textColourOffId, text);
  setColour(juce::TextButton::textColourOnId, accent);
  setColour(juce::TextEditor::backgroundColourId, field);
  setColour(juce::TextEditor::textColourId, text);
  setColour(juce::TextEditor::outlineColourId, border);
  setColour(juce::TextEditor::focusedOutlineColourId, accent);
  setColour(juce::TextEditor::highlightColourId, accent.withAlpha(0.35f));
  setColour(juce::TextEditor::highlightedTextColourId, text);
  setColour(juce::CaretComponent::caretColourId, accent);
  setColour(juce::ComboBox::backgroundColourId, field);
  setColour(juce::ComboBox::outlineColourId, border);
  setColour(juce::ComboBox::arrowColourId, muted);
  setColour(juce::ComboBox::textColourId, text);
  setColour(juce::ComboBox::focusedOutlineColourId, accent);
  setColour(juce::PopupMenu::backgroundColourId, panel);
  setColour(juce::PopupMenu::textColourId, text);
  setColour(juce::PopupMenu::highlightedBackgroundColourId, accent);
  setColour(juce::PopupMenu::highlightedTextColourId, accentInk);
  setColour(juce::ToggleButton::textColourId, text);
  setColour(juce::ToggleButton::tickColourId, accent);
  setColour(juce::ToggleButton::tickDisabledColourId, muted);
  setColour(juce::Slider::trackColourId, accent);
  setColour(juce::Slider::thumbColourId, accent);
  setColour(juce::Slider::textBoxBackgroundColourId, field);
  setColour(juce::Slider::textBoxOutlineColourId, border);
  setColour(juce::Slider::textBoxTextColourId, text);
  setColour(juce::ScrollBar::thumbColourId, muted.withAlpha(0.6f));
 }

 juce::Font getTextButtonFont(juce::TextButton&, int height) override { return font(height<24 ? 11.0f : 13.0f, juce::Font::bold); }
 juce::Font getComboBoxFont(juce::ComboBox&) override { return font(14.0f); }

 void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& colour,
                           bool highlighted, bool pressed) override {
  const auto area = button.getLocalBounds().toFloat().reduced(0.5f);
  const bool enabled = button.isEnabled();
  auto fill = enabled ? colour : field;
  if (enabled && (highlighted || pressed))
   fill = fill.interpolatedWith(colour == accent ? accentInk : text, pressed ? 0.16f : 0.08f);
  const bool primary = enabled && colour == accent;
  const auto top = fill.interpolatedWith(juce::Colours::white, primary ? 0.13f : 0.08f);
  const auto bottom = fill.darker(primary ? 0.17f : 0.25f);
  g.setGradientFill(juce::ColourGradient(top, area.getTopLeft(), bottom, area.getBottomLeft(), false));
  g.fillRoundedRectangle(area, 6.0f);
  g.setColour(button.hasKeyboardFocus(true) ? (colour == accent ? text : accent) :
              enabled && colour == accent ? fill : border.withAlpha(enabled ? 1.0f : 0.55f));
  g.drawRoundedRectangle(area, 6.0f, 1.0f);
 }

 void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                       float position, float start, float end, juce::Slider& slider) override {
  const auto centre = juce::Rectangle<float>(float(x),float(y),float(width),float(height)).getCentre();
  const float radius = std::min(43.0f, float(std::min(width,height))*0.45f);
  if (radius < 12.0f) return;
  const auto disk = juce::Rectangle<float>(radius*2, radius*2).withCentre(centre);
  const auto tint = slider.isEnabled() ? accent : border;
  const float angle = start+position*(end-start);
  juce::Path track;
  track.addCentredArc(centre.x,centre.y,radius,radius,0.0f,start,end,true);
  const juce::PathStrokeType stroke(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
  g.setColour(border);
  g.strokePath(track, stroke);
  if (slider.isEnabled() && position > 0.0f) {
   juce::Path value;
   value.addCentredArc(centre.x,centre.y,radius,radius,0.0f,start,angle,true);
   g.setColour(tint);
   g.strokePath(value, stroke);
  }
  const auto face = disk.reduced(7.0f);
  g.setGradientFill(juce::ColourGradient(field.interpolatedWith(text,0.08f), face.getTopLeft(),
                                        background, face.getBottomLeft(), false));
  g.fillEllipse(face);
  g.setColour(slider.hasKeyboardFocus(true) ? accent : border);
  g.drawEllipse(face, 1.0f);
  const auto direction = juce::Point<float>(std::sin(angle), -std::cos(angle));
  g.setColour(tint);
  g.drawLine({centre+direction*(radius-22.0f),centre+direction*(radius-12.0f)}, 2.0f);
 }

 void drawComboBox(juce::Graphics& g, int width, int height, bool pressed, int, int, int, int,
                   juce::ComboBox& box) override {
  const auto area = juce::Rectangle<float>(float(width), float(height)).reduced(0.5f);
  g.setColour(pressed ? active : field);
  g.fillRoundedRectangle(area, 5.0f);
  g.setColour(box.hasKeyboardFocus(true) ? accent : border);
  g.drawRoundedRectangle(area, 6.0f, 1.0f);
  const float x = float(width) - 16.0f, y = float(height) / 2.0f;
  juce::Path arrow;
  arrow.startNewSubPath(x - 4.0f, y - 2.0f);
  arrow.lineTo(x, y + 2.0f);
  arrow.lineTo(x + 4.0f, y - 2.0f);
  g.setColour(box.isEnabled() ? muted : border);
  g.strokePath(arrow, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
 }
};

inline void primary(juce::TextButton& button, bool enabled = true) {
 button.setColour(juce::TextButton::buttonColourId, enabled ? accent : field);
 button.setColour(juce::TextButton::textColourOffId, enabled && button.isEnabled() ? accentInk : text);
}
}
