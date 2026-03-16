#include "tremolo_plugin/include/Tremolo/VolumeMeter.h"
#include <cmath>

namespace tremolo {

VolumeMeter::VolumeMeter() {
    // 初始化历史数据
    levelHistory.fill(0.0f);
    
    // 启动定时器用于实时更新显示（30fps）
    startTimerHz(30);
}

VolumeMeter::~VolumeMeter() {
    // 停止定时器
    stopTimer();
}

void VolumeMeter::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // 绘制背景
    g.setColour(meterBackground);
    g.fillRoundedRectangle(bounds, 5.0f);
    
    // 绘制边框
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
    
    if (useWaveformDisplay) {
        // 滚动曲线模式：按时间从左到右显示历史电平。
        g.setColour(waveformColor);
        
        // 计算波形路径
        juce::Path waveformPath;
        float width = bounds.getWidth();
        float height = bounds.getHeight();
        const float bottomY = bounds.getBottom();

        // 绘制波形
        for (int i = 0; i < HISTORY_SIZE; ++i) {
            int index = (historyIndex + i) % HISTORY_SIZE;
            float x = (float)i / (HISTORY_SIZE - 1) * width;
            float y = bottomY - levelHistory[index] * height;

            if (i == 0) {
                waveformPath.startNewSubPath(x, y);
            } else {
                waveformPath.lineTo(x, y);
            }
        }
        
        // 绘制波形线
        g.strokePath(waveformPath, juce::PathStrokeType(2.0f));
        
    } else {
        // 电平条显示模式
        float currentLevelValue = currentLevel.load();
        float peakLevelValue = peakLevel.load();
        
        // 绘制当前电平条
        if (currentLevelValue > 0.0f) {
            float levelWidth = bounds.getWidth() * currentLevelValue;
            auto levelBounds = bounds.withWidth(levelWidth);
            
            // 根据电平值设置颜色（绿色到黄色到红色）
            juce::Colour levelColor;
            if (currentLevelValue < 0.7f) {
                levelColor = juce::Colours::green;
            } else if (currentLevelValue < 0.9f) {
                levelColor = juce::Colours::yellow;
            } else {
                levelColor = juce::Colours::red;
            }
            
            g.setColour(levelColor);
            g.fillRoundedRectangle(levelBounds, 5.0f);
        }
        
        // 绘制峰值指示器
        if (peakLevelValue > 0.0f) {
            float peakX = bounds.getWidth() * peakLevelValue;
            g.setColour(peakIndicator);
            g.drawLine(peakX, 0.0f, peakX, bounds.getHeight(), 2.0f);
        }
    }

    const auto threshold = thresholdDb.load(std::memory_order_relaxed);
    const auto thresholdY = thresholdDbToY(threshold, bounds);
    g.setColour(thresholdColor);
    g.drawLine(bounds.getX(), thresholdY, bounds.getRight(), thresholdY, 1.5f);

    const auto thresholdText = juce::String(threshold, 1) + " dB";
    auto labelBounds = juce::Rectangle<float>(bounds.getRight() - 88.0f,
                                              thresholdY - 10.0f,
                                              84.0f,
                                              18.0f);
    labelBounds = labelBounds.constrainedWithin(bounds.reduced(2.0f));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(labelBounds, 3.0f);
    g.setColour(thresholdColor.brighter(0.3f));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(thresholdText, labelBounds, juce::Justification::centredRight, false);

    // 绘制刻度线和标签
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(10.0f));
    
    // 绘制刻度线
    float width = bounds.getWidth();
    for (int i = 0; i <= 10; ++i) {
        float xPos = width * (i / 10.0f);
        g.drawLine(xPos, 0.0f, xPos, 5.0f, 1.0f);
        
        // 绘制刻度标签
        if (i % 2 == 0) {
            juce::String label = juce::String(i * 10) + "%";
            g.drawText(label, juce::Rectangle<float>(xPos - 20.0f, bounds.getHeight() - 15.0f, 40.0f, 12.0f), 
                      juce::Justification::centred, true);
        }
    }
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
    float deltaTime = 1.0f / 30.0f; // 30fps
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
    const auto db = yToThresholdDb(y, getLocalBounds().toFloat());
    setThresholdDb(db);

    if (thresholdChangedCallback) {
        thresholdChangedCallback(db);
    }
}

}  // namespace tremolo