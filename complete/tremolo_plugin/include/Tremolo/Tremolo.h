#pragma once
#include <juce_dsp/juce_dsp.h>
#include "Defaults.h"

#include <atomic>

namespace tremolo {

// 颤音效果器主类（简化版，只保留增益和clipper效果）
class Tremolo {
public:
  // 构造函数
  Tremolo() = default;

  // 准备音频处理环境（添加滤波器初始化）
  void prepare(double sampleRate, int expectedMaxFramesPerBlock) {
    juce::ignoreUnused(sampleRate, expectedMaxFramesPerBlock);

    // 重置电平检测器
    peakLevel = 0.0f;
    isFlashing = false;
    flashTimer = 0.0f;
  }

  // 设置XY控制器的值
  void setXYValues(float x, float y) noexcept {
    xValue = x;  // 当前X值
    yValue = y;  // 当前Y值

    // 根据XY位置计算增益（距离目标点越近，增益越大）
    constexpr float targetX = tremolo::defaults::xyTargetX;  // 目标点X坐标
    constexpr float targetY = tremolo::defaults::xyTargetY;  // 目标点Y坐标

    // 计算到目标点的距离
    const auto distanceToTarget = std::sqrt(std::pow(x - targetX, 2.0f) + std::pow(y - targetY, 2.0f));

    // === 增益提升（带距离阈值）===
    // 距离>=阈值：不增益；距离=0：达到最大增益
    const auto gainThreshold = juce::jmax(0.0001f, tremolo::defaults::gainBoostDistanceThreshold);
    const auto configuredMaxGain = juce::jmax(1.0f, tremolo::defaults::maxGain);

    if (distanceToTarget >= gainThreshold) {
      gainBoost = 1.0f;
    } else {
      gainBoost = juce::jmap(distanceToTarget, 0.0f, gainThreshold, configuredMaxGain, 1.0f);
      gainBoost = juce::jlimit(1.0f, configuredMaxGain, gainBoost);
    }

    // 将距离映射到“公式映射湿度”：距离=0 -> 1(全湿)，距离>=阈值 -> 0(全干)
    const auto threshold = juce::jmax(0.0001f, tremolo::defaults::sawWetDistanceThreshold);
    sawWet = juce::jlimit(0.0f, 1.0f, 1.0f - (distanceToTarget / threshold));
  }

  // 设置触发阈值（dB）
  void setThresholdDb(float newThresholdDb) noexcept {
    thresholdDB = juce::jlimit(-60.0f, 0.0f, newThresholdDb);
  }

  // 获取当前触发阈值（dB）
  float getThresholdDb() const noexcept {
    return thresholdDB;
  }

  // 将外部分析得到的峰值送入检测器，用于指示灯触发。
  void updateDetectionPeak(float detectedPeakLinear, float deltaTimeSeconds) noexcept {
    peakLevel = juce::jmax(0.0f, detectedPeakLinear);
    levelDetectionElapsedSeconds += juce::jmax(0.0f, deltaTimeSeconds);
    updateLevelDetection();
  }

  // 主音频处理函数（添加信号拆分和电平检测）
  void process(juce::AudioBuffer<float>& buffer) noexcept {
    // Clipper阈值参数
    constexpr float thresholdHigh = 1.0f;   // 软削波上限阈值
    constexpr float thresholdLow = -1.0f;   // 负向阈值

    // 公式映射参数（来自 Defaults）
    constexpr float pi = 3.14159265358979323846f;
    const auto A = tremolo::defaults::sawMapA;
    const auto B = tremolo::defaults::sawMapB;
    const auto C = tremolo::defaults::sawMapC;

    const auto bTerm = (B * 20.0f + 1.0f);

    // 处理每个音频帧
    for (const auto frameIndex : std::views::iota(0, buffer.getNumSamples())) {
      // 处理每个通道
      for (const auto channelIndex : std::views::iota(0, buffer.getNumChannels())) {
        // 获取输入样本
        const auto inputSample = buffer.getSample(channelIndex, frameIndex);

        // === 公式映射（在 clipper 之前，且用于干湿混合）===
        // y = asinh(x*A*10)/pi + (round((-x/2)*(B*20+1)) - (-x/2)*(B*20+1)) * C
        const auto x = inputSample;
        const auto t = (-x * 0.5f) * bTerm;
        const auto mapped = (std::asinh(x * A * 10.0f) / pi) + ((std::round(t) - t) * C);

        // 根据目标点距离进行干湿混合：距离越近湿度越大
        const auto mixed = (1.0f - sawWet) * x + sawWet * mapped;

        // === 主信号处理 ===
        // 应用增益提升（基于XY控制器位置）
        const auto boostedSample = mixed * gainBoost;

        // 应用Clipper算法（硬限制）防止削波失真
        const auto clippedSample = juce::jlimit(thresholdLow, thresholdHigh, boostedSample);

        // 设置输出样本
        buffer.setSample(channelIndex, frameIndex, clippedSample);
      }
    }
  }

  // 重置所有状态
  void reset() noexcept {
    peakLevel = 0.0f;
    isFlashing = false;
    flashTimer = 0.0f;
  }

  // 获取当前电平检测状态（用于指示灯）
  bool shouldFlashIndicator() const noexcept {
    return isFlashing;
  }

  // 获取“阈值触发次数”（单调递增）。用于UI识别“快速连续触发”并立即重播动画。
  // 注意：该值只在“从低于阈值->高于阈值”的上升沿时递增。
  uint64_t getIndicatorTriggerSequence() const noexcept {
    return indicatorTriggerSequence.load(std::memory_order_relaxed);
  }

  // 获取指示灯亮起持续时间
  float getIndicatorDuration() const noexcept {
    return flashDurationSeconds;
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
      indicatorTriggerSequence.fetch_add(1, std::memory_order_relaxed);

      // 记录本次触发与上次触发的间隔（从第二次触发开始才有意义）
      if (hasLastTrigger) {
        const auto intervalSec = levelDetectionElapsedSeconds;
        // 仅接受合理区间，避免暂停/恢复或采样率异常导致的巨大间隔污染
        if (intervalSec > 0.0f) {
          recentTriggerIntervalsSec[recentTriggerIntervalWriteIndex] = intervalSec;
          recentTriggerIntervalWriteIndex = (recentTriggerIntervalWriteIndex + 1) % recentTriggerIntervalsSec.size();
          recentTriggerIntervalCount = juce::jmin<int>(recentTriggerIntervalCount + 1,
                                                      static_cast<int>(recentTriggerIntervalsSec.size()));
        }
      }

      // 本次触发后重置计时基准
      levelDetectionElapsedSeconds = 0.0f;
      hasLastTrigger = true;

      // 计算动态闪烁持续时间：首次触发用默认值；之后用最近三次间隔平均值*0.8
      flashDurationSeconds = tremolo::defaults::indicatorFlashDurationSecDefault;
      if (recentTriggerIntervalCount >= 1) {
        float sum = 0.0f;
        for (int i = 0; i < recentTriggerIntervalCount; ++i) {
          sum += recentTriggerIntervalsSec[static_cast<size_t>(i)];
        }
        const auto avgIntervalSec = sum / static_cast<float>(recentTriggerIntervalCount);
        flashDurationSeconds = avgIntervalSec * tremolo::defaults::indicatorFlashDurationScale;
      }
      flashDurationSeconds = juce::jlimit(tremolo::defaults::indicatorFlashDurationSecMin,
                                          tremolo::defaults::indicatorFlashDurationSecMax,
                                          flashDurationSeconds);

      isFlashing = true;
      flashTimer = flashDurationSeconds;
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
  float sawWet = 0.0f;        // 公式映射湿度（0=全干，1=全湿）

  // 信号检测相关参数
  float peakLevel = 0.0f;            // 峰值电平
  bool isFlashing = false;           // 指示灯闪烁状态
  float flashTimer = 0.0f;           // 闪烁计时器
  float thresholdDB = tremolo::defaults::triggerThresholdDb;         // 触发阈值（dB）
  float flashDurationSeconds = tremolo::defaults::indicatorFlashDurationSecDefault;  // 动态闪烁时长（秒）
  bool wasAboveThreshold = false;    // 上次是否超过阈值

  // 动态闪烁时长计算：记录最近N次“阈值上升沿”的间隔
  float levelDetectionElapsedSeconds = 0.0f;
  bool hasLastTrigger = false;
  std::array<float, 3> recentTriggerIntervalsSec {0.0f, 0.0f, 0.0f};
  size_t recentTriggerIntervalWriteIndex = 0;
  int recentTriggerIntervalCount = 0;

  // 指示灯触发序列号（每次阈值上升沿递增）
  std::atomic<uint64_t> indicatorTriggerSequence{0};
 };

}  // namespace tremolo