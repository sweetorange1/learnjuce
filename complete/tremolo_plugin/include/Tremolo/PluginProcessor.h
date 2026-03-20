#pragma once

#include <atomic>
#include <cstdint>

namespace tremolo {
class PluginProcessor : public juce::AudioProcessor {
public:
  PluginProcessor();

  void prepareToPlay(double sampleRate, int expectedMaxFramesPerBlock) override;

  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  void releaseResources() override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  [[nodiscard]] Parameters& getParameterRefs() noexcept;
  
  // 获取Tremolo实例（用于指示灯控制）
  Tremolo& getTremolo() noexcept { return tremolo; }

  // 获取“MIDI触发序列号”（每当本block里有任何MIDI事件输入时递增一次）
  uint64_t getMidiTriggerSequence() const noexcept;

  // MIDI触发模式（用于UI：是否用MIDI NoteOn来触发指示灯/动画）
  void setMidiModeEnabled(bool enabled) noexcept;
  bool getMidiModeEnabled() const noexcept;

  /**
   *
   * @param bufferToFill
   */
  void readAllLfoSamples(juce::AudioBuffer<float>& bufferToFill);

  /** @brief Retrieves the most recent sample rate the processor was given
   * in a thread-safe manner */
  double getSampleRateThreadSafe() const noexcept;

  /** Returns latest input peak (linear gain 0..1+) sampled on audio thread. */
  float getLatestInputLevel() const noexcept;

  /** Returns latest windowed input peak (linear gain 0..1+) computed across multiple blocks. */
  float getLatestWindowedInputPeak() const noexcept;

  void setLevelCaptureWindowMs(float windowMs) noexcept;
  float getLevelCaptureWindowMs() const noexcept;

  void setTriggerThresholdDb(float thresholdDb) noexcept;
  float getTriggerThresholdDb() const noexcept;
  void setInputFilterFrequencies(float highpassHz, float lowpassHz) noexcept;
  float getInputHighpassHz() const noexcept;
  float getInputLowpassHz() const noexcept;

private:
  void updateDetectionFilterCoefficients(float highpassHz,
                                         float lowpassHz) noexcept;
  float analyseFilteredInputPeak(const juce::AudioBuffer<float>& buffer) noexcept;
  void pushFilteredSamplesAndMaybeUpdateWindowPeak(
      const juce::AudioBuffer<float>& buffer) noexcept;
  void resetLevelCaptureState() noexcept;

  Parameters parameters{*this};
  Tremolo tremolo;
  std::atomic<double> currentSampleRate{0.};
  std::atomic<float> latestInputLevel{0.0f};
  std::atomic<float> latestWindowedInputPeak{0.0f};
  std::atomic<float> triggerThresholdDb{tremolo::defaults::triggerThresholdDb};

  // MIDI输入触发序列号：每个processBlock只要收到任意MIDI事件就+1
  std::atomic<uint64_t> midiTriggerSequence{0};

  // UI模式：是否启用MIDI触发（需要持久化到插件状态）
  std::atomic<bool> midiModeEnabled{false};

  std::atomic<float> inputHighpassHz{tremolo::defaults::inputHighpassHz};
  std::atomic<float> inputLowpassHz{tremolo::defaults::inputLowpassHz};
  std::vector<juce::IIRFilter> detectionHighpassFilters;
  std::vector<juce::IIRFilter> detectionLowpassFilters;
  float activeInputHighpassHz{tremolo::defaults::inputHighpassHz};
  float activeInputLowpassHz{tremolo::defaults::inputLowpassHz};

  // 电平捕捉窗口（跨多个block累计）
  int levelCaptureTargetSamples{0};
  int levelCaptureAccumulatedSamples{0};
  float levelCaptureRunningPeak{0.0f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
}  // namespace tremolo