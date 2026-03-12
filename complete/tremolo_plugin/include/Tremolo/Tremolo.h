#pragma once

namespace tremolo {
enum class ApplySmoothing { no, yes };

class Tremolo {
public:
  enum class LfoWaveform : size_t {
    sine = 0,
    triangle = 1,
  };

  Tremolo() { setModulationRateHz(5.f, ApplySmoothing::no); }

  void prepare(double sampleRate, int expectedMaxFramesPerBlock) {
    const juce::dsp::ProcessSpec processSpec{
        .sampleRate = sampleRate,
        .maximumBlockSize =
            static_cast<juce::uint32>(expectedMaxFramesPerBlock),
        .numChannels = 1u,
    };
    for (auto& lfo : lfos) {
      lfo.prepare(processSpec);
    }
    lfoSampleFifo.prepare(sampleRate);
    lfoTransitionSmoother.reset(sampleRate, 0.025 /* 25 milliseconds */);

    // allocate defensively
    lfoSamples.resize(4u * static_cast<size_t>(expectedMaxFramesPerBlock));
  }

  void setModulationRateHz(
      float rateHz,
      ApplySmoothing applySmoothing = ApplySmoothing::yes) noexcept {
    const auto force = applySmoothing == ApplySmoothing::no;
    for (auto& lfo : lfos) {
      lfo.setFrequency(rateHz, force);
    }
  }

  void setLfoWaveform(LfoWaveform waveform,
                      ApplySmoothing applySmoothing = ApplySmoothing::yes) {
    jassert(waveform == LfoWaveform::sine || waveform == LfoWaveform::triangle);

    lfoToSet = waveform;

    if (applySmoothing == ApplySmoothing::no) {
      currentLfo = waveform;
    }
  }

  void setXYValues(float x, float y) noexcept {
    xValue = x;
    yValue = y;
    // 根据X和Y值调整调制参数
    // X值可以控制调制深度，Y值可以控制调制频率
    dynamicModulationDepth = juce::jmap(x, 0.0f, 1.0f, 0.1f, 0.8f);
    dynamicModulationRate = juce::jmap(y, 0.0f, 1.0f, 0.5f, 20.0f);
  }

  void process(juce::AudioBuffer<float>& buffer) noexcept {
    // actual updating of the LFO waveform happens in process()
    // to keep setLfoWaveform() idempotent
    updateLfoWaveform();

    // for each frame
    for (const auto frameIndex : std::views::iota(0, buffer.getNumSamples())) {
      // generate the LFO value
      const auto lfoValue = getNextLfoValue();
      lfoSampleFifo.push(lfoValue);

      // calculate the modulation value using dynamic parameters from XY controller
      const auto modulationValue = dynamicModulationDepth * lfoValue + 1.f;

      // calculate volume attenuation based on XY distance from center (0.5, 0.5)
      const auto centerDistance = std::sqrt(std::pow(xValue - 0.5f, 2.0f) + std::pow(yValue - 0.5f, 2.0f));
      // map distance (0 to sqrt(0.5)) to volume (1.0 to 0.0)
      const auto volumeAttenuation = juce::jmap(centerDistance, 0.0f, std::sqrt(0.5f), 1.0f, 0.0f);

      for (const auto channelIndex :
           std::views::iota(0, buffer.getNumChannels())) {
        // get the input sample
        const auto inputSample = buffer.getSample(channelIndex, frameIndex);

        // modulate the sample
        const auto modulatedSample = modulationValue * inputSample;
        
        // apply volume attenuation based on XY distance
        const auto outputSample = modulatedSample * volumeAttenuation;

        // set the output sample
        buffer.setSample(channelIndex, frameIndex, outputSample);
      }
    }
  }

  void processChannelwise(juce::AudioBuffer<float>& buffer) noexcept {
    // LFO波形的实际更新在process()中进行
    // 以保持setLfoWaveform()的幂等性
    updateLfoWaveform();

    const auto samplesToProcess = std::min(
        lfoSamples.size(), static_cast<size_t>(buffer.getNumSamples()));

    // 检测主机是否行为异常；如果此断言失败，则表示处理帧数超过了prepare()中声明的数量
    jassert(samplesToProcess <= lfoSamples.size());

    // 生成LFO信号
    for (const auto i : std::views::iota(0u, samplesToProcess)) {
      lfoSamples[i] = getNextLfoValue();
      lfoSampleFifo.push(lfoSamples[i]);
    }

    // 计算调制值
    juce::FloatVectorOperations::multiply(lfoSamples.data(), modulationDepth,
                                          samplesToProcess);
    juce::FloatVectorOperations::add(lfoSamples.data(), 1.f, samplesToProcess);

    // 对每个通道进行处理
    for (const auto channelIndex :
         std::views::iota(0, buffer.getNumChannels())) {
      juce::FloatVectorOperations::multiply(
          buffer.getWritePointer(channelIndex), lfoSamples.data(),
          samplesToProcess);
    }
  }

  void reset() noexcept {
    for (auto& lfo : lfos) {
      lfo.reset();
    }
    lfoSampleFifo.reset();
  }

  void readAllLfoSamples(juce::AudioBuffer<float>& bufferToFill) {
    lfoSampleFifo.popAll(bufferToFill);
  }

private:
  static constexpr auto modulationDepth = 0.4f;
  float dynamicModulationDepth = modulationDepth; // 动态调制深度，由XY控制器控制
  float dynamicModulationRate = 5.0f; // 动态调制频率，由XY控制器控制
  float xValue = 0.5f; // 当前X值
  float yValue = 0.5f; // 当前Y值

  static float triangle(float phase) {
    // 将相位偏移pi/2，以便在相位为0时返回0
    // 并与正弦波形匹配
    // （否则波形将从1开始）
    const auto offsetPhase = phase - juce::MathConstants<float>::halfPi;

    // 源代码参考：
    // https://thewolfsound.com/sine-saw-square-triangle-pulse-basic-waveforms-in-synthesis/#triangle
    const auto ft = offsetPhase / juce::MathConstants<float>::twoPi;
    return 4.f * std::abs(ft - std::floor(ft + 0.5f)) - 1.f;
  }

  void updateLfoWaveform() {
    if (lfoToSet != currentLfo) {
      // 更新平滑器
      lfoTransitionSmoother.setCurrentAndTargetValue(getNextLfoValue());

      currentLfo = lfoToSet;

      // 启动平滑处理
      lfoTransitionSmoother.setTargetValue(getNextLfoValue());
    }
  }

  float getNextLfoValue() {
    if (lfoTransitionSmoother.isSmoothing()) {
      return lfoTransitionSmoother.getNextValue();
    }
    // the argument is added to the generated sample, thus, we pass in 0
    // to get just the generated sample
    return lfos[juce::toUnderlyingType(currentLfo)].processSample(0.f);
  }

  std::array<juce::dsp::Oscillator<float>, 2u> lfos{
      juce::dsp::Oscillator<float>{[](auto phase) {
        // start phase is -pi -> change it to 0 to match the mathematical sine
        return std::sin(phase + juce::MathConstants<float>::pi);
      }},
      juce::dsp::Oscillator<float>{triangle}};

  LfoWaveform currentLfo = LfoWaveform::sine;
  LfoWaveform lfoToSet = currentLfo;

  juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
      lfoTransitionSmoother{0.f};
  std::vector<float> lfoSamples;

  SampleFifo<float> lfoSampleFifo;
};
}  // namespace tremolo
