#include "../include/Tremolo/JjAnimator.h"

#include <cmath>

namespace tremolo {

void JjAnimator::trigger(float indicatorDurationSec) {
  if (!enabled_) {
    return;
  }

  if (!view_.xyContainer || !view_.jjImage) {
    return;
  }

  isAnimating_ = true;
  isMovingUp_ = true;
  animationProgress_ = 0.0f;

  // 与旧逻辑保持一致：duration/2作为单程时间
  animationDuration_ = juce::jmax(1.0e-4f, indicatorDurationSec * 0.5f);

  startYPosition_ = tremolo::defaults::jjAnimationStartYOffsetPx;
  targetYPosition_ = tremolo::defaults::jjAnimationPeakYOffsetPx;

  applyJjBoundsForYOffset(startYPosition_);
}

void JjAnimator::stop() {
  isAnimating_ = false;
  animationProgress_ = 0.0f;
  isMovingUp_ = true;

  startYPosition_ = tremolo::defaults::jjAnimationStartYOffsetPx;
  targetYPosition_ = tremolo::defaults::jjAnimationPeakYOffsetPx;

  if (view_.xyContainer && view_.jjImage) {
    applyJjBoundsForYOffset(tremolo::defaults::jjAnimationStartYOffsetPx);
  }
}

void JjAnimator::tick(double /*dtSec*/) {
  // 暂时屏蔽：即使被错误调用也不会动
  if (!enabled_) {
    return;
  }

  if (!isAnimating_) {
    return;
  }

  if (!view_.xyContainer || !view_.jjImage) {
    return;
  }

  // 保持旧实现：依赖60Hz调用频率，通过progress += 1/(duration*60)
  animationProgress_ += 1.0f / (animationDuration_ * 60.0f);

  if (animationProgress_ >= 1.0f) {
    if (isMovingUp_) {
      isMovingUp_ = false;
      animationProgress_ = 0.0f;
      startYPosition_ = tremolo::defaults::jjAnimationPeakYOffsetPx;
      targetYPosition_ = tremolo::defaults::jjAnimationStartYOffsetPx;
    } else {
      stop();
      return;
    }
  }

  float currentYOffset = 0.0f;
  if (isMovingUp_) {
    const float eased = easeInOutQuad(animationProgress_);
    currentYOffset = startYPosition_ + (targetYPosition_ - startYPosition_) * eased;
  } else {
    const float eased = easeOutInQuad(animationProgress_);
    currentYOffset = startYPosition_ + (targetYPosition_ - startYPosition_) * eased;
  }

  applyJjBoundsForYOffset(currentYOffset);
}

float JjAnimator::easeInOutQuad(float t) {
  return t < 0.5f ? 2.0f * t * t
                  : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float JjAnimator::easeOutInQuad(float t) {
  return t < 0.5f ? 0.5f * (1.0f - std::pow(1.0f - 2.0f * t, 3.0f))
                  : 0.5f * (1.0f + std::pow(2.0f * t - 1.0f, 3.0f));
}

void JjAnimator::applyJjBoundsForYOffset(float yOffsetPx) {
  if (!view_.xyContainer || !view_.jjImage) {
    return;
  }

  const auto baseBounds = view_.xyContainer->getLocalBounds()
                              .withSizeKeepingCentre(
                                  juce::jmax(1, juce::roundToInt(51.0f * tremolo::defaults::jjImageScale)),
                                  juce::jmax(1, juce::roundToInt(325.0f * tremolo::defaults::jjImageScale)))
                              .translated(0, 200);

  auto jjImageBounds = baseBounds.translated(0, static_cast<int>(-yOffsetPx));
  view_.jjImage->setBounds(jjImageBounds);
}

}  // namespace tremolo
