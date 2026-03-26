#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Defaults.h"

#include <atomic>
#include <mutex>
#include <thread>

namespace tremolo {

// XY皮肤动画管理器：
// - 负责不同XY皮肤（HCR/GGGG/WB/DS/ZSZ/BT）的“资源绑定 + 帧动画播放/停留”
// - 将与皮肤预设相关的状态从 PluginEditor 中剥离出来
// - WB的sprite sheet采用后台线程加载，并通过timer线程轮询提交到UI，避免消息线程回调悬空
class XYSkinAnimator {
public:
  struct View {
    juce::Component* owner{}; // 用于repaint

    juce::Component* xyContainer{};

    juce::ImageComponent* btImage{};
    juce::ImageComponent* toneImage{};

    // BT皮肤会用到jj.png（目前仅负责显示/隐藏与绑定图片；动画由JjAnimator管理）
    juce::Component* jjClipper{};
    juce::ImageComponent* jjImage{};
  };

  XYSkinAnimator() = default;
  ~XYSkinAnimator();

  XYSkinAnimator(const XYSkinAnimator&) = delete;
  XYSkinAnimator& operator=(const XYSkinAnimator&) = delete;

  void attach(View view);

  void setSkin(tremolo::defaults::XYSkinId newSkin);

  // shouldFlash: 指示灯是否闪烁
  // retriggered: 是否发生“重触发”（快速连续触发）
  // dtSec: 本帧增量时间（秒）
  void tick(tremolo::defaults::XYSkinId currentSkin,
            bool shouldFlash,
            bool retriggered,
            double dtSec);

  bool isLoadingOverlayVisible() const noexcept {
    return skinLoadingOverlayVisible;
  }

  juce::String getLoadingOverlayText() const {
    return skinLoadingOverlayText;
  }

  void shutdown();

private:
  // 资源与显示
  View view_{};

  // 指示灯边沿检测
  bool wasIndicatorFlashing{false};

  // UI：切换皮肤加载提示（目前用于WB异步加载）
  bool skinLoadingOverlayVisible{false};
  juce::String skinLoadingOverlayText{"loading"};

  // ====== 帧动画：通用推进逻辑 ======
  static void advanceFrameAnimation(bool& active,
                                   int& frameIndex,
                                   double& timeAccSec,
                                   int triggerStep,
                                   int triggerEndFrame,
                                   int frameMin,
                                   int frameMax,
                                   double frameDurationSec,
                                   double dtSec,
                                   const std::function<void(int)>& setFrameIndex,
                                   const std::function<void()>& stop);

  // ====== HCR ======
  bool hcrFrameAnimActive{false};
  int hcrFrameIndex{0};
  double hcrFrameTimeAccSec{0.0};
  juce::Image hcrSpriteSheet;
  int hcrTriggerStep{+1};
  int hcrTriggerEndFrame{0};

  void startHcrFrameAnimation();
  void stopHcrFrameAnimation();
  void setHcrFrameIndex(int newIndex);

  // ====== GGGG ======
  bool ggggFrameAnimActive{false};
  int ggggFrameIndex{0};
  double ggggFrameTimeAccSec{0.0};
  juce::Image ggggSpriteSheet;
  int ggggTriggerProgramIndex{0};
  int ggggTriggerStep{+1};
  int ggggTriggerEndFrame{0};

  std::atomic<bool> ggggSpriteSheetLoading{false};
  std::atomic<bool> ggggSpriteSheetLoadCancel{false};
  std::atomic<bool> ggggSpriteSheetReady{false};
  std::thread ggggSpriteSheetLoadThread;
  std::mutex ggggSpriteSheetMutex;
  juce::Image ggggSpriteSheetStaged;

  void startGgggFrameAnimation();
  void stopGgggFrameAnimation();
  void setGgggFrameIndex(int newIndex);
  void beginLoadGgggSpriteSheetAsync();
  void consumeGgggSpriteSheetIfReady();

  // ====== WB ======
  bool wbFrameAnimActive{false};
  int wbFrameIndex{0};
  double wbFrameTimeAccSec{0.0};
  juce::Image wbSpriteSheet;
  int wbTriggerProgramIndex{0};
  int wbTriggerStep{+1};
  int wbTriggerEndFrame{0};

  std::atomic<bool> wbSpriteSheetLoading{false};
  std::atomic<bool> wbSpriteSheetLoadCancel{false};
  std::atomic<bool> wbSpriteSheetReady{false};
  std::thread wbSpriteSheetLoadThread;
  std::mutex wbSpriteSheetMutex;
  juce::Image wbSpriteSheetStaged;

  void stopWbFrameAnimation();
  void setWbFrameIndex(int newIndex);
  void beginLoadWbSpriteSheetAsync();
  void consumeWbSpriteSheetIfReady();

  // ====== DS ======
  bool dsFrameAnimActive{false};
  int dsFrameIndex{0};
  double dsFrameTimeAccSec{0.0};
  juce::Image dsSpriteSheet;
  int dsTriggerProgramIndex{0};
  int dsTriggerStep{+1};
  int dsTriggerEndFrame{0};

  std::atomic<bool> dsSpriteSheetLoading{false};
  std::atomic<bool> dsSpriteSheetLoadCancel{false};
  std::atomic<bool> dsSpriteSheetReady{false};
  std::thread dsSpriteSheetLoadThread;
  std::mutex dsSpriteSheetMutex;
  juce::Image dsSpriteSheetStaged;

  void startDsFrameAnimation();
  void stopDsFrameAnimation();
  void setDsFrameIndex(int newIndex);
  void beginLoadDsSpriteSheetAsync();
  void consumeDsSpriteSheetIfReady();

  // ====== ZSZ ======
  bool zszFrameAnimActive{false};
  int zszFrameIndex{0};
  double zszFrameTimeAccSec{0.0};
  juce::Image zszSpriteSheet;
  int zszTriggerProgramIndex{0};
  int zszTriggerStep{+1};
  int zszTriggerEndFrame{0};

  std::atomic<bool> zszSpriteSheetLoading{false};
  std::atomic<bool> zszSpriteSheetLoadCancel{false};
  std::atomic<bool> zszSpriteSheetReady{false};
  std::thread zszSpriteSheetLoadThread;
  std::mutex zszSpriteSheetMutex;
  juce::Image zszSpriteSheetStaged;

  void startZszFrameAnimation();
  void stopZszFrameAnimation();
  void setZszFrameIndex(int newIndex);
  void beginLoadZszSpriteSheetAsync();
  void consumeZszSpriteSheetIfReady();

  // ====== YB ======
  bool ybFrameAnimActive{false};
  int ybFrameIndex{0};
  double ybFrameTimeAccSec{0.0};
  juce::Image ybSpriteSheet;
  int ybTriggerProgramIndex{0};
  int ybTriggerStep{+1};
  int ybTriggerEndFrame{0};

  std::atomic<bool> ybSpriteSheetLoading{false};
  std::atomic<bool> ybSpriteSheetLoadCancel{false};
  std::atomic<bool> ybSpriteSheetReady{false};
  std::thread ybSpriteSheetLoadThread;
  std::mutex ybSpriteSheetMutex;
  juce::Image ybSpriteSheetStaged;

  void startYbFrameAnimation();
  void stopYbFrameAnimation();
  void setYbFrameIndex(int newIndex);
  void beginLoadYbSpriteSheetAsync();
  void consumeYbSpriteSheetIfReady();

  // ====== DAN ======
  bool danFrameAnimActive{false};
  int danFrameIndex{0};
  double danFrameTimeAccSec{0.0};
  juce::Image danSpriteSheet;
  int danTriggerProgramIndex{0};
  int danTriggerStep{+1};
  int danTriggerEndFrame{0};

  std::atomic<bool> danSpriteSheetLoading{false};
  std::atomic<bool> danSpriteSheetLoadCancel{false};
  std::atomic<bool> danSpriteSheetReady{false};
  std::thread danSpriteSheetLoadThread;
  std::mutex danSpriteSheetMutex;
  juce::Image danSpriteSheetStaged;

  void startDanFrameAnimation();
  void stopDanFrameAnimation();
  void setDanFrameIndex(int newIndex);
  void beginLoadDanSpriteSheetAsync();
  void consumeDanSpriteSheetIfReady();

  // ====== GZY ======
  bool gzyFrameAnimActive{false};
  int gzyFrameIndex{0};
  double gzyFrameTimeAccSec{0.0};
  juce::Image gzySpriteSheet;
  int gzyTriggerProgramIndex{0};
  int gzyTriggerStep{+1};
  int gzyTriggerEndFrame{0};

  std::atomic<bool> gzySpriteSheetLoading{false};
  std::atomic<bool> gzySpriteSheetLoadCancel{false};
  std::atomic<bool> gzySpriteSheetReady{false};
  std::thread gzySpriteSheetLoadThread;
  std::mutex gzySpriteSheetMutex;
  juce::Image gzySpriteSheetStaged;

  void startGzyFrameAnimation();
  void stopGzyFrameAnimation();
  void setGzyFrameIndex(int newIndex);
  void beginLoadGzySpriteSheetAsync();
  void consumeGzySpriteSheetIfReady();

  // ====== KK ======
  bool kkFrameAnimActive{false};
  int kkFrameIndex{0};
  double kkFrameTimeAccSec{0.0};
  juce::Image kkSpriteSheet;
  int kkTriggerProgramIndex{0};
  int kkTriggerStep{+1};
  int kkTriggerEndFrame{0};

  std::atomic<bool> kkSpriteSheetLoading{false};
  std::atomic<bool> kkSpriteSheetLoadCancel{false};
  std::atomic<bool> kkSpriteSheetReady{false};
  std::thread kkSpriteSheetLoadThread;
  std::mutex kkSpriteSheetMutex;
  juce::Image kkSpriteSheetStaged;

  void startKkFrameAnimation();
  void stopKkFrameAnimation();
  void setKkFrameIndex(int newIndex);
  void beginLoadKkSpriteSheetAsync();
  void consumeKkSpriteSheetIfReady();
};

}  // namespace tremolo