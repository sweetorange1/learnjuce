#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Tremolo.h"

namespace tremolo {

// 音频处理器类：继承自JUCE框架的AudioProcessor基类
// 功能：实现音频效果处理的核心逻辑，包括参数管理、音频处理和状态控制
class PluginProcessor : public juce::AudioProcessor {
public:
  // 构造函数：初始化音频处理器
  PluginProcessor();

  // 析构函数：清理资源
  ~PluginProcessor() override;

  // JUCE框架要求的接口方法：提供插件的基本信息
  const juce::String getName() const override;
  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  // 插件程序管理：保存和恢复插件状态
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  // 音频处理准备和释放资源
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  // 音频处理核心方法：对每个音频块进行处理
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  // 编辑器相关方法
  bool hasEditor() const override;
  juce::AudioProcessorEditor* createEditor() override;

  // 状态保存和恢复：序列化和反序列化插件状态
  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  // 参数引用获取方法：提供对插件参数的访问
  struct ParameterRefs {
    juce::AudioParameterBool& bypassed; // 旁路参数引用
    juce::AudioParameterFloat& gain;    // 增益参数引用
    juce::AudioParameterFloat& xValue;  // X值参数引用
    juce::AudioParameterFloat& yValue;  // Y值参数引用
  };
  
  // 获取参数引用：返回参数引用结构体
  ParameterRefs& getParameterRefs();

  // 获取颤音效果器引用
  Tremolo& getTremolo() { return tremolo; }

  // 获取当前采样率（线程安全）
  double getSampleRateThreadSafe() const { return currentSampleRate; }

  // 创建参数布局
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  // 检查音频总线布局是否受支持
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  // 获取旁路参数
  juce::AudioProcessorParameter* getBypassParameter() const noexcept override;

  // 频谱分析器相关方法
  /**
   * @brief 获取输入音频数据用于频谱分析
   * @param buffer 输出缓冲区，用于存储音频数据
   * @param maxSamples 最大采样点数
   */
  void getInputAudioForAnalysis(juce::AudioBuffer<float>& buffer, int maxSamples);
  
  /**
   * @brief 设置频谱分析器激活状态
   * @param shouldBeActive 是否激活频谱分析器
   */
  void setSpectrumAnalyserActive(bool shouldBeActive);

private:
  // 参数管理器：管理插件的所有参数
  juce::AudioProcessorValueTreeState parameters;

  // 颤音效果器：实现核心音频处理逻辑
  Tremolo tremolo;

  // 旁路过渡平滑器：实现旁路状态的平滑过渡
  juce::dsp::DryWetMixer<float> bypassTransitionSmoother;

  // 当前采样率：存储音频处理的采样率
  double currentSampleRate{44100.0};

  // 频谱分析器相关成员变量
  juce::AbstractFifo analysisFifo; // 音频数据FIFO缓冲区
  juce::AudioBuffer<float> inputBufferForAnalysis; // 输入音频分析缓冲区
  bool spectrumAnalyserActive{false}; // 频谱分析器激活状态

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};

}  // namespace tremolo
