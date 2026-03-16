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
    
    // 定时器回调（用于实时更新显示）
    void timerCallback() override;
    
    // 更新音量值（从音频线程调用）
    void updateLevel(float newLevel);
    
    // 设置显示模式（电平条或波形图）
    void setDisplayMode(bool useWaveform);
    
    // 设置峰值保持时间
    void setPeakHoldTime(float seconds);

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
    
    // 颜色配置
    juce::Colour meterBackground{juce::Colour(0xFF222222)};     // 背景色
    juce::Colour meterForeground{juce::Colour(0xFF00FF00)};      // 前景色（绿色）
    juce::Colour peakIndicator{juce::Colour(0xFFFF0000)};       // 峰值指示器（红色）
    juce::Colour waveformColor{juce::Colour(0xFF6EA0C7)};       // 波形颜色
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VolumeMeter)
};

}  // namespace tremolo