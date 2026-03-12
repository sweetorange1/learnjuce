#pragma once
#include <juce_dsp/juce_dsp.h>

namespace tremolo {

// 颤音效果器主类（简化版，只保留增益和clipper效果）
class Tremolo {
public:
  // 构造函数
  Tremolo() = default;

  // 准备音频处理环境（添加滤波器初始化）
  void prepare(double sampleRate, int expectedMaxFramesPerBlock) {
    // 初始化低通和高通滤波器
    lowPassFilter.setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, 24000.0f));
    highPassFilter.setCoefficients(juce::IIRCoefficients::makeHighPass(sampleRate, 100.0f));
    
    // 重置滤波器状态
    lowPassFilter.reset();
    highPassFilter.reset();
    
    // 重置电平检测器
    peakLevel = 0.0f;
    isFlashing = false;
    flashTimer = 0.0f;
  }

  // 设置XY控制器的值
  void setXYValues(float x, float y) noexcept {
    xValue = x;  // 当前X值
    yValue = y;  // 当前Y值
    
    // 根据XY位置计算增益（距离中心点越近，增益越大）
    constexpr float targetX = 0.5f;  // 目标点X坐标
    constexpr float targetY = 0.38f;  // 目标点Y坐标
    
    // 计算到目标点的距离
    const auto distanceToTarget = std::sqrt(std::pow(x - targetX, 2.0f) + std::pow(y - targetY, 2.0f));
    
    // 将距离映射到增益（maxGain到1.0）
    gainBoost = juce::jmap(distanceToTarget, 0.0f, std::sqrt(0.5f), maxGain, 1.0f);
  }

  // 设置最大增益值
  void setMaxGain(float maxGainValue) noexcept {
    maxGain = maxGainValue;  // 设置最大增益值
  }

  // 主音频处理函数（添加信号拆分和电平检测）
  void process(juce::AudioBuffer<float>& buffer) noexcept {
    // Clipper阈值参数
    constexpr float thresholdHigh = 1.0f;   // 软削波上限阈值
    constexpr float thresholdLow = -1.0f;   // 负向阈值

    // 处理每个音频帧
    for (const auto frameIndex : std::views::iota(0, buffer.getNumSamples())) {
      // 处理每个通道
      for (const auto channelIndex : std::views::iota(0, buffer.getNumChannels())) {
        // 获取输入样本
        const auto inputSample = buffer.getSample(channelIndex, frameIndex);

        // === 信号拆分和电平检测 ===
        // 复制输入信号用于检测（不传回宿主）
        const auto detectionSample = inputSample;
        
        // 应用低通滤波器
        const auto lowPassed = lowPassFilter.processSingleSampleRaw(detectionSample);
        
        // 应用高通滤波器
        const auto filteredSample = highPassFilter.processSingleSampleRaw(lowPassed);
        
        // 计算峰值电平（绝对值）
        const auto absSample = std::abs(filteredSample);
        if (absSample > peakLevel) {
          peakLevel = absSample;
        }

        // === 主信号处理 ===
        // 应用增益提升（基于XY控制器位置）
        const auto boostedSample = inputSample * gainBoost;
        
        // 应用Clipper算法（硬限制）防止削波失真
        const auto clippedSample = juce::jlimit(thresholdLow, thresholdHigh, boostedSample);

        // 设置输出样本
        buffer.setSample(channelIndex, frameIndex, clippedSample);
      }
    }
    
    // 电平检测和指示灯控制
    updateLevelDetection();
  }

  // 重置所有状态
  void reset() noexcept {
    lowPassFilter.reset();
    highPassFilter.reset();
    peakLevel = 0.0f;
    isFlashing = false;
    flashTimer = 0.0f;
  }

  // 获取当前电平检测状态（用于指示灯）
  bool shouldFlashIndicator() const noexcept {
    return isFlashing;
  }

  // 更新指示灯状态（需要在音频线程外调用）
  void updateIndicatorState(float deltaTime) noexcept {
    if (isFlashing) {
      flashTimer -= deltaTime;
      if (flashTimer <= 0.0f) {
        isFlashing = false;
        flashTimer = 0.0f;
      }
    }
  }

private:
  // 电平检测逻辑
  void updateLevelDetection() noexcept {
    // 将峰值电平转换为dB
    const auto peakDB = juce::Decibels::gainToDecibels(peakLevel);
    
    // 检查是否超过阈值
    const bool isAboveThreshold = peakDB > thresholdDB;
    
    // 如果从低于阈值变为高于阈值，触发闪烁
    if (isAboveThreshold && !wasAboveThreshold) {
      isFlashing = true;
      flashTimer = flashDuration;
    }
    
    // 更新状态
    wasAboveThreshold = isAboveThreshold;
    
    // 重置峰值电平用于下一帧检测
    peakLevel = 0.0f;
  }

private:
  // XY控制器参数
  float xValue = 0.5f;        // 当前X值
  float yValue = 0.4f;        // 当前Y值
  float gainBoost = 1.0f;     // 当前增益值（基于XY位置计算）
  float maxGain = 4.0f;       // 最大增益值（由控制条设置）
  
  // 信号检测相关参数
  juce::IIRFilter lowPassFilter;     // 低通滤波器
  juce::IIRFilter highPassFilter;    // 高通滤波器
  float peakLevel = 0.0f;            // 峰值电平
  bool isFlashing = false;           // 指示灯闪烁状态
  float flashTimer = 0.0f;           // 闪烁计时器
  const float thresholdDB = -12.0f;   // 触发阈值（-6dB）
  const float flashDuration = 1.0f;  // 闪烁持续时间（0.5秒）
  bool wasAboveThreshold = false;    // 上次是否超过阈值
};

}  // namespace tremolo
