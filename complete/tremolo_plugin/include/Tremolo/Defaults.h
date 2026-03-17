#pragma once

namespace tremolo::defaults {

// 参数默认值（这些值用于：参数创建默认值、UI初始值、以及序列化fallback）

inline constexpr float modulationRateHz = 5.0f;
inline constexpr bool bypassed = false;
inline constexpr int waveformIndex = 0; // 0=Sine

inline constexpr float xyX = 0.5f;
inline constexpr float xyY = 0.3f;

inline constexpr float gain = 4.0f;

// 电平捕捉窗口（毫秒）
inline constexpr float levelCaptureWindowMs = 60.0f;
inline constexpr float levelCaptureWindowMsMin = 5.0f;
inline constexpr float levelCaptureWindowMsMax = 200.0f;
inline constexpr float levelCaptureWindowMsStep = 1.0f;
inline constexpr float levelCaptureWindowMsSkew = 0.4f;
inline constexpr float levelCaptureWindowMsSkewMid = 60.0f;

// 输入检测滤波器默认值（Hz）
inline constexpr float inputHighpassHz = 1300.0f;
inline constexpr float inputLowpassHz = 20000.0f;

// 电平检测阈值默认值（dB）
inline constexpr float triggerThresholdDb = -15.0f;

// 指示灯闪烁时长（秒）：首次默认值与动态时长限制
inline constexpr float indicatorFlashDurationSecDefault = 0.4f;
inline constexpr float indicatorFlashDurationSecMin = 0.2f;
inline constexpr float indicatorFlashDurationSecMax = 1.0f;

// 动态时长缩放：最近若干次触发间隔平均值 * scale
inline constexpr float indicatorFlashDurationScale = 0.8f;

}  // namespace tremolo::defaults
