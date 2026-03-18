#pragma once

namespace tremolo {
class MessageOnClick : private juce::MouseListener {
public:
  MessageOnClick(juce::Component& topLevelComponent,
                 juce::Component& clickTarget,
                 juce::String messageOnClick)
      : parent{topLevelComponent},
        target{clickTarget},
        message{std::move(messageOnClick)} {
    popup.setAllowedPlacement(juce::BubbleComponent::BubblePlacement::below);
    popup.setAlwaysOnTop(true);

    message.setColour(
        CustomLookAndFeel::getColor(CustomLookAndFeel::Colors::textPrimary));
    message.setJustification(juce::Justification::centred);

    target.addMouseListener(this, true);
  }

  ~MessageOnClick() override { target.removeMouseListener(this); }

  void mouseDoubleClick(const juce::MouseEvent&) override { displayPopup(); }

private:
  void displayPopup() {
    // 只有第一次调用 addChildComponent() 会生效
    parent.addChildComponent(popup);

    if (!popup.isVisible()) {
      popup.showAt(&target, message, 0, true);
    }
  }

  juce::Component& parent;
  juce::Component& target;
  juce::AttributedString message;
  juce::BubbleMessageComponent popup;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MessageOnClick)
};
}  // namespace tremolo