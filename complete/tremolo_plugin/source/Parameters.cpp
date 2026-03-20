namespace tremolo {
namespace {
auto& addParameterToProcessor(juce::AudioProcessor& processor, auto parameter) {
  auto& result = *parameter;
  processor.addParameter(parameter.release());
  return result;
}

juce::AudioParameterFloat& createModulationRateParameter(
    juce::AudioProcessor& processor) {
  constexpr auto versionHint = 1;
  return addParameterToProcessor(
      processor,
      std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{"modulation.rate", versionHint}, "Modulation rate",
          juce::NormalisableRange<float>{0.1f, 20.f, 0.01f, 0.4f},
          tremolo::defaults::modulationRateHz,
          juce::AudioParameterFloatAttributes{}.withLabel("Hz")));
}

juce::AudioParameterChoice& createWaveformParameter(
    juce::AudioProcessor& processor) {
  constexpr auto versionHint = 1;
  return addParameterToProcessor(
      processor,
      std::make_unique<juce::AudioParameterChoice>(
          juce::ParameterID{"modulation.waveform", versionHint},
          "Modulation waveform", juce::StringArray{"Sine", "Triangle"},
          tremolo::defaults::waveformIndex));
}

juce::AudioParameterFloat& createXValueParameter(
    juce::AudioProcessor& processor) {
  constexpr auto versionHint = 1;
  return addParameterToProcessor(
      processor,
      std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{"xy.x", versionHint}, "X Value",
          juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
          tremolo::defaults::xyX,
          juce::AudioParameterFloatAttributes{}.withLabel("")));
}

juce::AudioParameterFloat& createYValueParameter(
    juce::AudioProcessor& processor) {
  constexpr auto versionHint = 1;
  return addParameterToProcessor(
      processor,
      std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{"xy.y", versionHint}, "Y Value",
          juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
          tremolo::defaults::xyY,
          juce::AudioParameterFloatAttributes{}.withLabel("")));
}

juce::AudioParameterFloat& createGainParameter(
    juce::AudioProcessor& processor) {
  constexpr auto versionHint = 1;
  return addParameterToProcessor(
      processor,
      std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{"gain", versionHint}, "Gain",
          juce::NormalisableRange<float>{0.1f, 10.0f, 0.1f},
          tremolo::defaults::gain,
          juce::AudioParameterFloatAttributes{}.withLabel("x")));
}

juce::AudioParameterFloat& createLevelCaptureWindowMsParameter(
    juce::AudioProcessor& processor) {
  constexpr auto versionHint = 1;
  return addParameterToProcessor(
      processor,
      std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{"level.captureWindowMs", versionHint},
          "Level capture window",
          juce::NormalisableRange<float>{tremolo::defaults::levelCaptureWindowMsMin,
                                         tremolo::defaults::levelCaptureWindowMsMax,
                                         tremolo::defaults::levelCaptureWindowMsStep,
                                         tremolo::defaults::levelCaptureWindowMsSkew},
          tremolo::defaults::levelCaptureWindowMs,
          juce::AudioParameterFloatAttributes{}.withLabel("ms")));
}
}  // namespace

Parameters::Parameters(juce::AudioProcessor& processor)
    : rate{createModulationRateParameter(processor)},
      waveform{createWaveformParameter(processor)},
      xValue{createXValueParameter(processor)},
      yValue{createYValueParameter(processor)},
      gain{createGainParameter(processor)},
      levelCaptureWindowMs{createLevelCaptureWindowMsParameter(processor)} {}
}  // namespace tremolo