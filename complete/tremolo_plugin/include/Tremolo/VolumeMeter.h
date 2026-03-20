#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace tremolo {

// 音量电平监测组件：实时显示输入信号的音量电平
class VolumeMeter : public juce::Component, private juce::Timer {
public:
    VolumeMeter();
    ~VolumeMeter() override;

    // 组件绘制
    void paint(juce::Graphics& g) override;
    
    // 组件大小调整
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    // 定时器回调（用于实时更新显示）
    void timerCallback() override;
    
    // 更新音量值（从音频线程调用）
    void updateLevel(float newLevel);
    
    // 设置显示模式（电平条或波形图）
    void setDisplayMode(bool useWaveform);
    
    // 设置峰值保持时间
    void setPeakHoldTime(float seconds);

    void setThresholdDb(float thresholdDb);
    float getThresholdDb() const;
    void setThresholdChangedCallback(std::function<void(float)> callback);

private:
    // 音量电平相关变量
    std::atomic<float> currentLevel{0.0f};     // 当前音量电平
    std::atomic<float> peakLevel{0.0f};         // 峰值电平
    std::atomic<float> peakHoldTimer{0.0f};    // 峰值保持计时器
    
    // 显示模式
    std::atomic<bool> useWaveformDisplay{false}; // 是否使用波形显示
    
    // 峰值保持时间（秒）
    std::atomic<float> peakHoldDuration{2.0f};   // 峰值保持持续时间
    
    // 历史数据用于波形显示
    static constexpr int HISTORY_SIZE = 200;     // 历史数据大小
    std::array<float, HISTORY_SIZE> levelHistory; // 历史电平数据
    int historyIndex{0};                         // 历史数据索引

    // 触发阈值（dB）
    std::atomic<float> thresholdDb{-12.0f};
    std::function<void(float)> thresholdChangedCallback;

    // 颜色配置
    juce::Colour meterBackground{juce::Colour(0xFF222222)};     // 背景色
    juce::Colour meterForeground{juce::Colour(0xFFBDBDBD)};     // 前景色（灰）
    juce::Colour peakIndicator{juce::Colour(0xFFE6E6E6)};       // 峰值指示器（亮灰）
    juce::Colour waveformColor{juce::Colour(0xFFC8C8C8)};       // 波形颜色（灰）
    juce::Colour thresholdColor{juce::Colour(0xFFE04A4A)};      // 阈值线颜色（红，提示该线为手动设置）

    static constexpr float minThresholdDb = -60.0f;
    static constexpr float maxThresholdDb = 0.0f;

    float thresholdDbToY(float db, juce::Rectangle<float> bounds) const;
    float yToThresholdDb(float y, juce::Rectangle<float> bounds) const;
    void updateThresholdFromY(float y);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VolumeMeter)
};

}  // namespace tremolo