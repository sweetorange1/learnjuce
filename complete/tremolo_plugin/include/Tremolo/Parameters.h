#pragma once

namespace tremolo {
struct Parameters {
  explicit Parameters(juce::AudioProcessor&);

  juce::AudioParameterFloat& rate;
  juce::AudioParameterBool& bypassed;
  juce::AudioParameterChoice& waveform;
  juce::AudioParameterFloat& xValue; // XY控制器的X值参数
  juce::AudioParameterFloat& yValue; // XY控制器的Y值参数

  JUCE_DECLARE_NON_COPYABLE(Parameters)
  JUCE_DECLARE_NON_MOVEABLE(Parameters)
};
}  // namespace tremolo
