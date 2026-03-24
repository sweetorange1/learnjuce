#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Defaults.h"

namespace tremolo {

// JJ预设动画：用于BT皮肤里jj.png的上下往复。
// 当前需求：逻辑单独拆分，且暂时屏蔽功能（保留结构便于未来重新启用）。
class JjAnimator {
public:
  struct View {
    juce::Component* xyContainer{};
    juce::ImageComponent* jjImage{};
  };

  void attach(View view) { view_ = view; }

  // 暂时屏蔽：默认关闭
  void setEnabled(bool enabled) { enabled_ = enabled; stop(); }
  bool isEnabled() const noexcept { return enabled_; }

  // 触发一次完整动画（上行+下行）
  void trigger(float indicatorDurationSec);

  // dtSec：每帧时间步长（秒）
  void tick(double dtSec);

  bool isAnimating() const noexcept { return isAnimating_; }
  void stop();

private:
  View view_{};
  bool enabled_{false};

  bool isAnimating_{false};
  bool isMovingUp_{true};
  float animationProgress_{0.0f};
  float animationDuration_{0.0f};
  float startYPosition_{0.0f};
  float targetYPosition_{0.0f};

  static float easeInOutQuad(float t);
  static float easeOutInQuad(float t);

  void applyJjBoundsForYOffset(float yOffsetPx);
};

}  // namespace tremolo
