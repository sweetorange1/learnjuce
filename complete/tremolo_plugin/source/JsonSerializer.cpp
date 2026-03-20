namespace {
struct SerializableParameters {
  float rate;
  juce::String waveform;
  float x{tremolo::defaults::xyX};
  float y{tremolo::defaults::xyY};
  float levelCaptureWindowMs{tremolo::defaults::levelCaptureWindowMs};

  // v2/3: 额外包含 gain；v4: 移除 gain（MAX GAIN 改为 Defaults 固定配置）
  static constexpr auto marshallingVersion = 4;

  template <typename Archive, typename T>
  static void serialise(Archive& archive, T& p) {
    using namespace juce;

    const auto version = archive.getVersion();
    if (version != 1 && version != 2 && version != 3 && version != 4) {
      return;
    }

    std::string pluginName = TREMOLO_PLUGIN_NAME;

    archive(named("pluginName", pluginName));

    if (pluginName != TREMOLO_PLUGIN_NAME) {
      return;
    }

    archive(named("modulationRateHz", p.rate),
            named("modulationWaveform", p.waveform));

    if (version >= 2) {
      // v2/3: 还会写入 gain；我们在反序列化时允许旧字段存在，但新版本不再写入。
      archive(named("xyX", p.x), named("xyY", p.y));
    }

    if (version >= 3) {
      archive(named("levelCaptureWindowMs", p.levelCaptureWindowMs));
    }
  }
};

SerializableParameters from(const tremolo::Parameters& p) {
  return {
      .rate = p.rate.get(),
      .waveform = p.waveform.getCurrentChoiceName(),
      .x = p.xValue.get(),
      .y = p.yValue.get(),
      .levelCaptureWindowMs = p.levelCaptureWindowMs.get(),
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

  const auto version = static_cast<int>(parsedResult.getProperty("__version__", 0));
  if (version >= 2) {
    parameters.xValue = parsedParameters->x;
    parameters.yValue = parsedParameters->y;
  }

  if (version >= 3) {
    parameters.levelCaptureWindowMs = parsedParameters->levelCaptureWindowMs;
  }

  return juce::Result::ok();
}
}  // namespace tremolo