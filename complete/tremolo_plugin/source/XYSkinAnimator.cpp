#include "../include/Tremolo/XYSkinAnimator.h"

#include <TremoloPluginAssets.h>
#include <TremoloPluginAssetsGGGG.h>
#include <TremoloPluginAssetsWB.h>
#include <TremoloPluginAssetsDS.h>
#include <TremoloPluginAssetsZSZ.h>
#include <TremoloPluginAssetsYB.h>
#include <TremoloPluginAssetsDAN.h>
#include <TremoloPluginAssetsGZY.h>
#include <TremoloPluginAssetsKK.h>

#include <array>
#include <limits>

namespace tremolo {

namespace {
struct BinaryImage {
  const char* data{};
  int size{};
};

inline juce::Image loadImageFromBinary(const BinaryImage& img) {
  return juce::ImageCache::getFromMemory(img.data, img.size);
}

// HCR：001为底图；HCR_pnglist.png为指示灯闪烁时的sprite sheet（22帧）
const BinaryImage kHcrBase{assets::_001_png, assets::_001_pngSize};
const BinaryImage kHcrSpriteSheet{assets::HCR_pnglist_png, assets::HCR_pnglist_pngSize};
const BinaryImage kHcrTone{assets::tone_png, assets::tone_pngSize};

// GGGG：GGGG_pnglist.png为底图/指示灯共用的sprite sheet（25帧）
const BinaryImage kGgggSpriteSheet{assets_gggg::GGGG_pnglist_png, assets_gggg::GGGG_pnglist_pngSize};
const BinaryImage kGgggTone{assets_gggg::tone_png, assets_gggg::tone_pngSize};

// WB：一张sprite sheet（1行×64列，每帧550×550），指示灯闪烁时按规则切换显示帧
const BinaryImage kWbSpriteSheet{assets_wb::WB_pnglist_png, assets_wb::WB_pnglist_pngSize};
const BinaryImage kWbTone{assets_wb::tone_png, assets_wb::tone_pngSize};

// DS：DS_pnglist.png为底图/指示灯共用的sprite sheet（57帧）
const BinaryImage kDsSpriteSheet{assets_ds::DS_pnglist_png, assets_ds::DS_pnglist_pngSize};
const BinaryImage kDsTone{assets_ds::tone_png, assets_ds::tone_pngSize};

// ZSZ：ZSZ_pnglist.png为底图/指示灯共用的sprite sheet（6帧）
const BinaryImage kZszSpriteSheet{assets_zsz::ZSZ_pnglist_png, assets_zsz::ZSZ_pnglist_pngSize};
const BinaryImage kZszTone{assets_zsz::tone_png, assets_zsz::tone_pngSize};

// YB：YB_pnglist.png为底图/指示灯共用的sprite sheet（54帧）
const BinaryImage kYbSpriteSheet{assets_yb::YB_pnglist_png, assets_yb::YB_pnglist_pngSize};
const BinaryImage kYbTone{assets_yb::tone_png, assets_yb::tone_pngSize};

// DAN：DAN_pnglist.png为底图/指示灯共用的sprite sheet（77帧）
const BinaryImage kDanSpriteSheet{assets_dan::DAN_pnglist_png, assets_dan::DAN_pnglist_pngSize};
const BinaryImage kDanTone{assets_dan::tone_png, assets_dan::tone_pngSize};

// GZY：GZY_pnglist.png为底图/指示灯共用的sprite sheet（232帧，4行×58列）
const BinaryImage kGzySpriteSheet{assets_gzy::GZY_pnglist_png, assets_gzy::GZY_pnglist_pngSize};
const BinaryImage kGzyTone{assets_gzy::tone_png, assets_gzy::tone_pngSize};

// KK：KK_pnglist.png为底图/指示灯共用的sprite sheet（31帧）
const BinaryImage kkSpriteSheet{assets_kk::KK_pnglist_png, assets_kk::KK_pnglist_pngSize};
const BinaryImage kkTone{assets_kk::tone_png, assets_kk::tone_pngSize};

constexpr int kWbFrameWidthPx = 550;
constexpr int kWbFrameHeightPx = 550;
constexpr int kWbFrameCount = 64;

constexpr int kDsFrameWidthPx = 550;
constexpr int kDsFrameHeightPx = 550;
constexpr int kDsFrameCount = 57;

constexpr int kZszFrameWidthPx = 550;
constexpr int kZszFrameHeightPx = 550;
constexpr int kZszFrameCount = 6;

constexpr int kYbFrameWidthPx = 550;
constexpr int kYbFrameHeightPx = 550;
constexpr int kYbFrameCount = 54;

constexpr int kDanFrameWidthPx = 550;
constexpr int kDanFrameHeightPx = 550;
constexpr int kDanFrameCount = 77;

constexpr int kGzyFrameWidthPx = 550;
constexpr int kGzyFrameHeightPx = 550;
constexpr int kGzyFrameCount = 232;
constexpr int kGzyFramesPerRow = 58;

constexpr int kkFrameWidthPx = 550;
constexpr int kkFrameHeightPx = 550;
constexpr int kkFrameCount = 31;

}  // namespace

XYSkinAnimator::~XYSkinAnimator() {
  shutdown();
}

void XYSkinAnimator::shutdown() {
  wbSpriteSheetLoadCancel.store(true);
  if (wbSpriteSheetLoadThread.joinable()) {
    wbSpriteSheetLoadThread.join();
  }
  wbSpriteSheetLoading.store(false);
  wbSpriteSheetReady.store(false);

  ybSpriteSheetLoadCancel.store(true);
  if (ybSpriteSheetLoadThread.joinable()) {
    ybSpriteSheetLoadThread.join();
  }
  ybSpriteSheetLoading.store(false);
  ybSpriteSheetReady.store(false);

  ggggSpriteSheetLoadCancel.store(true);
  if (ggggSpriteSheetLoadThread.joinable()) {
    ggggSpriteSheetLoadThread.join();
  }
  ggggSpriteSheetLoading.store(false);
  ggggSpriteSheetReady.store(false);

  dsSpriteSheetLoadCancel.store(true);
  if (dsSpriteSheetLoadThread.joinable()) {
    dsSpriteSheetLoadThread.join();
  }
  dsSpriteSheetLoading.store(false);
  dsSpriteSheetReady.store(false);

  zszSpriteSheetLoadCancel.store(true);
  if (zszSpriteSheetLoadThread.joinable()) {
    zszSpriteSheetLoadThread.join();
  }
  zszSpriteSheetLoading.store(false);
  zszSpriteSheetReady.store(false);

  danSpriteSheetLoadCancel.store(true);
  if (danSpriteSheetLoadThread.joinable()) {
    danSpriteSheetLoadThread.join();
  }
  danSpriteSheetLoading.store(false);
  danSpriteSheetReady.store(false);

  gzySpriteSheetLoadCancel.store(true);
  if (gzySpriteSheetLoadThread.joinable()) {
    gzySpriteSheetLoadThread.join();
  }
  gzySpriteSheetLoading.store(false);
  gzySpriteSheetReady.store(false);

  kkSpriteSheetLoadCancel.store(true);
  if (kkSpriteSheetLoadThread.joinable()) {
    kkSpriteSheetLoadThread.join();
  }
  kkSpriteSheetLoading.store(false);
  kkSpriteSheetReady.store(false);
}

void XYSkinAnimator::attach(View view) {
  view_ = view;
}

void XYSkinAnimator::setSkin(tremolo::defaults::XYSkinId newSkin) {
  // BT 皮肤资源已不再打包；这里做兼容：遇到 BT 直接降级到 HCR。
  if (newSkin == tremolo::defaults::XYSkinId::BT) {
    newSkin = tremolo::defaults::XYSkinId::HCR;
  }

  // 切换皮肤时，停止所有皮肤相关动画，避免状态串台
  stopHcrFrameAnimation();
  stopGgggFrameAnimation();
  stopDsFrameAnimation();
  stopZszFrameAnimation();
  stopWbFrameAnimation();
  stopYbFrameAnimation();
  // DAN
  stopDanFrameAnimation();
  // GZY
  stopGzyFrameAnimation();
  // KK
  stopKkFrameAnimation();

  // 皮肤加载提示：默认隐藏；异步加载时显示
  skinLoadingOverlayVisible = false;

  // 如果正在异步加载资源，而这次切走对应皮肤，则取消加载
  if (newSkin != tremolo::defaults::XYSkinId::WB) {
    wbSpriteSheetLoadCancel.store(true);
  }
  if (newSkin != tremolo::defaults::XYSkinId::YB) {
    ybSpriteSheetLoadCancel.store(true);
  }
  if (newSkin != tremolo::defaults::XYSkinId::GGGG) {
    ggggSpriteSheetLoadCancel.store(true);
  }
  if (newSkin != tremolo::defaults::XYSkinId::DS) {
    dsSpriteSheetLoadCancel.store(true);
  }
  if (newSkin != tremolo::defaults::XYSkinId::ZSZ) {
    zszSpriteSheetLoadCancel.store(true);
  }
  if (newSkin != tremolo::defaults::XYSkinId::DAN) {
    danSpriteSheetLoadCancel.store(true);
  }
  if (newSkin != tremolo::defaults::XYSkinId::GZY) {
    gzySpriteSheetLoadCancel.store(true);
  }
  if (newSkin != tremolo::defaults::XYSkinId::KK) {
    kkSpriteSheetLoadCancel.store(true);
  }

  // 统一恢复到“未闪烁”状态的基础底图
  if (newSkin == tremolo::defaults::XYSkinId::HCR) {
    const auto base = loadImageFromBinary(kHcrBase);
    if (base.isValid() && view_.btImage) {
      view_.btImage->setImage(base);
    }

    // HCR皮肤不使用jj上下往复动画
    if (view_.jjImage) {
      view_.jjImage->setVisible(false);
    }
    if (view_.jjClipper) {
      view_.jjClipper->setVisible(false);
    }

    const auto tone = loadImageFromBinary(kHcrTone);
    if (tone.isValid() && view_.toneImage) {
      view_.toneImage->setImage(tone);
    }
  } else if (newSkin == tremolo::defaults::XYSkinId::GGGG) {
    // GGGG：异步加载sprite sheet；资源到位后用第0帧作为底图
    ggggTriggerProgramIndex = 0;
    stopGgggFrameAnimation();

    beginLoadGgggSpriteSheetAsync();
    if (ggggSpriteSheet.isValid()) {
      setGgggFrameIndex(0);
    }

    if (view_.jjImage) {
      view_.jjImage->setVisible(false);
    }
    if (view_.jjClipper) {
      view_.jjClipper->setVisible(false);
    }

    const auto tone = loadImageFromBinary(kGgggTone);
    if (tone.isValid() && view_.toneImage) {
      view_.toneImage->setImage(tone);
    }
  } else if (newSkin == tremolo::defaults::XYSkinId::DS) {
    dsTriggerProgramIndex = 0;
    stopDsFrameAnimation();

    beginLoadDsSpriteSheetAsync();
    if (dsSpriteSheet.isValid()) {
      setDsFrameIndex(0);
    }

    if (view_.jjImage) {
      view_.jjImage->setVisible(false);
    }
    if (view_.jjClipper) {
      view_.jjClipper->setVisible(false);
    }

    const auto tone = loadImageFromBinary(kDsTone);
    if (tone.isValid() && view_.toneImage) {
      view_.toneImage->setImage(tone);
    }
  } else if (newSkin == tremolo::defaults::XYSkinId::ZSZ) {
    zszTriggerProgramIndex = 0;
    stopZszFrameAnimation();

    beginLoadZszSpriteSheetAsync();
    if (zszSpriteSheet.isValid()) {
      setZszFrameIndex(0);
    }

    if (view_.jjImage) {
      view_.jjImage->setVisible(false);
    }
    if (view_.jjClipper) {
      view_.jjClipper->setVisible(false);
    }

    const auto tone = loadImageFromBinary(kZszTone);
    if (tone.isValid() && view_.toneImage) {
      view_.toneImage->setImage(tone);
    }
  } else if (newSkin == tremolo::defaults::XYSkinId::YB) {
    ybTriggerProgramIndex = 0;

    beginLoadYbSpriteSheetAsync();
    if (ybSpriteSheet.isValid()) {
      setYbFrameIndex(0);
    }

    if (view_.jjImage) {
      view_.jjImage->setVisible(false);
    }
    if (view_.jjClipper) {
      view_.jjClipper->setVisible(false);
    }

    const auto tone = loadImageFromBinary(kYbTone);
    if (tone.isValid() && view_.toneImage) {
      view_.toneImage->setImage(tone);
    }
  } else if (newSkin == tremolo::defaults::XYSkinId::DAN) {
    danTriggerProgramIndex = 0;
    stopDanFrameAnimation();

    beginLoadDanSpriteSheetAsync();
    if (danSpriteSheet.isValid()) {
      setDanFrameIndex(0);
    }

    if (view_.jjImage) {
      view_.jjImage->setVisible(false);
    }
    if (view_.jjClipper) {
      view_.jjClipper->setVisible(false);
    }

    const auto tone = loadImageFromBinary(kDanTone);
    if (tone.isValid() && view_.toneImage) {
      view_.toneImage->setImage(tone);
    }
  } else if (newSkin == tremolo::defaults::XYSkinId::GZY) {
    gzyTriggerProgramIndex = 0;
    stopGzyFrameAnimation();

    beginLoadGzySpriteSheetAsync();
    if (gzySpriteSheet.isValid()) {
      setGzyFrameIndex(0);
    }

    if (view_.jjImage) {
      view_.jjImage->setVisible(false);
    }
    if (view_.jjClipper) {
      view_.jjClipper->setVisible(false);
    }

    const auto tone = loadImageFromBinary(kGzyTone);
    if (tone.isValid() && view_.toneImage) {
      view_.toneImage->setImage(tone);
    }
  } else if (newSkin == tremolo::defaults::XYSkinId::KK) {
    kkTriggerProgramIndex = 0;
    stopKkFrameAnimation();

    beginLoadKkSpriteSheetAsync();
    if (kkSpriteSheet.isValid()) {
      setKkFrameIndex(0);
    }

    if (view_.jjImage) {
      view_.jjImage->setVisible(false);
    }
    if (view_.jjClipper) {
      view_.jjClipper->setVisible(false);
    }

    const auto tone = loadImageFromBinary(kkTone);
    if (tone.isValid() && view_.toneImage) {
      view_.toneImage->setImage(tone);
    }
  } else {
    // WB
    wbTriggerProgramIndex = 0;

    beginLoadWbSpriteSheetAsync();
    if (wbSpriteSheet.isValid()) {
      setWbFrameIndex(0);
    }

    if (view_.jjImage) {
      view_.jjImage->setVisible(false);
    }
    if (view_.jjClipper) {
      view_.jjClipper->setVisible(false);
    }

    const auto tone = loadImageFromBinary(kWbTone);
    if (tone.isValid() && view_.toneImage) {
      view_.toneImage->setImage(tone);
    }
  }

  if (view_.owner) {
    view_.owner->repaint();
  }
}

void XYSkinAnimator::tick(tremolo::defaults::XYSkinId currentSkin,
                         bool shouldFlash,
                         bool retriggered,
                         double dtSec) {
  // BT 皮肤资源已不再打包；兼容旧配置：按 HCR 处理。
  if (currentSkin == tremolo::defaults::XYSkinId::BT) {
    currentSkin = tremolo::defaults::XYSkinId::HCR;
  }

  consumeWbSpriteSheetIfReady();
  consumeYbSpriteSheetIfReady();
  consumeGgggSpriteSheetIfReady();
  consumeDsSpriteSheetIfReady();
  consumeZszSpriteSheetIfReady();
  consumeDanSpriteSheetIfReady();
  consumeGzySpriteSheetIfReady();
  consumeKkSpriteSheetIfReady();

  const bool risingEdge = shouldFlash && (!wasIndicatorFlashing || retriggered);
  wasIndicatorFlashing = shouldFlash;

  if (currentSkin == tremolo::defaults::XYSkinId::HCR) {
    if (risingEdge) {
      startHcrFrameAnimation();
    }

    if (!shouldFlash) {
      const auto base = loadImageFromBinary(kHcrBase);
      if (base.isValid() && view_.btImage) {
        view_.btImage->setImage(base);
      }
      stopHcrFrameAnimation();
      return;
    }

    if (!hcrFrameAnimActive) {
      return;
    }

    advanceFrameAnimation(hcrFrameAnimActive,
                         hcrFrameIndex,
                         hcrFrameTimeAccSec,
                         hcrTriggerStep,
                         hcrTriggerEndFrame,
                         0,
                         21,
                         tremolo::defaults::hcrIndicatorFrameDurationSec,
                         dtSec,
                         [this](int i) { setHcrFrameIndex(i); },
                         [this]() { stopHcrFrameAnimation(); });
    return;
  }

  if (currentSkin == tremolo::defaults::XYSkinId::GGGG) {
    if (risingEdge) {
      startGgggFrameAnimation();
    }

    if (!ggggFrameAnimActive) {
      return;
    }

    advanceFrameAnimation(ggggFrameAnimActive,
                         ggggFrameIndex,
                         ggggFrameTimeAccSec,
                         ggggTriggerStep,
                         ggggTriggerEndFrame,
                         0,
                         24,
                         tremolo::defaults::ggggIndicatorFrameDurationSec,
                         dtSec,
                         [this](int i) { setGgggFrameIndex(i); },
                         [this]() { stopGgggFrameAnimation(); });
    return;
  }

  if (currentSkin == tremolo::defaults::XYSkinId::DS) {
    if (risingEdge) {
      startDsFrameAnimation();
    }

    if (!dsFrameAnimActive) {
      return;
    }

    advanceFrameAnimation(dsFrameAnimActive,
                         dsFrameIndex,
                         dsFrameTimeAccSec,
                         dsTriggerStep,
                         dsTriggerEndFrame,
                         0,
                         kDsFrameCount - 1,
                         tremolo::defaults::dsIndicatorFrameDurationSec,
                         dtSec,
                         [this](int i) { setDsFrameIndex(i); },
                         [this]() { stopDsFrameAnimation(); });
    return;
  }

  if (currentSkin == tremolo::defaults::XYSkinId::ZSZ) {
    if (risingEdge) {
      startZszFrameAnimation();
    }

    if (!zszFrameAnimActive) {
      return;
    }

    advanceFrameAnimation(zszFrameAnimActive,
                         zszFrameIndex,
                         zszFrameTimeAccSec,
                         zszTriggerStep,
                         zszTriggerEndFrame,
                         0,
                         kZszFrameCount - 1,
                         tremolo::defaults::zszIndicatorFrameDurationSec,
                         dtSec,
                         [this](int i) { setZszFrameIndex(i); },
                         [this]() { stopZszFrameAnimation(); });
    return;
  }

  if (currentSkin == tremolo::defaults::XYSkinId::WB) {
    if (risingEdge) {
      if (!wbSpriteSheet.isValid()) {
        beginLoadWbSpriteSheetAsync();
        return;
      }

      const auto& program = tremolo::defaults::wbTriggerFrameProgram;
      if (program.empty()) {
        return;
      }

      const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, wbTriggerProgramIndex);
      const auto seg = program[static_cast<size_t>(segIndex)];

      const int from = juce::jlimit(0, kWbFrameCount - 1, seg.fromFrame);
      const int to = juce::jlimit(0, kWbFrameCount - 1, seg.toFrame);

      wbTriggerStep = (from <= to) ? +1 : -1;
      wbTriggerEndFrame = to;

      setWbFrameIndex(from);

      wbFrameAnimActive = true;
      wbFrameTimeAccSec = 0.0;

      wbTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
    }

    if (!wbFrameAnimActive) {
      return;
    }

    advanceFrameAnimation(wbFrameAnimActive,
                         wbFrameIndex,
                         wbFrameTimeAccSec,
                         wbTriggerStep,
                         wbTriggerEndFrame,
                         0,
                         kWbFrameCount - 1,
                         tremolo::defaults::wbIndicatorFrameDurationSec,
                         dtSec,
                         [this](int i) { setWbFrameIndex(i); },
                         [this]() { stopWbFrameAnimation(); });
    return;
  }

  if (currentSkin == tremolo::defaults::XYSkinId::YB) {
    if (risingEdge) {
      if (!ybSpriteSheet.isValid()) {
        beginLoadYbSpriteSheetAsync();
        return;
      }
      startYbFrameAnimation();
    }

    if (!ybFrameAnimActive) {
      return;
    }

    advanceFrameAnimation(ybFrameAnimActive,
                         ybFrameIndex,
                         ybFrameTimeAccSec,
                         ybTriggerStep,
                         ybTriggerEndFrame,
                         0,
                         kYbFrameCount - 1,
                         tremolo::defaults::ybIndicatorFrameDurationSec,
                         dtSec,
                         [this](int i) { setYbFrameIndex(i); },
                         [this]() { stopYbFrameAnimation(); });
    return;
  }

  if (currentSkin == tremolo::defaults::XYSkinId::DAN) {
    if (risingEdge) {
      if (!danSpriteSheet.isValid()) {
        beginLoadDanSpriteSheetAsync();
        return;
      }
      startDanFrameAnimation();
    }

    if (!danFrameAnimActive) {
      return;
    }

    advanceFrameAnimation(danFrameAnimActive,
                         danFrameIndex,
                         danFrameTimeAccSec,
                         danTriggerStep,
                         danTriggerEndFrame,
                         0,
                         kDanFrameCount - 1,
                         tremolo::defaults::danIndicatorFrameDurationSec,
                         dtSec,
                         [this](int i) { setDanFrameIndex(i); },
                         [this]() { stopDanFrameAnimation(); });
    return;
  }

  if (currentSkin == tremolo::defaults::XYSkinId::GZY) {
    if (risingEdge) {
      if (!gzySpriteSheet.isValid()) {
        beginLoadGzySpriteSheetAsync();
        return;
      }
      startGzyFrameAnimation();
    }

    if (!gzyFrameAnimActive) {
      return;
    }

    advanceFrameAnimation(gzyFrameAnimActive,
                         gzyFrameIndex,
                         gzyFrameTimeAccSec,
                         gzyTriggerStep,
                         gzyTriggerEndFrame,
                         0,
                         kGzyFrameCount - 1,
                         tremolo::defaults::gzyIndicatorFrameDurationSec,
                         dtSec,
                         [this](int i) { setGzyFrameIndex(i); },
                         [this]() { stopGzyFrameAnimation(); });
    return;
  }

  if (currentSkin == tremolo::defaults::XYSkinId::KK) {
    if (risingEdge) {
      if (!kkSpriteSheet.isValid()) {
        beginLoadKkSpriteSheetAsync();
        return;
      }
      startKkFrameAnimation();
    }

    if (!kkFrameAnimActive) {
      return;
    }

    advanceFrameAnimation(kkFrameAnimActive,
                         kkFrameIndex,
                         kkFrameTimeAccSec,
                         kkTriggerStep,
                         kkTriggerEndFrame,
                         0,
                         kkFrameCount - 1,
                         tremolo::defaults::kkIndicatorFrameDurationSec,
                         dtSec,
                         [this](int i) { setKkFrameIndex(i); },
                         [this]() { stopKkFrameAnimation(); });
    return;
  }
}

void XYSkinAnimator::advanceFrameAnimation(bool& active,
                                          int& frameIndex,
                                          double& timeAccSec,
                                          int triggerStep,
                                          int triggerEndFrame,
                                          int frameMin,
                                          int frameMax,
                                          double frameDurationSec,
                                          double dtSec,
                                          const std::function<void(int)>& setFrameIndex,
                                          const std::function<void()>& stop) {
  if (!active) {
    return;
  }

  const double frameDuration = juce::jmax(1.0e-6, frameDurationSec);
  timeAccSec += dtSec;

  while (timeAccSec >= frameDuration && active) {
    timeAccSec -= frameDuration;

    if (frameIndex == triggerEndFrame) {
      stop();
      break;
    }

    const int prev = frameIndex;
    const int candidate = prev + triggerStep;
    const int next = juce::jlimit(frameMin, frameMax, candidate);

    setFrameIndex(next);

    if (next == prev || next == triggerEndFrame) {
      if (next == triggerEndFrame) {
        setFrameIndex(triggerEndFrame);
      }
      stop();
      break;
    }
  }
}

// ====== HCR ======
void XYSkinAnimator::startHcrFrameAnimation() {
  if (!hcrSpriteSheet.isValid()) {
    hcrSpriteSheet = loadImageFromBinary(kHcrSpriteSheet);
    if (!hcrSpriteSheet.isValid()) {
      return;
    }
  }

  const auto& program = tremolo::defaults::hcrTriggerFrameProgram;
  const auto seg = program[0];

  const int from = seg.fromFrame;
  const int to = seg.toFrame;

  hcrTriggerStep = (from <= to) ? +1 : -1;
  hcrTriggerEndFrame = to;

  hcrFrameAnimActive = true;
  hcrFrameIndex = from;
  hcrFrameTimeAccSec = 0.0;

  setHcrFrameIndex(from);
}

void XYSkinAnimator::stopHcrFrameAnimation() {
  hcrFrameAnimActive = false;
  hcrFrameTimeAccSec = 0.0;
  hcrTriggerStep = +1;
  hcrTriggerEndFrame = hcrFrameIndex;
}

void XYSkinAnimator::setHcrFrameIndex(int newIndex) {
  if (!hcrSpriteSheet.isValid() || !view_.btImage) {
    return;
  }

  constexpr int kHcrFrameWidthPx = 550;
  constexpr int kHcrFrameHeightPx = 550;
  constexpr int kHcrFrameCount = 22;

  hcrFrameIndex = juce::jlimit(0, kHcrFrameCount - 1, newIndex);

  const int x = hcrFrameIndex * kHcrFrameWidthPx;
  const auto clipped = hcrSpriteSheet.getClippedImage(
      juce::Rectangle<int>{x, 0, kHcrFrameWidthPx, kHcrFrameHeightPx});

  if (clipped.isValid()) {
    view_.btImage->setImage(clipped);
  }
}

// ====== GGGG ======
void XYSkinAnimator::startGgggFrameAnimation() {
  if (!ggggSpriteSheet.isValid()) {
    beginLoadGgggSpriteSheetAsync();
    return;
  }

  const auto& program = tremolo::defaults::ggggTriggerFrameProgram;
  if (program.empty()) {
    return;
  }

  constexpr int kGgggFrameCount = 25;

  const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, ggggTriggerProgramIndex);
  const auto seg = program[static_cast<size_t>(segIndex)];

  const int from = juce::jlimit(0, kGgggFrameCount - 1, seg.fromFrame);
  const int to = juce::jlimit(0, kGgggFrameCount - 1, seg.toFrame);

  ggggTriggerStep = (from <= to) ? +1 : -1;
  ggggTriggerEndFrame = to;

  ggggFrameAnimActive = true;
  ggggFrameIndex = from;
  ggggFrameTimeAccSec = 0.0;

  setGgggFrameIndex(from);

  ggggTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
}

void XYSkinAnimator::stopGgggFrameAnimation() {
  ggggFrameAnimActive = false;
  ggggFrameTimeAccSec = 0.0;
  ggggTriggerStep = +1;
  ggggTriggerEndFrame = ggggFrameIndex;
}

void XYSkinAnimator::setGgggFrameIndex(int newIndex) {
  if (!ggggSpriteSheet.isValid() || !view_.btImage) {
    return;
  }

  constexpr int kGgggFrameWidthPx = 550;
  constexpr int kGgggFrameHeightPx = 550;
  constexpr int kGgggFrameCount = 25;

  ggggFrameIndex = juce::jlimit(0, kGgggFrameCount - 1, newIndex);

  const int x = ggggFrameIndex * kGgggFrameWidthPx;
  const auto clipped = ggggSpriteSheet.getClippedImage(
      juce::Rectangle<int>{x, 0, kGgggFrameWidthPx, kGgggFrameHeightPx});

  if (clipped.isValid()) {
    view_.btImage->setImage(clipped);
  }
}

// ====== GGGG async loading ======
void XYSkinAnimator::beginLoadGgggSpriteSheetAsync() {
  if (ggggSpriteSheet.isValid()) {
    skinLoadingOverlayVisible = false;
    return;
  }

  if (ggggSpriteSheetLoading.load()) {
    skinLoadingOverlayVisible = true;
    skinLoadingOverlayText = "loading";
    if (view_.owner) {
      view_.owner->repaint();
    }
    return;
  }

  ggggSpriteSheetLoading.store(true);
  ggggSpriteSheetLoadCancel.store(false);

  skinLoadingOverlayVisible = true;
  skinLoadingOverlayText = "loading";
  if (view_.owner) {
    view_.owner->repaint();
  }

  if (ggggSpriteSheetLoadThread.joinable()) {
    ggggSpriteSheetLoadCancel.store(true);
    ggggSpriteSheetLoadThread.join();
    ggggSpriteSheetLoadCancel.store(false);
  }

  ggggSpriteSheetLoadThread = std::thread([this]() {
    auto img = loadImageFromBinary(kGgggSpriteSheet);

    if (ggggSpriteSheetLoadCancel.load()) {
      ggggSpriteSheetLoading.store(false);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(ggggSpriteSheetMutex);
      ggggSpriteSheetStaged = img;
    }

    ggggSpriteSheetReady.store(true);
    ggggSpriteSheetLoading.store(false);
  });
}

void XYSkinAnimator::consumeGgggSpriteSheetIfReady() {
  if (!ggggSpriteSheetReady.load()) {
    return;
  }

  juce::Image staged;
  {
    std::lock_guard<std::mutex> lock(ggggSpriteSheetMutex);
    staged = ggggSpriteSheetStaged;
    ggggSpriteSheetStaged = {};
  }

  ggggSpriteSheetReady.store(false);

  if (ggggSpriteSheetLoadCancel.load()) {
    return;
  }

  ggggSpriteSheet = staged;
  skinLoadingOverlayVisible = false;

  if (ggggSpriteSheet.isValid()) {
    setGgggFrameIndex(0);
  }

  if (view_.owner) {
    view_.owner->repaint();
  }
}

// ====== DS ======
void XYSkinAnimator::startDsFrameAnimation() {
  if (!dsSpriteSheet.isValid()) {
    beginLoadDsSpriteSheetAsync();
    return;
  }

  const auto& program = tremolo::defaults::dsTriggerFrameProgram;
  if (program.empty()) {
    return;
  }

  const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, dsTriggerProgramIndex);
  const auto seg = program[static_cast<size_t>(segIndex)];

  const int from = juce::jlimit(0, kDsFrameCount - 1, seg.fromFrame);
  const int to = juce::jlimit(0, kDsFrameCount - 1, seg.toFrame);

  dsTriggerStep = (from <= to) ? +1 : -1;
  dsTriggerEndFrame = to;

  dsFrameAnimActive = true;
  dsFrameIndex = from;
  dsFrameTimeAccSec = 0.0;

  setDsFrameIndex(from);

  dsTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
}

void XYSkinAnimator::stopDsFrameAnimation() {
  dsFrameAnimActive = false;
  dsFrameTimeAccSec = 0.0;
  dsTriggerStep = +1;
  dsTriggerEndFrame = dsFrameIndex;
}

void XYSkinAnimator::setDsFrameIndex(int newIndex) {
  if (!dsSpriteSheet.isValid() || !view_.btImage) {
    return;
  }

  dsFrameIndex = juce::jlimit(0, kDsFrameCount - 1, newIndex);

  const int x = dsFrameIndex * kDsFrameWidthPx;
  const auto clipped = dsSpriteSheet.getClippedImage(
      juce::Rectangle<int>{x, 0, kDsFrameWidthPx, kDsFrameHeightPx});

  if (clipped.isValid()) {
    view_.btImage->setImage(clipped);
  }
}

// ====== DS async loading ======
void XYSkinAnimator::beginLoadDsSpriteSheetAsync() {
  if (dsSpriteSheet.isValid()) {
    skinLoadingOverlayVisible = false;
    return;
  }

  if (dsSpriteSheetLoading.load()) {
    skinLoadingOverlayVisible = true;
    skinLoadingOverlayText = "loading";
    if (view_.owner) {
      view_.owner->repaint();
    }
    return;
  }

  dsSpriteSheetLoading.store(true);
  dsSpriteSheetLoadCancel.store(false);

  skinLoadingOverlayVisible = true;
  skinLoadingOverlayText = "loading";
  if (view_.owner) {
    view_.owner->repaint();
  }

  if (dsSpriteSheetLoadThread.joinable()) {
    dsSpriteSheetLoadCancel.store(true);
    dsSpriteSheetLoadThread.join();
    dsSpriteSheetLoadCancel.store(false);
  }

  dsSpriteSheetLoadThread = std::thread([this]() {
    auto img = loadImageFromBinary(kDsSpriteSheet);

    if (dsSpriteSheetLoadCancel.load()) {
      dsSpriteSheetLoading.store(false);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(dsSpriteSheetMutex);
      dsSpriteSheetStaged = img;
    }

    dsSpriteSheetReady.store(true);
    dsSpriteSheetLoading.store(false);
  });
}

void XYSkinAnimator::consumeDsSpriteSheetIfReady() {
  if (!dsSpriteSheetReady.load()) {
    return;
  }

  juce::Image staged;
  {
    std::lock_guard<std::mutex> lock(dsSpriteSheetMutex);
    staged = dsSpriteSheetStaged;
    dsSpriteSheetStaged = {};
  }

  dsSpriteSheetReady.store(false);

  if (dsSpriteSheetLoadCancel.load()) {
    return;
  }

  dsSpriteSheet = staged;
  skinLoadingOverlayVisible = false;

  if (dsSpriteSheet.isValid()) {
    setDsFrameIndex(0);
  }

  if (view_.owner) {
    view_.owner->repaint();
  }
}

// ====== ZSZ ======
void XYSkinAnimator::startZszFrameAnimation() {
  if (!zszSpriteSheet.isValid()) {
    beginLoadZszSpriteSheetAsync();
    return;
  }

  const auto& program = tremolo::defaults::zszTriggerFrameProgram;
  if (program.empty()) {
    return;
  }

  const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, zszTriggerProgramIndex);
  const auto seg = program[static_cast<size_t>(segIndex)];

  const int from = juce::jlimit(0, kZszFrameCount - 1, seg.fromFrame);
  const int to = juce::jlimit(0, kZszFrameCount - 1, seg.toFrame);

  zszTriggerStep = (from <= to) ? +1 : -1;
  zszTriggerEndFrame = to;

  zszFrameAnimActive = true;
  zszFrameIndex = from;
  zszFrameTimeAccSec = 0.0;

  setZszFrameIndex(from);

  zszTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
}

void XYSkinAnimator::stopZszFrameAnimation() {
  zszFrameAnimActive = false;
  zszFrameTimeAccSec = 0.0;
  zszTriggerStep = +1;
  zszTriggerEndFrame = zszFrameIndex;
}

void XYSkinAnimator::setZszFrameIndex(int newIndex) {
  if (!zszSpriteSheet.isValid() || !view_.btImage) {
    return;
  }

  zszFrameIndex = juce::jlimit(0, kZszFrameCount - 1, newIndex);

  const int x = zszFrameIndex * kZszFrameWidthPx;
  const auto clipped = zszSpriteSheet.getClippedImage(
      juce::Rectangle<int>{x, 0, kZszFrameWidthPx, kZszFrameHeightPx});

  if (clipped.isValid()) {
    view_.btImage->setImage(clipped);
  }
}

// ====== ZSZ async loading ======
void XYSkinAnimator::beginLoadZszSpriteSheetAsync() {
  if (zszSpriteSheet.isValid()) {
    skinLoadingOverlayVisible = false;
    return;
  }

  if (zszSpriteSheetLoading.load()) {
    skinLoadingOverlayVisible = true;
    skinLoadingOverlayText = "loading";
    if (view_.owner) {
      view_.owner->repaint();
    }
    return;
  }

  zszSpriteSheetLoading.store(true);
  zszSpriteSheetLoadCancel.store(false);

  skinLoadingOverlayVisible = true;
  skinLoadingOverlayText = "loading";
  if (view_.owner) {
    view_.owner->repaint();
  }

  if (zszSpriteSheetLoadThread.joinable()) {
    zszSpriteSheetLoadCancel.store(true);
    zszSpriteSheetLoadThread.join();
    zszSpriteSheetLoadCancel.store(false);
  }

  zszSpriteSheetLoadThread = std::thread([this]() {
    auto img = loadImageFromBinary(kZszSpriteSheet);

    if (zszSpriteSheetLoadCancel.load()) {
      zszSpriteSheetLoading.store(false);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(zszSpriteSheetMutex);
      zszSpriteSheetStaged = img;
    }

    zszSpriteSheetReady.store(true);
    zszSpriteSheetLoading.store(false);
  });
}

void XYSkinAnimator::consumeZszSpriteSheetIfReady() {
  if (!zszSpriteSheetReady.load()) {
    return;
  }

  juce::Image staged;
  {
    std::lock_guard<std::mutex> lock(zszSpriteSheetMutex);
    staged = zszSpriteSheetStaged;
    zszSpriteSheetStaged = {};
  }

  zszSpriteSheetReady.store(false);

  if (zszSpriteSheetLoadCancel.load()) {
    return;
  }

  zszSpriteSheet = staged;
  skinLoadingOverlayVisible = false;

  if (zszSpriteSheet.isValid()) {
    setZszFrameIndex(0);
  }

  if (view_.owner) {
    view_.owner->repaint();
  }
}

// ====== WB ======
void XYSkinAnimator::stopWbFrameAnimation() {
  wbFrameAnimActive = false;
  wbFrameTimeAccSec = 0.0;
  wbTriggerStep = +1;
  wbTriggerEndFrame = wbFrameIndex;
}

void XYSkinAnimator::setWbFrameIndex(int newIndex) {
  if (!wbSpriteSheet.isValid() || !view_.btImage) {
    return;
  }

  wbFrameIndex = juce::jlimit(0, kWbFrameCount - 1, newIndex);

  const int x = wbFrameIndex * kWbFrameWidthPx;
  const auto clipped = wbSpriteSheet.getClippedImage(
      juce::Rectangle<int>{x, 0, kWbFrameWidthPx, kWbFrameHeightPx});

  if (clipped.isValid()) {
    view_.btImage->setImage(clipped);
  }
}

void XYSkinAnimator::beginLoadWbSpriteSheetAsync() {
  if (wbSpriteSheet.isValid()) {
    skinLoadingOverlayVisible = false;
    return;
  }

  if (wbSpriteSheetLoading.load()) {
    skinLoadingOverlayVisible = true;
    skinLoadingOverlayText = "loading";
    if (view_.owner) {
      view_.owner->repaint();
    }
    return;
  }

  wbSpriteSheetLoading.store(true);
  wbSpriteSheetLoadCancel.store(false);

  skinLoadingOverlayVisible = true;
  skinLoadingOverlayText = "loading";
  if (view_.owner) {
    view_.owner->repaint();
  }

  if (wbSpriteSheetLoadThread.joinable()) {
    wbSpriteSheetLoadCancel.store(true);
    wbSpriteSheetLoadThread.join();
    wbSpriteSheetLoadCancel.store(false);
  }

  wbSpriteSheetLoadThread = std::thread([this]() {
    auto img = loadImageFromBinary(kWbSpriteSheet);

    if (wbSpriteSheetLoadCancel.load()) {
      wbSpriteSheetLoading.store(false);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(wbSpriteSheetMutex);
      wbSpriteSheetStaged = img;
    }

    wbSpriteSheetReady.store(true);
    wbSpriteSheetLoading.store(false);
  });
}

void XYSkinAnimator::consumeWbSpriteSheetIfReady() {
  if (!wbSpriteSheetReady.load()) {
    return;
  }

  juce::Image staged;
  {
    std::lock_guard<std::mutex> lock(wbSpriteSheetMutex);
    staged = wbSpriteSheetStaged;
    wbSpriteSheetStaged = {};
  }

  wbSpriteSheetReady.store(false);

  if (wbSpriteSheetLoadCancel.load()) {
    return;
  }

  wbSpriteSheet = staged;
  skinLoadingOverlayVisible = false;

  if (wbSpriteSheet.isValid()) {
    setWbFrameIndex(0);
  }

  if (view_.owner) {
    view_.owner->repaint();
  }
}

// ====== YB ======
void XYSkinAnimator::stopYbFrameAnimation() {
  ybFrameAnimActive = false;
  ybFrameTimeAccSec = 0.0;
  ybTriggerStep = +1;
  ybTriggerEndFrame = ybFrameIndex;
}

void XYSkinAnimator::setYbFrameIndex(int newIndex) {
  if (!ybSpriteSheet.isValid() || !view_.btImage) {
    return;
  }

  ybFrameIndex = juce::jlimit(0, kYbFrameCount - 1, newIndex);

  const int x = ybFrameIndex * kYbFrameWidthPx;
  const auto clipped = ybSpriteSheet.getClippedImage(
      juce::Rectangle<int>{x, 0, kYbFrameWidthPx, kYbFrameHeightPx});

  if (clipped.isValid()) {
    view_.btImage->setImage(clipped);
  }
}

void XYSkinAnimator::startYbFrameAnimation() {
  if (!ybSpriteSheet.isValid()) {
    beginLoadYbSpriteSheetAsync();
    return;
  }

  const auto& program = tremolo::defaults::ybTriggerFrameProgram;
  if (program.empty()) {
    return;
  }

  const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, ybTriggerProgramIndex);
  const auto seg = program[static_cast<size_t>(segIndex)];

  const int from = juce::jlimit(0, kYbFrameCount - 1, seg.fromFrame);
  const int to = juce::jlimit(0, kYbFrameCount - 1, seg.toFrame);

  ybTriggerStep = (from <= to) ? +1 : -1;
  ybTriggerEndFrame = to;

  setYbFrameIndex(from);

  ybFrameAnimActive = true;
  ybFrameTimeAccSec = 0.0;

  ybTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
}

void XYSkinAnimator::beginLoadYbSpriteSheetAsync() {
  if (ybSpriteSheet.isValid()) {
    skinLoadingOverlayVisible = false;
    return;
  }

  if (ybSpriteSheetLoading.load()) {
    skinLoadingOverlayVisible = true;
    skinLoadingOverlayText = "loading";
    if (view_.owner) {
      view_.owner->repaint();
    }
    return;
  }

  ybSpriteSheetLoading.store(true);
  ybSpriteSheetLoadCancel.store(false);

  skinLoadingOverlayVisible = true;
  skinLoadingOverlayText = "loading";
  if (view_.owner) {
    view_.owner->repaint();
  }

  if (ybSpriteSheetLoadThread.joinable()) {
    ybSpriteSheetLoadCancel.store(true);
    ybSpriteSheetLoadThread.join();
    ybSpriteSheetLoadCancel.store(false);
  }

  ybSpriteSheetLoadThread = std::thread([this]() {
    auto img = loadImageFromBinary(kYbSpriteSheet);

    if (ybSpriteSheetLoadCancel.load()) {
      ybSpriteSheetLoading.store(false);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(ybSpriteSheetMutex);
      ybSpriteSheetStaged = img;
    }

    ybSpriteSheetReady.store(true);
    ybSpriteSheetLoading.store(false);
  });
}

void XYSkinAnimator::consumeYbSpriteSheetIfReady() {
  if (!ybSpriteSheetReady.load()) {
    return;
  }

  juce::Image staged;
  {
    std::lock_guard<std::mutex> lock(ybSpriteSheetMutex);
    staged = ybSpriteSheetStaged;
    ybSpriteSheetStaged = {};
  }

  ybSpriteSheetReady.store(false);

  if (ybSpriteSheetLoadCancel.load()) {
    return;
  }

  ybSpriteSheet = staged;
  skinLoadingOverlayVisible = false;

  if (ybSpriteSheet.isValid()) {
    setYbFrameIndex(0);
  }

  if (view_.owner) {
    view_.owner->repaint();
  }
}

// ====== DAN ======
void XYSkinAnimator::stopDanFrameAnimation() {
  danFrameAnimActive = false;
  danFrameTimeAccSec = 0.0;
  danTriggerStep = +1;
  danTriggerEndFrame = danFrameIndex;
}

void XYSkinAnimator::setDanFrameIndex(int newIndex) {
  if (!danSpriteSheet.isValid() || !view_.btImage) {
    return;
  }

  danFrameIndex = juce::jlimit(0, kDanFrameCount - 1, newIndex);

  const int x = danFrameIndex * kDanFrameWidthPx;
  const auto clipped = danSpriteSheet.getClippedImage(
      juce::Rectangle<int>{x, 0, kDanFrameWidthPx, kDanFrameHeightPx});

  if (clipped.isValid()) {
    view_.btImage->setImage(clipped);
  }
}

void XYSkinAnimator::startDanFrameAnimation() {
  if (!danSpriteSheet.isValid()) {
    beginLoadDanSpriteSheetAsync();
    return;
  }

  const auto& program = tremolo::defaults::danTriggerFrameProgram;
  if (program.empty()) {
    return;
  }

  const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, danTriggerProgramIndex);
  const auto seg = program[static_cast<size_t>(segIndex)];

  const int from = juce::jlimit(0, kDanFrameCount - 1, seg.fromFrame);
  const int to = juce::jlimit(0, kDanFrameCount - 1, seg.toFrame);

  danTriggerStep = (from <= to) ? +1 : -1;
  danTriggerEndFrame = to;

  setDanFrameIndex(from);

  danFrameAnimActive = true;
  danFrameTimeAccSec = 0.0;

  danTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
}

void XYSkinAnimator::beginLoadDanSpriteSheetAsync() {
  if (danSpriteSheet.isValid()) {
    skinLoadingOverlayVisible = false;
    return;
  }

  if (danSpriteSheetLoading.load()) {
    skinLoadingOverlayVisible = true;
    skinLoadingOverlayText = "loading";
    if (view_.owner) {
      view_.owner->repaint();
    }
    return;
  }

  danSpriteSheetLoading.store(true);
  danSpriteSheetLoadCancel.store(false);

  skinLoadingOverlayVisible = true;
  skinLoadingOverlayText = "loading";
  if (view_.owner) {
    view_.owner->repaint();
  }

  if (danSpriteSheetLoadThread.joinable()) {
    danSpriteSheetLoadCancel.store(true);
    danSpriteSheetLoadThread.join();
    danSpriteSheetLoadCancel.store(false);
  }

  danSpriteSheetLoadThread = std::thread([this]() {
    auto img = loadImageFromBinary(kDanSpriteSheet);

    if (danSpriteSheetLoadCancel.load()) {
      danSpriteSheetLoading.store(false);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(danSpriteSheetMutex);
      danSpriteSheetStaged = img;
    }

    danSpriteSheetReady.store(true);
    danSpriteSheetLoading.store(false);
  });
}

void XYSkinAnimator::consumeDanSpriteSheetIfReady() {
  if (!danSpriteSheetReady.load()) {
    return;
  }

  juce::Image staged;
  {
    std::lock_guard<std::mutex> lock(danSpriteSheetMutex);
    staged = danSpriteSheetStaged;
    danSpriteSheetStaged = {};
  }

  danSpriteSheetReady.store(false);

  if (danSpriteSheetLoadCancel.load()) {
    return;
  }

  danSpriteSheet = staged;
  skinLoadingOverlayVisible = false;

  if (danSpriteSheet.isValid()) {
    setDanFrameIndex(0);
  }

  if (view_.owner) {
    view_.owner->repaint();
  }
}

// ====== GZY ======
void XYSkinAnimator::stopGzyFrameAnimation() {
  gzyFrameAnimActive = false;
  gzyFrameTimeAccSec = 0.0;
  gzyTriggerStep = +1;
  gzyTriggerEndFrame = gzyFrameIndex;
}

void XYSkinAnimator::setGzyFrameIndex(int newIndex) {
  if (!gzySpriteSheet.isValid() || !view_.btImage) {
    return;
  }

  gzyFrameIndex = juce::jlimit(0, kGzyFrameCount - 1, newIndex);

  const int col = gzyFrameIndex % kGzyFramesPerRow;
  const int row = gzyFrameIndex / kGzyFramesPerRow;

  const int x = col * kGzyFrameWidthPx;
  const int y = row * kGzyFrameHeightPx;

  const auto clipped = gzySpriteSheet.getClippedImage(
      juce::Rectangle<int>{x, y, kGzyFrameWidthPx, kGzyFrameHeightPx});

  if (clipped.isValid()) {
    view_.btImage->setImage(clipped);
  }
}

void XYSkinAnimator::startGzyFrameAnimation() {
  if (!gzySpriteSheet.isValid()) {
    beginLoadGzySpriteSheetAsync();
    return;
  }

  const auto& program = tremolo::defaults::gzyTriggerFrameProgram;
  if (program.empty()) {
    return;
  }

  const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, gzyTriggerProgramIndex);
  const auto seg = program[static_cast<size_t>(segIndex)];

  const int from = juce::jlimit(0, kGzyFrameCount - 1, seg.fromFrame);
  const int to = juce::jlimit(0, kGzyFrameCount - 1, seg.toFrame);

  gzyTriggerStep = (from <= to) ? +1 : -1;
  gzyTriggerEndFrame = to;

  setGzyFrameIndex(from);

  gzyFrameAnimActive = true;
  gzyFrameTimeAccSec = 0.0;

  gzyTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
}

void XYSkinAnimator::beginLoadGzySpriteSheetAsync() {
  if (gzySpriteSheet.isValid()) {
    skinLoadingOverlayVisible = false;
    return;
  }

  if (gzySpriteSheetLoading.load()) {
    skinLoadingOverlayVisible = true;
    skinLoadingOverlayText = "loading";
    if (view_.owner) {
      view_.owner->repaint();
    }
    return;
  }

  gzySpriteSheetLoading.store(true);
  gzySpriteSheetLoadCancel.store(false);

  skinLoadingOverlayVisible = true;
  skinLoadingOverlayText = "loading";
  if (view_.owner) {
    view_.owner->repaint();
  }

  if (gzySpriteSheetLoadThread.joinable()) {
    gzySpriteSheetLoadCancel.store(true);
    gzySpriteSheetLoadThread.join();
    gzySpriteSheetLoadCancel.store(false);
  }

  gzySpriteSheetLoadThread = std::thread([this]() {
    auto img = loadImageFromBinary(kGzySpriteSheet);

    if (gzySpriteSheetLoadCancel.load()) {
      gzySpriteSheetLoading.store(false);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(gzySpriteSheetMutex);
      gzySpriteSheetStaged = img;
    }

    gzySpriteSheetReady.store(true);
    gzySpriteSheetLoading.store(false);
  });
}

void XYSkinAnimator::consumeGzySpriteSheetIfReady() {
  if (!gzySpriteSheetReady.load()) {
    return;
  }

  juce::Image staged;
  {
    std::lock_guard<std::mutex> lock(gzySpriteSheetMutex);
    staged = gzySpriteSheetStaged;
    gzySpriteSheetStaged = {};
  }

  gzySpriteSheetReady.store(false);

  if (gzySpriteSheetLoadCancel.load()) {
    return;
  }

  gzySpriteSheet = staged;
  skinLoadingOverlayVisible = false;

  if (gzySpriteSheet.isValid()) {
    setGzyFrameIndex(0);
  }

  if (view_.owner) {
    view_.owner->repaint();
  }
  }

// ====== KK ======
void XYSkinAnimator::stopKkFrameAnimation() {
  kkFrameAnimActive = false;
  kkFrameTimeAccSec = 0.0;
  kkTriggerStep = +1;
  kkTriggerEndFrame = kkFrameIndex;
}

void XYSkinAnimator::setKkFrameIndex(int newIndex) {
  if (!kkSpriteSheet.isValid() || !view_.btImage) {
    return;
  }

  kkFrameIndex = juce::jlimit(0, kkFrameCount - 1, newIndex);

  const int x = kkFrameIndex * kkFrameWidthPx;
  const auto clipped = kkSpriteSheet.getClippedImage(
      juce::Rectangle<int>{x, 0, kkFrameWidthPx, kkFrameHeightPx});

  if (clipped.isValid()) {
    view_.btImage->setImage(clipped);
  }
}

void XYSkinAnimator::startKkFrameAnimation() {
  if (!kkSpriteSheet.isValid()) {
    beginLoadKkSpriteSheetAsync();
    return;
  }

  const auto& program = tremolo::defaults::kkTriggerFrameProgram;
  if (program.empty()) {
    return;
  }

  const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, kkTriggerProgramIndex);
  const auto seg = program[static_cast<size_t>(segIndex)];

  const int from = juce::jlimit(0, kkFrameCount - 1, seg.fromFrame);
  const int to = juce::jlimit(0, kkFrameCount - 1, seg.toFrame);

  kkTriggerStep = (from <= to) ? +1 : -1;
  kkTriggerEndFrame = to;

  setKkFrameIndex(from);

  kkFrameAnimActive = true;
  kkFrameTimeAccSec = 0.0;

  kkTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
}

void XYSkinAnimator::beginLoadKkSpriteSheetAsync() {
  if (kkSpriteSheet.isValid()) {
    skinLoadingOverlayVisible = false;
    return;
  }

  if (kkSpriteSheetLoading.load()) {
    skinLoadingOverlayVisible = true;
    skinLoadingOverlayText = "loading";
    if (view_.owner) {
      view_.owner->repaint();
    }
    return;
  }

  kkSpriteSheetLoading.store(true);
  kkSpriteSheetLoadCancel.store(false);

  skinLoadingOverlayVisible = true;
  skinLoadingOverlayText = "loading";
  if (view_.owner) {
    view_.owner->repaint();
  }

  if (kkSpriteSheetLoadThread.joinable()) {
    kkSpriteSheetLoadCancel.store(true);
    kkSpriteSheetLoadThread.join();
    kkSpriteSheetLoadCancel.store(false);
  }

  kkSpriteSheetLoadThread = std::thread([this]() {
    auto img = loadImageFromBinary(kkSpriteSheet);

    if (kkSpriteSheetLoadCancel.load()) {
      kkSpriteSheetLoading.store(false);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(kkSpriteSheetMutex);
      kkSpriteSheetStaged = img;
    }

    kkSpriteSheetReady.store(true);
    kkSpriteSheetLoading.store(false);
  });
}

void XYSkinAnimator::consumeKkSpriteSheetIfReady() {
  if (!kkSpriteSheetReady.load()) {
    return;
  }

  juce::Image staged;
  {
    std::lock_guard<std::mutex> lock(kkSpriteSheetMutex);
    staged = kkSpriteSheetStaged;
    kkSpriteSheetStaged = {};
  }

  kkSpriteSheetReady.store(false);

  if (kkSpriteSheetLoadCancel.load()) {
    return;
  }

  kkSpriteSheet = staged;
  skinLoadingOverlayVisible = false;

  if (kkSpriteSheet.isValid()) {
    setKkFrameIndex(0);
  }

  if (view_.owner) {
    view_.owner->repaint();
  }
}

}  // namespace tremolo