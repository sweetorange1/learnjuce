#pragma once

namespace tremolo {
/**
 * 用于在单个块内实现旁路状态转换的类。
 *
 * 它提供两个功能：
 *  - 检测旁路状态转换的发生
 *  - 根据转换类型对干（未处理）和湿（已处理）缓冲区进行淡入淡出
 *
 * 在插件处理器的processBlock()方法中使用以下代码：
 *
 * @code
 * //...
 *
 * bypassTransitionSmoother.setBypass(parameters.bypassed);
 *
 * if (bypassTransitionSmoother.isTransitioning()) {
 *   bypassTransitionSmoother.setDryBuffer(buffer);
 *
 *   yourAudioEffectClassInstance.process(buffer);
 *
 *   bypassTransitionSmoother.mixToWetBuffer(buffer);
 *
 *   return;
 * }
 *
 * // 如果旁路则避免处理，否则进行处理...
 * @endcode
 *
 * 或者，你可以无条件调用setDryBuffer()和mixToWetBuffer()，如下所示：
 *
 * @code
 * //...
 *
 * bypassTransitionSmoother.setBypass(parameters.bypassed);
 *
 * if (parameters.bypassed && !bypassTransitionSmoother.isTransitioning()) {
 *   // 如果插件处于旁路状态且没有转换，则避免处理
 *   return;
 * }
 *
 * bypassTransitionSmoother.setDryBuffer(buffer);
 * yourAudioEffectClassInstance.process(buffer);
 * bypassTransitionSmoother.mixToWetBuffer(buffer);
 *
 * // 无需更多处理
 * @endcode
 *
 * 记得在prepareToPlay()中调用prepare()，
 * 在setStateInformation()中调用setBypassForced()，
 * 在releaseResources()中调用reset()。
 */
class BypassTransitionSmoother {
public:
  explicit BypassTransitionSmoother(double crossfadeLengthSecondsValue = 0.01)
      : crossfadeLengthSeconds{crossfadeLengthSecondsValue} {
    jassert(0.0 < crossfadeLengthSeconds);

    reset();
  }

  void prepare(const juce::dsp::ProcessSpec& spec) {
    sampleRateHz = spec.sampleRate;
    dryBuffer.setSize(static_cast<int>(spec.numChannels),
                      static_cast<int>(spec.maximumBlockSize));
    dryGain.reset(spec.sampleRate, crossfadeLengthSeconds);
    wetGain.reset(spec.sampleRate, crossfadeLengthSeconds);
    reset();
  }

  void setBypass(bool bypass) noexcept {
    if (bypass == isBypassed()) {
      return;
    }

    const auto current = dryGain.getCurrentValue();
    const auto target = bypass ? 1.0f : 0.0f;
    const auto duration = crossfadeLengthSeconds * std::abs(target - current);

    dryGain.reset(sampleRateHz, duration);
    wetGain.reset(sampleRateHz, duration);

    dryGain.setCurrentAndTargetValue(current);
    dryGain.setTargetValue(target);

    wetGain.setCurrentAndTargetValue(1.0f - current);
    wetGain.setTargetValue(1.0f - target);
  }

  void setBypassForced(bool bypass) noexcept {
    dryGain.setCurrentAndTargetValue(bypass ? 1.0f : 0.0f);
    wetGain.setCurrentAndTargetValue(1.0f - dryGain.getTargetValue());
  }

  [[nodiscard]] bool isTransitioning() const noexcept {
    return dryGain.isSmoothing() || wetGain.isSmoothing();
  }

  void setDryBuffer(const juce::AudioBuffer<float>& buffer) noexcept {
    if (shouldAvoidProcessing()) {
      // 插件正在运行：无需存储干缓冲区
      return;
    }

    jassert(buffer.getNumSamples() <= dryBuffer.getNumSamples());
    jassert(buffer.getNumChannels() <= dryBuffer.getNumChannels());

    for (const auto channel : std::views::iota(0, buffer.getNumChannels())) {
      dryBuffer.copyFrom(channel, 0, buffer, channel, 0,
                         buffer.getNumSamples());
    }
    dryGain.applyGain(dryBuffer, buffer.getNumSamples());
  }

  void mixToWetBuffer(juce::AudioBuffer<float>& buffer) noexcept {
    if (shouldAvoidProcessing()) {
      // 插件正在运行：无需修改湿缓冲区
      return;
    }

    jassert(buffer.getNumSamples() <= dryBuffer.getNumSamples());
    jassert(buffer.getNumChannels() <= dryBuffer.getNumChannels());

    wetGain.applyGain(buffer, buffer.getNumSamples());
    for (const auto channel : std::views::iota(0, buffer.getNumChannels())) {
      buffer.addFrom(channel, 0, dryBuffer, channel, 0, buffer.getNumSamples());
    }
  }

  void reset() noexcept {
    setBypassForced(false);
    dryBuffer.clear();
  }

private:
  [[nodiscard]] bool isBypassed() const noexcept {
    return juce::exactlyEqual(dryGain.getTargetValue(), 1.0f);
  }

  [[nodiscard]] bool shouldAvoidProcessing() const noexcept {
    return !isTransitioning() && !isBypassed();
  }

  double crossfadeLengthSeconds = 0.0;
  double sampleRateHz = 0.0;
  juce::LinearSmoothedValue<float> dryGain{0.f};
  juce::LinearSmoothedValue<float> wetGain{1.f};
  juce::AudioBuffer<float> dryBuffer;
};
}  // namespace tremolo