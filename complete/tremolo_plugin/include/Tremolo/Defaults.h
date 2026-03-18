#pragma once

namespace tremolo::defaults {

// 参数默认值（这些值用于：参数创建默认值、UI初始值、以及序列化fallback）

inline constexpr float modulationRateHz = 5.0f;
inline constexpr bool bypassed = false;
inline constexpr int waveformIndex = 0; // 0=Sine

inline constexpr float xyX = 0.5f;
inline constexpr float xyY = 0.3f;

// XY 控制器布局（像素）：固定尺寸 + 左上角偏移
inline constexpr int xyControllerWidthPx = 550;
inline constexpr int xyControllerHeightPx = 550;
inline constexpr int xyControllerLeftPx = 20;
inline constexpr int xyControllerTopPx = 40;

// 设置按钮布局（像素）：固定尺寸 + 左上角偏移
inline constexpr int settingsButtonWidthPx = 27;
inline constexpr int settingsButtonHeightPx = 27;
inline constexpr int settingsButtonLeftPx = 10;
inline constexpr int settingsButtonTopPx = 7;

// 指示灯布局（像素）：固定尺寸 + 左上角偏移
inline constexpr int indicatorLightWidthPx = 20;
inline constexpr int indicatorLightHeightPx = 20;
inline constexpr int indicatorLightLeftPx = 552;
inline constexpr int indicatorLightTopPx = 10;

inline constexpr float gain = 6.0f;

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
inline constexpr float indicatorFlashDurationSecMin = 0.1f;
inline constexpr float indicatorFlashDurationSecMax = 1.0f;

// 动态时长缩放：最近若干次触发间隔平均值 * scale
inline constexpr float indicatorFlashDurationScale = 0.8f;

// jj.png 动画配置（像素）：起始位置与最高点位置（用于调整上下往复动画幅度）
// 说明：这里使用“向上偏移量”，数值越大，图片越往上移动。
inline constexpr float jjAnimationStartYOffsetPx = -160.0f;
inline constexpr float jjAnimationPeakYOffsetPx = 35.0f;

// jj.png 在插件界面中的缩放比：1.0=原始尺寸；>1 放大；<1 缩小
inline constexpr float jjImageScale = 5.5f;

}  // namespace tremolo::defaults