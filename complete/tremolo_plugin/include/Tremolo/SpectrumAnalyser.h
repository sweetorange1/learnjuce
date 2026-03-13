#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace tremolo {

/**
 * @brief 频谱分析器组件，用于实时显示输入音频信号的频率分布
 */
class SpectrumAnalyser : public juce::Component, public juce::Timer {
public:
    SpectrumAnalyser();
    ~SpectrumAnalyser() override;

    /**
     * @brief 设置频谱分析器参数
     * @param sampleRate 采样率
     * @param fftOrder FFT阶数（决定频率分辨率）
     */
    void setupAnalyser(double sampleRate, int fftOrder = 12);

    /**
     * @brief 添加音频数据进行分析
     * @param buffer 音频缓冲区
     * @param channel 要分析的通道索引
     */
    void addAudioData(const juce::AudioBuffer<float>& buffer, int channel = 0);

    /**
     * @brief 设置频谱分析器是否激活
     * @param shouldBeActive 是否激活分析器
     */
    void setActive(bool shouldBeActive);

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Timer overrides
    void timerCallback() override;

private:
    // FFT相关成员
    std::unique_ptr<juce::dsp::FFT> fft;
    juce::AudioBuffer<float> fftBuffer;
    juce::AudioBuffer<float> windowedBuffer;
    juce::dsp::WindowingFunction<float> window;
    
    // 音频数据缓冲区
    juce::AudioBuffer<float> audioFifo;
    juce::AbstractFifo abstractFifo;
    
    // 频谱路径
    juce::Path spectrumPath;
    juce::CriticalSection pathLock;
    
    // 分析参数
    double sampleRate = 44100.0;
    int fftSize = 4096;
    bool isActive = false;
    
    // 频谱显示参数
    float minFrequency = 20.0f;    // 最低显示频率（Hz）
    float maxFrequency = 20000.0f; // 最高显示频率（Hz）
    float minDecibels = -80.0f;    // 最低显示分贝
    float maxDecibels = 0.0f;       // 最高显示分贝
    
    /**
     * @brief 执行FFT分析
     */
    void performFFT();
    
    /**
     * @brief 创建频谱路径
     */
    void createSpectrumPath();
    
    /**
     * @brief 将频率转换为X坐标
     * @param frequency 频率值
     * @param bounds 显示边界
     * @return X坐标
     */
    float frequencyToX(float frequency, const juce::Rectangle<float>& bounds) const;
    
    /**
     * @brief 将分贝值转换为Y坐标
     * @param decibels 分贝值
     * @param bounds 显示边界
     * @return Y坐标
     */
    float decibelsToY(float decibels, const juce::Rectangle<float>& bounds) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyser)
};

} // namespace tremolo