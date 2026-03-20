#include "tremolo_plugin/include/Tremolo/VolumeMeter.h"
#include <cmath>

namespace tremolo {

VolumeMeter::VolumeMeter() {
    // 初始化历史数据
    levelHistory.fill(0.0f);
    
    // 启动定时器用于实时更新显示（60fps）
    startTimerHz(60);
}

VolumeMeter::~VolumeMeter() {
    // 停止定时器
    stopTimer();
}

void VolumeMeter::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    auto plotBounds = bounds.reduced(8.0f);
    auto meterArea = plotBounds;
    const bool shouldShowDbScale = showDbScale.load(std::memory_order_relaxed);
    if (shouldShowDbScale) {
        meterArea.removeFromLeft(34.0f);
    }

    // 绘制背景
    g.setColour(meterBackground);
    g.fillRoundedRectangle(bounds, 5.0f);
    
    // 绘制边框
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    if (shouldShowDbScale) {
        for (const auto db : {0.0f, -12.0f, -24.0f, -36.0f, -48.0f, -60.0f}) {
            const auto y = thresholdDbToY(db, meterArea);
            g.setColour(juce::Colours::white.withAlpha(0.12f));
            g.drawLine(meterArea.getX(), y, meterArea.getRight(), y, 1.0f);

            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
            g.drawText(juce::String(db, 0) + " dB",
                       juce::Rectangle<float>(plotBounds.getX(), y - 8.0f, 30.0f, 16.0f),
                       juce::Justification::centredRight, false);
        }
    }

    if (useWaveformDisplay) {
        // 滚动曲线模式：按时间从左到右显示历史电平。
        g.setColour(waveformColor);
        
        // 计算波形路径
        juce::Path waveformPath;
        const float width = meterArea.getWidth();
        const float height = meterArea.getHeight();
        const float bottomY = meterArea.getBottom();
        const float startX = meterArea.getX();

        // 绘制波形
        for (int i = 0; i < HISTORY_SIZE; ++i) {
            int index = (historyIndex + i) % HISTORY_SIZE;
            float x = startX + (float)i / (HISTORY_SIZE - 1) * width;
            float y = bottomY - (levelHistory[index] * height);

            if (i == 0) {
                waveformPath.startNewSubPath(x, y);
            } else {
                waveformPath.lineTo(x, y);
            }
        }
        
        // 绘制波形线
        g.strokePath(waveformPath, juce::PathStrokeType(2.0f));
        
    } else {
        // 电平条显示模式（自下向上），与dB阈值横线语义保持一致。
        const float currentLevelValue = currentLevel.load();
        const float peakLevelValue = peakLevel.load();

        // 绘制当前电平条
        if (currentLevelValue > 0.0f) {
            const float levelHeight = meterArea.getHeight() * currentLevelValue;
            auto levelBounds = juce::Rectangle<float>(meterArea.getX(),
                                                      meterArea.getBottom() - levelHeight,
                                                      meterArea.getWidth(),
                                                      levelHeight);

            // 根据电平值设置颜色（绿色到黄色到红色）
            juce::Colour levelColor;
            if (currentLevelValue < 0.7f) {
                levelColor = juce::Colour(0xFF6F6F6F);
            } else if (currentLevelValue < 0.9f) {
                levelColor = juce::Colour(0xFFB0B0B0);
            } else {
                levelColor = juce::Colour(0xFFE0E0E0);
            }
            
            g.setColour(levelColor);
            g.fillRoundedRectangle(levelBounds, 5.0f);
        }
        
        // 绘制峰值指示器
        if (peakLevelValue > 0.0f) {
            const float peakY = meterArea.getBottom() - meterArea.getHeight() * peakLevelValue;
            g.setColour(peakIndicator);
            g.drawLine(meterArea.getX(), peakY, meterArea.getRight(), peakY, 2.0f);
        }
    }

    const auto threshold = thresholdDb.load(std::memory_order_relaxed);
    const auto thresholdY = thresholdDbToY(threshold, meterArea);
    g.setColour(thresholdColor);
    g.drawLine(meterArea.getX(), thresholdY, meterArea.getRight(), thresholdY, 1.5f);

    const auto thresholdText = juce::String(threshold, 1) + " dB";
    auto labelBounds = juce::Rectangle<float>(meterArea.getRight() - 88.0f,
                                              thresholdY - 10.0f,
                                              84.0f,
                                              18.0f);
    labelBounds = labelBounds.constrainedWithin(meterArea.reduced(2.0f));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(labelBounds, 3.0f);
    g.setColour(thresholdColor.brighter(0.3f));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText(thresholdText, labelBounds, juce::Justification::centredRight, false);
}

void VolumeMeter::resized() {
    // 组件大小调整时无需特殊处理
}

void VolumeMeter::mouseDown(const juce::MouseEvent& event) {
    updateThresholdFromY(event.position.y);
}

void VolumeMeter::mouseDrag(const juce::MouseEvent& event) {
    updateThresholdFromY(event.position.y);
}

void VolumeMeter::timerCallback() {
    // 更新峰值保持计时器
    const float deltaTime = 1.0f / 60.0f; // 60fps
    float currentPeakHoldTimer = peakHoldTimer.load();
    
    if (currentPeakHoldTimer > 0.0f) {
        peakHoldTimer.store(currentPeakHoldTimer - deltaTime);
        if (peakHoldTimer.load() <= 0.0f) {
            peakLevel.store(0.0f);
        }
    }
    
    // 触发重绘
    repaint();
}

void VolumeMeter::updateLevel(float newLevel) {
    // 将线性增益转换到dB刻度后再归一化到0..1，保证与阈值线语义一致。
    const auto minGain = juce::Decibels::decibelsToGain(minThresholdDb);
    const auto safeGain = juce::jmax(newLevel, minGain);
    const auto levelDb = juce::Decibels::gainToDecibels(safeGain, minThresholdDb);
    const auto normalizedLevel = juce::jmap(levelDb, minThresholdDb, maxThresholdDb, 0.0f, 1.0f);
    const auto clampedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);
    currentLevel.store(clampedLevel);
    
    // 更新历史数据（用于波形显示）
    levelHistory[historyIndex] = clampedLevel;
    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    
    // 更新峰值电平
    if (clampedLevel > peakLevel.load()) {
        peakLevel.store(clampedLevel);
        peakHoldTimer.store(peakHoldDuration.load());
    }
}

void VolumeMeter::setDisplayMode(bool useWaveform) {
    useWaveformDisplay.store(useWaveform);
    repaint();
}

void VolumeMeter::setShowDbScale(bool shouldShow) {
    showDbScale.store(shouldShow, std::memory_order_relaxed);
    repaint();
}

void VolumeMeter::setPeakHoldTime(float seconds) {
    peakHoldDuration.store(juce::jlimit(0.1f, 10.0f, seconds));
}

void VolumeMeter::setThresholdDb(float newThresholdDb) {
    thresholdDb.store(juce::jlimit(minThresholdDb, maxThresholdDb, newThresholdDb),
                      std::memory_order_relaxed);
    repaint();
}

float VolumeMeter::getThresholdDb() const {
    return thresholdDb.load(std::memory_order_relaxed);
}

void VolumeMeter::setThresholdChangedCallback(std::function<void(float)> callback) {
    thresholdChangedCallback = callback;
}

float VolumeMeter::thresholdDbToY(float db, juce::Rectangle<float> bounds) const {
    return juce::jmap(juce::jlimit(minThresholdDb, maxThresholdDb, db),
                      maxThresholdDb,
                      minThresholdDb,
                      bounds.getY(),
                      bounds.getBottom());
}

float VolumeMeter::yToThresholdDb(float y, juce::Rectangle<float> bounds) const {
    return juce::jmap(juce::jlimit(bounds.getY(), bounds.getBottom(), y),
                      bounds.getY(),
                      bounds.getBottom(),
                      maxThresholdDb,
                      minThresholdDb);
}

void VolumeMeter::updateThresholdFromY(float y) {
    auto plotBounds = getLocalBounds().toFloat().reduced(8.0f);
    auto meterArea = plotBounds;
    if (showDbScale.load(std::memory_order_relaxed)) {
        meterArea.removeFromLeft(34.0f);
    }

    const auto db = yToThresholdDb(y, meterArea);
    setThresholdDb(db);

    if (thresholdChangedCallback) {
        thresholdChangedCallback(db);
    }
}

}  // namespace tremolo