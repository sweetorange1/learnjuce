namespace tremolo {
namespace {
constexpr auto buttonInsetWidth = 2.f;

// 统一灰阶主题色（避免出现蓝/黄等点缀色）
const juce::Colour kBgDark{0xFF121212};
const juce::Colour kBgMid{0xFF1E1E1E};
const juce::Colour kInsetTop{0xFF262626};
const juce::Colour kInsetBottom{0xFF1A1A1A};
const juce::Colour kTextPrimary{0xFFE6E6E6};
const juce::Colour kTextSecondary{0xFFBDBDBD};
const juce::Colour kBorder{0xFF3A3A3A};
const juce::Colour kHighlightBg{0xFF303030};
const juce::Colour kControlTop{0xFF3A3A3A};
const juce::Colour kControlBottom{0xFF222222};
const juce::Colour kControlTopEdge{0xFFB0B0B0};

void drawGradientButton(juce::Graphics& g,
                        const juce::Rectangle<float>& bounds,
                        const juce::ColourGradient& gradient) {
  g.setGradientFill(gradient);
  g.fillRoundedRectangle(bounds, 4.f);
}

void drawNeutralGradientButton(juce::Graphics& g,
                               const juce::Rectangle<float>& bounds,
                               bool isOn) {
  // 关闭：更暗；开启：稍亮（仍为灰阶）
  const auto top = isOn ? juce::Colour{0xFF4A4A4A} : kControlTop;
  const auto bottom = isOn ? juce::Colour{0xFF2A2A2A} : kControlBottom;
  auto buttonGradient = juce::ColourGradient::vertical(top, bottom, bounds);
  buttonGradient.addColour(0.73, isOn ? juce::Colour{0xFF3A3A3A}
                                      : juce::Colour{0xFF2A2A2A});
  drawGradientButton(g, bounds, buttonGradient);
}

void drawButtonInset(juce::Graphics& g, const juce::Rectangle<float>& bounds) {
  auto insetGradient = juce::ColourGradient::vertical(
      kInsetTop, kInsetBottom, bounds);
  insetGradient.addColour(0.35, juce::Colour{0xFF2D2D2D});
  g.setGradientFill(insetGradient);
  g.fillRoundedRectangle(bounds, 6.f);
}
}  // namespace

juce::Colour CustomLookAndFeel::getColor(Colors colorName) {
  static const std::array colors{
      kTextSecondary,
      kTextPrimary,
  };
  return colors.at(juce::toUnderlyingType(colorName));
}

CustomLookAndFeel::CustomLookAndFeel() {
  setColour(juce::ComboBox::textColourId, getColor(Colors::textPrimary));
  setColour(juce::Label::textColourId, getColor(Colors::textPrimary));
  setColour(juce::PopupMenu::backgroundColourId, kBgDark);
  setColour(juce::PopupMenu::textColourId, getColor(Colors::textPrimary));
  setColour(juce::PopupMenu::highlightedTextColourId, kTextPrimary);
  setColour(juce::PopupMenu::highlightedBackgroundColourId, kHighlightBg);
  setColour(juce::BubbleComponent::backgroundColourId, kBgDark);
  setColour(juce::BubbleComponent::outlineColourId, kBorder);

  // Slider：统一灰阶（避免默认主题蓝色）
  setColour(juce::Slider::backgroundColourId, juce::Colour{0xFF2A2A2A});
  setColour(juce::Slider::trackColourId, juce::Colour{0xFF9A9A9A});
  setColour(juce::Slider::thumbColourId, kTextPrimary);
  setColour(juce::Slider::textBoxTextColourId, kTextPrimary);
  setColour(juce::Slider::textBoxBackgroundColourId, kBgDark);
  setColour(juce::Slider::textBoxOutlineColourId, kBorder);
  setColour(juce::Slider::textBoxHighlightColourId, kHighlightBg);

  // used to set the font of the default standalone plugin window
  getDefaultLookAndFeel().setDefaultSansSerifTypeface(
      interMedium().getTypeface());
}

juce::FontOptions CustomLookAndFeel::getSideLabelsFont() {
  return interMedium().withPointHeight(10.f);
}

juce::BorderSize<int> CustomLookAndFeel::getLabelBorderSize(juce::Label&) {
  return juce::BorderSize{0};
}

juce::FontOptions CustomLookAndFeel::getRateLabelFont() {
  return interBold().withPointHeight(12.f);
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                         int x,
                                         int y,
                                         int width,
                                         int height,
                                         float sliderPos,
                                         const float rotaryStartAngle,
                                         const float rotaryEndAngle,
                                         juce::Slider&) {
  const auto bounds = juce::Rectangle{x, y, width, height};
  const auto knobCanalBounds = bounds.toFloat().reduced(3.75f);

  g.setColour(kBgMid);
  g.fillEllipse(knobCanalBounds);

  const auto valueArcBounds = knobCanalBounds.reduced(0.25f);

  juce::Path arc;
  const auto toAngle =
      rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
  arc.addPieSegment(valueArcBounds, rotaryStartAngle, toAngle, 0.f);
  g.setColour(juce::Colour{0xFFBDBDBD});
  g.fillPath(arc);

  const auto knobBounds = knobCanalBounds.reduced(4.f);

  auto knobFill = juce::ColourGradient::vertical(
      juce::Colour{0xFF3A3A3A}, juce::Colour{0xFF161616}, knobBounds);
  knobFill.addColour(0.29, juce::Colour{0xFF2E2E2E});
  knobFill.addColour(0.75, juce::Colour{0xFF222222});
  g.setGradientFill(knobFill);
  g.fillEllipse(knobBounds);

  // Knob stroke
  g.setColour(juce::Colour{0x403A3A3A});
  constexpr auto knobStrokeThickness = 1.33f;
  g.drawEllipse(knobBounds.reduced(knobStrokeThickness / 2.f),
                knobStrokeThickness);

  // Knob top
  const auto knobTopBounds = knobBounds.reduced(7.f);
  auto knobTopFill = juce::ColourGradient{juce::Colour{0xFF6A6A6A},
                                           knobTopBounds.getCentreX(),
                                           knobTopBounds.getY() - 7.f,
                                           juce::Colour{0xFF1A1A1A},
                                           knobTopBounds.getCentreX(),
                                           knobTopBounds.getBottom() + 41.f,
                                           true};
  knobTopFill.addColour(0.66, juce::Colour{0xFF2A2A2A});
  g.setGradientFill(knobTopFill);
  g.fillEllipse(knobTopBounds);

  // Knob top edge
  auto knobTopEdgeFill = juce::ColourGradient{juce::Colour{0xFFBDBDBD},
                                               knobTopBounds.getCentreX(),
                                               knobTopBounds.getY(),
                                               juce::Colour{0xFF1A1A1A},
                                               knobTopBounds.getCentreX(),
                                               knobTopBounds.getBottom() + 6.f,
                                               true};
  knobTopEdgeFill.addColour(0.55, juce::Colour{0xFF8A8A8A});
  g.setGradientFill(knobTopEdgeFill);
  g.setOpacity(0.1f);
  g.drawEllipse(knobTopBounds, 1.f);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g,
                                     int width,
                                     int height,
                                     bool /* isButtonDown */,
                                     int /* buttonX */,
                                     int /* buttonY */,
                                     int /* buttonW */,
                                     int /* buttonH */,
                                     juce::ComboBox&) {
  const auto boxBounds = juce::Rectangle{0, 0, width, height}.toFloat();

  drawButtonInset(g, boxBounds);

  const auto buttonBounds = boxBounds.reduced(buttonInsetWidth);
  drawNeutralGradientButton(g, buttonBounds, false);

  auto arrowBounds = boxBounds.reduced(10.f, 11.f);
  arrowBounds.removeFromLeft(104);
  juce::Path arrow;
  arrow.startNewSubPath(arrowBounds.getTopLeft());
  arrow.lineTo(arrowBounds.getCentreX(), arrowBounds.getBottom());
  arrow.lineTo(arrowBounds.getTopRight());

  g.setColour(kTextSecondary);
  g.fillPath(arrow);
}

juce::Font CustomLookAndFeel::getComboBoxFont(juce::ComboBox&) {
  return interMedium().withPointHeight(12.f);
}

void CustomLookAndFeel::positionComboBoxText(juce::ComboBox& comboBox,
                                             juce::Label& labelToPosition) {
  auto bounds = comboBox.getLocalBounds().reduced(10, 6);
  bounds.removeFromRight(12);
  labelToPosition.setBounds(bounds);
  labelToPosition.setJustificationType(juce::Justification::centred);
  labelToPosition.setFont(getComboBoxFont(comboBox));
}

juce::PopupMenu::Options CustomLookAndFeel::getOptionsForComboBoxPopupMenu(
    juce::ComboBox& box,
    juce::Label& label) {
  const auto menuBounds = box.getScreenBounds().reduced(2, 0);
  return juce::LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label)
      .withStandardItemHeight(24)
      .withTargetScreenArea(menuBounds)
      .withMinimumWidth(128);
}

juce::Font CustomLookAndFeel::getPopupMenuFont() {
  return interMedium().withPointHeight(12.f);
}

juce::Path CustomLookAndFeel::getTickShape(float) {
  return {};
}

void CustomLookAndFeel::drawToggleButton(juce::Graphics& g,
                                         juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown) {
  juce::ignoreUnused(shouldDrawButtonAsDown, shouldDrawButtonAsHighlighted);

  const auto bounds = button.getLocalBounds().toFloat();

  drawButtonInset(g, bounds);
  const auto buttonBounds = bounds.reduced(buttonInsetWidth);

  const bool isOn = button.getToggleState();
  drawNeutralGradientButton(g, buttonBounds, isOn);
  g.setColour(isOn ? kTextPrimary : kTextSecondary);
  g.setFont((isOn ? interBold() : interMedium()).withPointHeight(12.f));
  g.drawText(button.getButtonText(), bounds, juce::Justification::centred,
             false);
}

juce::FontOptions CustomLookAndFeel::interMedium() {
  static const auto result = juce::Typeface::createSystemTypefaceFor(
      assets::InterMedium_ttf, assets::InterMedium_ttfSize);
  return juce::FontOptions{result};
}

juce::FontOptions CustomLookAndFeel::interBold() {
  static const auto result = juce::Typeface::createSystemTypefaceFor(
      assets::InterBold_ttf, assets::InterBold_ttfSize);
  return juce::FontOptions{result};
}
}  // namespace tremolo