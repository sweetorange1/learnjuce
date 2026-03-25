#pragma once

#include <atomic>
#include <cstdint>

#include "Defaults.h"

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

  // BPM触发序列号：当BPM节拍到点（按设定音符时值）则递增一次
  uint64_t getBpmTriggerSequence() const noexcept;

  // BPM触发模式（用于UI：是否用宿主BPM来触发指示灯/动画）
  void setBpmModeEnabled(bool enabled) noexcept;
  bool getBpmModeEnabled() const noexcept;

  // BPM触发频率：音符时值索引（0..defaults::bpmDivisionCount-1）
  void setBpmDivisionIndex(int index) noexcept;
  int getBpmDivisionIndex() const noexcept;

  // XY皮肤：当前选择（需要持久化到插件状态）
  void setXYSkinId(tremolo::defaults::XYSkinId skin) noexcept;
  tremolo::defaults::XYSkinId getXYSkinId() const noexcept;

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

  // BPM触发：由宿主BPM驱动（需要持久化到插件状态）
  std::atomic<uint64_t> bpmTriggerSequence{0};
  std::atomic<bool> bpmModeEnabled{false};
  std::atomic<int> bpmDivisionIndex{tremolo::defaults::bpmDivisionIndexDefault};

  // XY皮肤：当前选择（需要持久化到插件状态）
  std::atomic<int> xySkinId{static_cast<int>(tremolo::defaults::xyDefaultSkin)};

  // BPM节拍边界跟踪（仅音频线程使用，用于避免每个block都触发）
  bool bpmHasLastStep{false};
  int64_t bpmLastStepIndex{0};
  int bpmLastDivisionIndex{tremolo::defaults::bpmDivisionIndexDefault};

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