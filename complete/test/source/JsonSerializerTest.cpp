#include <tremolo_plugin/tremolo_plugin.h>
#include <gtest/gtest.h>

namespace tremolo {
TEST(JsonSerializer, SerializeToString) {
  PluginProcessor processor;
  auto& parameters = processor.getParameterRefs();

  parameters.rate = 10.f;
  parameters.bypassed = true;
  parameters.waveform = 1;
  parameters.xValue = 0.25f;
  parameters.yValue = 0.75f;
  parameters.gain = 2.0f;

  const juce::String expectedOutput =
      u8R"({
  "__version__": 2,
  "pluginName": "Tremolo",
  "modulationRateHz": 10.0,
  "bypassed": true,
  "modulationWaveform": "Triangle",
  "xyX": 0.25,
  "xyY": 0.75,
  "gain": 2.0
})";
  juce::MemoryBlock block;
  juce::MemoryOutputStream outputStream{block, false};

  JsonSerializer::serialize(parameters, outputStream);
  outputStream.flush();

  const auto result = outputStream.toUTF8().removeCharacters("\r");

  EXPECT_EQ(expectedOutput, result);
}

TEST(JsonSerializer, DeserializeFromString) {
  const juce::String savedParameters =
      u8R"({
  "__version__": 2,
  "pluginName": "Tremolo",
  "modulationRateHz": 10.0,
  "bypassed": true,
  "modulationWaveform": "Triangle",
  "xyX": 0.25,
  "xyY": 0.75,
  "gain": 2.0
})";

  juce::MemoryInputStream inputStream{
      savedParameters.getCharPointer(),
      static_cast<size_t>(savedParameters.length()), false};

  PluginProcessor processor;
  auto& parameters = processor.getParameterRefs();

  const auto result = JsonSerializer::deserialize(inputStream, parameters);

  EXPECT_TRUE(result.wasOk());
  EXPECT_FLOAT_EQ(parameters.rate, 10.f);
  EXPECT_TRUE(parameters.bypassed);
  EXPECT_EQ(juce::String{"Triangle"},
            parameters.waveform.getCurrentChoiceName());
  EXPECT_FLOAT_EQ(parameters.xValue.get(), 0.25f);
  EXPECT_FLOAT_EQ(parameters.yValue.get(), 0.75f);
  EXPECT_FLOAT_EQ(parameters.gain.get(), 2.0f);
}

TEST(JsonSerializer, DontUpdateParametersWhenWaveformNameIsInvalid) {
  // given
  const juce::String savedParameters =
      u8R"({
  "__version__": 1,
  "pluginName": "Tremolo",
  "modulationRateHz": 10.0,
  "bypassed": true,
  "modulationWaveform": "Foo"
})";

  juce::MemoryInputStream inputStream{
      savedParameters.getCharPointer(),
      static_cast<size_t>(savedParameters.length()), false};

  PluginProcessor processor;
  auto& parameters = processor.getParameterRefs();
  parameters.waveform = 0;
  parameters.bypassed = false;
  parameters.rate = 5.f;

  // when
  const auto result = JsonSerializer::deserialize(inputStream, parameters);

  // then
  EXPECT_TRUE(result.failed());
  EXPECT_FLOAT_EQ(parameters.rate.get(), 5.f);
  EXPECT_FALSE(parameters.bypassed.get());
  EXPECT_EQ(0, parameters.waveform.getIndex());
}
}  // namespace tremolo