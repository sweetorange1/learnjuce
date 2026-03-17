namespace {
struct SerializableParameters {
  float rate;
  bool bypassed;
  juce::String waveform;
  float x{0.5f};
  float y{0.5f};
  float gain{1.0f};

  static constexpr auto marshallingVersion = 2;

  template <typename Archive, typename T>
  static void serialise(Archive& archive, T& p) {
    using namespace juce;

    const auto version = archive.getVersion();
    if (version != 1 && version != 2) {
      return;
    }

    std::string pluginName = TREMOLO_PLUGIN_NAME;

    archive(named("pluginName", pluginName));

    if (pluginName != TREMOLO_PLUGIN_NAME) {
      return;
    }

    archive(named("modulationRateHz", p.rate), named("bypassed", p.bypassed),
            named("modulationWaveform", p.waveform));

    if (version >= 2) {
      archive(named("xyX", p.x), named("xyY", p.y), named("gain", p.gain));
    }
  }
};

SerializableParameters from(const tremolo::Parameters& p) {
  return {
      .rate = p.rate.get(),
      .bypassed = p.bypassed.get(),
      .waveform = p.waveform.getCurrentChoiceName(),
      .x = p.xValue.get(),
      .y = p.yValue.get(),
      .gain = p.gain.get(),
  };
}
}  // namespace

namespace tremolo {
void JsonSerializer::serialize(const Parameters& parameters,
                               juce::OutputStream& output) {
  const auto json = juce::ToVar::convert(from(parameters));

  if (!json.has_value()) {
    return;
  }

  juce::JSON::writeToStream(output, *json,
                            juce::JSON::FormatOptions{}
                                .withSpacing(juce::JSON::Spacing::multiLine)
                                .withMaxDecimalPlaces(2));
}

juce::Result JsonSerializer::deserialize(juce::InputStream& input,
                                         Parameters& parameters) {
  juce::var parsedResult;
  auto parsingResult =
      juce::JSON::parse(input.readEntireStreamAsString(), parsedResult);

  if (parsingResult.failed()) {
    return parsingResult;
  }

  const auto parsedParameters =
      juce::FromVar::convert<SerializableParameters>(parsedResult);

  if (!parsedParameters.has_value()) {
    return juce::Result::fail(
        "failed to parse parameters from JSON representation");
  }

  const auto modulationWaveformIndex =
      parameters.waveform.choices.indexOf(parsedParameters->waveform);
  if (modulationWaveformIndex < 0) {
    // don't update parameters if modulation waveform name is invalid
    return juce::Result::fail(
        "invalid modulation waveform name; supported values are: " +
        parameters.waveform.choices.joinIntoString(", "));
  }

  parameters.waveform = modulationWaveformIndex;
  parameters.rate = parsedParameters->rate;
  parameters.bypassed = parsedParameters->bypassed;

  const auto version = static_cast<int>(parsedResult.getProperty("__version__", 0));
  if (version >= 2) {
    parameters.xValue = parsedParameters->x;
    parameters.yValue = parsedParameters->y;
    parameters.gain = parsedParameters->gain;
  }

  return juce::Result::ok();
}
}  // namespace tremolo
