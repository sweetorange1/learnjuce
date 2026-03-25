#pragma once

#include <array>

namespace tremolo::defaults {

// 参数默认值（这些值用于：参数创建默认值、UI初始值、以及序列化fallback）

inline constexpr float modulationRateHz = 5.0f;
inline constexpr int waveformIndex = 0; // 0=Sine

inline constexpr float xyX = 1.0f;
inline constexpr float xyY = 1.0f;

// XY 目标点：距离该点越近，效果越强（用于DSP映射与UI十字标记）
inline constexpr float xyTargetX = 0.5f;
inline constexpr float xyTargetY = 0.38f;

// XY 控制器布局（像素）：固定尺寸 + 左上角偏移
inline constexpr int xyControllerWidthPx = 550;
inline constexpr int xyControllerHeightPx = 550;
inline constexpr int xyControllerLeftPx = 19;
inline constexpr int xyControllerTopPx = 40;

// XY 控制点（tone.png）绘制尺寸（像素）：以图片中心点作为坐标
inline constexpr int xyToneMarkerWidthPx = 59;
inline constexpr int xyToneMarkerHeightPx = 75;

// 公式映射（锯齿化）默认参数：x' = asinh(x*A*10)/pi + (round((-x/2)*(B*20+1))-(-x/2)*(B*20+1))*C
inline constexpr float sawMapA = 1.0f;
inline constexpr float sawMapB = 1.0f;
inline constexpr float sawMapC = 0.2f;

// XY 到“公式映射湿度”的距离阈值：距离>=阈值为纯干声；距离=0为纯湿声
inline constexpr float sawWetDistanceThreshold = 0.25f;

// XY 到“增益提升”的距离阈值：距离>=阈值则不进行任何增益（gainBoost=1.0）
// 提示：0.70710678 约等于 sqrt(0.5)，可覆盖大部分区域；你可以根据手感调小让“增益甜区”更集中。
inline constexpr float gainBoostDistanceThreshold = 0.5;

// 最大增益提升倍率（原 MAX GAIN 滑条已移除，改为固定配置）
inline constexpr float maxGain = 6.0f;
// 兼容旧命名（过去该值通过"gain"参数暴露给宿主）
inline constexpr float gain = maxGain;

// 调试显示：是否在界面边缘显示当前“增益倍率”（用于测试映射）
inline constexpr bool showDebugGainOverlay = false;

// BPM触发模式：默认触发频率（音符时值索引）
// 0=一分音符, 1=二分音符, 2=四分音符, 3=八分音符, 4=16分音符, 5=32分音符, 6=64分音符
inline constexpr int bpmDivisionIndexDefault = 2;
inline constexpr int bpmDivisionCount = 7;

// 设置按钮布局（像素）：固定尺寸 + 左上角偏移
inline constexpr int settingsButtonWidthPx = 27;
inline constexpr int settingsButtonHeightPx = 27;
inline constexpr int settingsButtonLeftPx = 10;
inline constexpr int settingsButtonTopPx = 7;

// skins按钮布局（像素）：位于设置按钮右侧
inline constexpr int skinsButtonWidthPx = 60;
inline constexpr int skinsButtonHeightPx = settingsButtonHeightPx;
inline constexpr int skinsButtonLeftPx = settingsButtonLeftPx + settingsButtonWidthPx * 2 + 8 * 2;
inline constexpr int skinsButtonTopPx = settingsButtonTopPx;

// about按钮布局（像素）：位于skins按钮右侧
inline constexpr int aboutButtonWidthPx = settingsButtonWidthPx;
inline constexpr int aboutButtonHeightPx = settingsButtonHeightPx;
inline constexpr int aboutButtonLeftPx = settingsButtonLeftPx + settingsButtonWidthPx + 8;
inline constexpr int aboutButtonTopPx = settingsButtonTopPx;

// hide按钮布局（像素）：右下角（通过右/下边距定位）
inline constexpr int hideButtonWidthPx = 54;
inline constexpr int hideButtonHeightPx = 15;
inline constexpr int hideButtonMarginRightPx = 15;
inline constexpr int hideButtonMarginBottomPx = 4;

// 缩放倍率按钮布局（像素）：左下角（通过左/下边距定位）
inline constexpr int scaleButtonWidthPx = 54;
inline constexpr int scaleButtonHeightPx = 15;
inline constexpr int scaleButtonMarginLeftPx = 15;
inline constexpr int scaleButtonMarginBottomPx = 4;

// HCR/GGGG皮肤：指示灯闪烁时的逐帧动画每帧时长（秒）
// HCR/GGGG皮肤：指示灯闪烁时的逐帧动画每帧时长（秒）
inline constexpr double hcrIndicatorFrameDurationSec = 1.0 / 60.0;
inline constexpr double ggggIndicatorFrameDurationSec = 1.0 / 60.0;
inline constexpr double wbIndicatorFrameDurationSec = 1.0 / 60.0;
inline constexpr double dsIndicatorFrameDurationSec = 1.0 / 60.0;
inline constexpr double zszIndicatorFrameDurationSec = 1.0 / 30.0;
inline constexpr double ybIndicatorFrameDurationSec = 1.0 / 60.0;
inline constexpr double danIndicatorFrameDurationSec = 1.0 / 60.0;
inline constexpr double gzyIndicatorFrameDurationSec = 1.0 / 30.0;
inline constexpr double kkIndicatorFrameDurationSec = 1.0 / 60.0;

// “分段播放”类型皮肤：每次触发播放的帧区间配置（含起止帧，inclusive）
// 说明：
// - 每次触发按顺序取一个区间播放；用完最后一个区间后从头循环。
// - fromFrame/toFrame 允许正向或反向（fromFrame > toFrame 表示反向播放）。
struct SkinTriggerFrameSpan {
  int fromFrame = 0;
  int toFrame = 0;
};

// WB：每次触发播放一段（例如：[(1,32)(32,63)(63,32)(32,1)]）
// 注意：这里使用0-based帧索引（与sprite sheet裁剪索引一致）。
inline constexpr std::array<SkinTriggerFrameSpan, 4> wbTriggerFrameProgram = {
    SkinTriggerFrameSpan{0, 31},
    SkinTriggerFrameSpan{31, 62},
    SkinTriggerFrameSpan{62, 31},
    SkinTriggerFrameSpan{31, 0},
};

// HCR：一张sprite sheet（22帧），每次触发仅向前播放一段（0->21）
inline constexpr std::array<SkinTriggerFrameSpan, 1> hcrTriggerFrameProgram = {
    SkinTriggerFrameSpan{0, 21},
};

// GGGG：一张sprite sheet（25帧），每次触发播放一段（按配置循环取段）
inline constexpr std::array<SkinTriggerFrameSpan, 2> ggggTriggerFrameProgram = {
    SkinTriggerFrameSpan{0, 13},
    SkinTriggerFrameSpan{13, 24},
};

// DS：一张sprite sheet（57帧），每次触发播放一段（往复）
// 分段：前28帧一段、后29帧一段，往复播放 => [0->27, 27->56, 56->27, 27->0]
inline constexpr std::array<SkinTriggerFrameSpan, 4> dsTriggerFrameProgram = {
    SkinTriggerFrameSpan{0, 27},
    SkinTriggerFrameSpan{27, 56},
    SkinTriggerFrameSpan{56, 27},
    SkinTriggerFrameSpan{27, 0},
};

// ZSZ：一张sprite sheet（6帧），每次触发从头到尾播放一次（0->5）
inline constexpr std::array<SkinTriggerFrameSpan, 1> zszTriggerFrameProgram = {
    SkinTriggerFrameSpan{0, 5},
};

// YB：一张sprite sheet（54帧，0..53），每次触发播放一段（往复）
// 分段：0->26, 26->53, 53->26, 26->0
inline constexpr std::array<SkinTriggerFrameSpan, 4> ybTriggerFrameProgram = {
    SkinTriggerFrameSpan{0, 26},
    SkinTriggerFrameSpan{26, 53},
    SkinTriggerFrameSpan{53, 26},
    SkinTriggerFrameSpan{26, 0},
};

// DAN：一张sprite sheet（77帧，0..76），每次触发播放三段，然后按相反方向播放三段（往复）
// 你给的区间是 1-25 / 25-50 / 50-77，这里换算成 0-based 帧索引：0-24 / 24-49 / 49-76
inline constexpr std::array<SkinTriggerFrameSpan, 6> danTriggerFrameProgram = {
    SkinTriggerFrameSpan{0, 24},
    SkinTriggerFrameSpan{24, 49},
    SkinTriggerFrameSpan{49, 76},
    SkinTriggerFrameSpan{76, 49},
    SkinTriggerFrameSpan{49, 24},
    SkinTriggerFrameSpan{24, 0},
};

// GZY：一张sprite sheet（232帧，0..231），4行×58列，每帧550×550。
// 每次触发单向向前推进：共16段，从0推进到最后一帧。
// - "走15帧"表示 to-from = 15（包含起止帧则显示16帧）
// - "走14帧"表示 to-from = 14（包含起止帧则显示15帧）
// 这里采用：前7段走15帧（15步），后9段走14帧（14步），刚好到231。
inline constexpr std::array<SkinTriggerFrameSpan, 16> gzyTriggerFrameProgram = {
    SkinTriggerFrameSpan{0, 16},
    SkinTriggerFrameSpan{16, 31},
    SkinTriggerFrameSpan{31, 46},
    SkinTriggerFrameSpan{46, 58},
    SkinTriggerFrameSpan{58, 69},
    SkinTriggerFrameSpan{69, 84 },
    SkinTriggerFrameSpan{84, 99},
    SkinTriggerFrameSpan{99, 114},
    SkinTriggerFrameSpan{114 , 128},
    SkinTriggerFrameSpan{128 , 142},
    SkinTriggerFrameSpan{142, 157},
    SkinTriggerFrameSpan{157, 174},
    SkinTriggerFrameSpan{174 ,  185},
    SkinTriggerFrameSpan{185, 200},
    SkinTriggerFrameSpan{200 , 214},
    SkinTriggerFrameSpan{214 , 231},
};

// KK：一张sprite sheet（帧数待定），每次触发播放一段（往复）
inline constexpr std::array<SkinTriggerFrameSpan, 4> kkTriggerFrameProgram = {
    SkinTriggerFrameSpan{0, 15},
    SkinTriggerFrameSpan{15, 30},
    SkinTriggerFrameSpan{30, 15},
    SkinTriggerFrameSpan{15, 0},
};

// 兼容旧配置（过去用fps表示）：建议新代码改用 *FrameDurationSec
inline constexpr float hcrIndicatorAnimFps = static_cast<float>(1.0 / hcrIndicatorFrameDurationSec);

// XY皮肤配置：默认皮肤与切换顺序（方便后续扩展多套皮肤）
// 约定：BT=第一套（assets/BT），HCR=第二套（assets/HCR）
enum class XYSkinId : int {
  BT = 0,
  HCR = 1,
  GGGG = 2,
  WB = 3,
  DS = 4,
  ZSZ = 5,
  YB = 6,
  DAN = 7,
  GZY = 8,
  KK = 9,
};

// 默认皮肤（启动时使用）
inline constexpr XYSkinId xyDefaultSkin = XYSkinId::HCR;

// 皮肤切换顺序（按顺序循环）。新增皮肤时把新枚举追加到这里即可。
inline constexpr std::array<XYSkinId, 9> xySkinCycleOrder = {
    XYSkinId::HCR,
    XYSkinId::GGGG,
    XYSkinId::WB,
    XYSkinId::DS,
    XYSkinId::ZSZ,
    XYSkinId::YB,
    XYSkinId::DAN,
    XYSkinId::GZY,
    XYSkinId::KK,
};

// 指示灯布局（像素）：固定尺寸 + 左上角偏移
inline constexpr int indicatorLightWidthPx = 20;
inline constexpr int indicatorLightHeightPx = 20;
inline constexpr int indicatorLightLeftPx = 552;
inline constexpr int indicatorLightTopPx = 10;

// 电平捕捉窗口（毫秒）
inline constexpr float levelCaptureWindowMs = 60.0f;
inline constexpr float levelCaptureWindowMsMin = 5.0f;
inline constexpr float levelCaptureWindowMsMax = 200.0f;
inline constexpr float levelCaptureWindowMsStep = 1.0f;
inline constexpr float levelCaptureWindowMsSkew = 0.4f;
inline constexpr float levelCaptureWindowMsSkewMid = 60.0f;

// 输入检测滤波器默认值（Hz）
inline constexpr float inputHighpassHz = 20.0f;
inline constexpr float inputLowpassHz = 20000.0f;

// 电平检测阈值默认值（dB）
inline constexpr float triggerThresholdDb = -15.0f;

// 指示灯闪烁时长（秒）：首次默认值与动态时长限制
inline constexpr float indicatorFlashDurationSecDefault = 0.4f;
inline constexpr float indicatorFlashDurationSecMin = 0.05f;
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