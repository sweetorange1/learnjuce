#include "../include/Tremolo/SpectrumAnalyser.h"

namespace tremolo {

SpectrumAnalyser::SpectrumAnalyser()
    : abstractFifo(48000), // 初始FIFO大小
      window(4096, juce::dsp::WindowingFunction<float>::kaiser) // 初始窗函数
{
    // 设置初始FFT大小
    setupAnalyser(44100.0, 12);
    
    // 启动定时器用于频谱更新（30fps）
    startTimerHz(30);
}

SpectrumAnalyser::~SpectrumAnalyser()
{
    stopTimer();
}

void SpectrumAnalyser::setupAnalyser(double newSampleRate, int fftOrder)
{
    sampleRate = newSampleRate;
    fftSize = 1 << fftOrder; // 2^fftOrder
    
    // 创建FFT处理器
    fft = std::make_unique<juce::dsp::FFT>(fftOrder);
    
    // 设置音频FIFO
    audioFifo.setSize(1, fftSize * 2); // 双倍大小用于重叠处理
    abstractFifo.setTotalSize(fftSize * 2);
    
    // 设置FFT缓冲区
    fftBuffer.setSize(1, fftSize * 2);
    windowedBuffer.setSize(1, fftSize);
    
    // 重新初始化窗函数（Kaiser窗）
    window.~WindowingFunction();
    new (&window) juce::dsp::WindowingFunction<float>(fftSize, juce::dsp::WindowingFunction<float>::kaiser);
}

void SpectrumAnalyser::addAudioData(const juce::AudioBuffer<float>& buffer, int channel)
{
    if (!isActive) return;
    
    const int numSamples = buffer.getNumSamples();
    
    // 检查FIFO是否有足够空间
    if (abstractFifo.getFreeSpace() < numSamples) return;
    
    // 写入FIFO
    int start1, block1, start2, block2;
    abstractFifo.prepareToWrite(numSamples, start1, block1, start2, block2);
    
    if (block1 > 0)
        audioFifo.copyFrom(0, start1, buffer.getReadPointer(channel), block1);
    
    if (block2 > 0)
        audioFifo.copyFrom(0, start2, buffer.getReadPointer(channel, block1), block2);
    
    abstractFifo.finishedWrite(block1 + block2);
}

void SpectrumAnalyser::setActive(bool shouldBeActive)
{
    isActive = shouldBeActive;
    
    if (isActive) {
        // 激活时清空缓冲区
        audioFifo.clear();
        abstractFifo.reset();
    }
}

void SpectrumAnalyser::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // 绘制背景（深灰色）
    g.setColour(juce::Colour(0xFF222222));
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // 绘制边框
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    
    // 绘制网格线
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    
    // 垂直网格线（对数频率）
    const float frequencies[] = {20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f};
    for (auto freq : frequencies) {
        float x = frequencyToX(freq, bounds);
        if (x >= 0 && x <= bounds.getWidth()) {
            g.drawVerticalLine(static_cast<int>(bounds.getX() + x), 
                              bounds.getY(), bounds.getBottom());
        }
    }
    
    // 水平网格线（分贝）
    const float decibels[] = {-80.0f, -60.0f, -40.0f, -20.0f, 0.0f};
    for (auto db : decibels) {
        float y = decibelsToY(db, bounds);
        if (y >= 0 && y <= bounds.getHeight()) {
            g.drawHorizontalLine(static_cast<int>(bounds.getY() + y), 
                               bounds.getX(), bounds.getRight());
        }
    }
    
    // 绘制频谱路径
    {
        juce::ScopedLock lock(pathLock);
        g.setColour(juce::Colours::cyan.withAlpha(0.8f));
        g.strokePath(spectrumPath, juce::PathStrokeType(2.0f));
    }
    
    // 绘制频率标签
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    
    for (auto freq : frequencies) {
        float x = frequencyToX(freq, bounds);
        if (x >= 0 && x <= bounds.getWidth()) {
            juce::String label = freq < 1000.0f ? juce::String(freq, 0) + " Hz" : juce::String(freq / 1000.0f, 1) + " kHz";
            g.drawText(label, static_cast<int>(bounds.getX() + x - 25), 
                      static_cast<int>(bounds.getBottom() - 15), 50, 12, juce::Justification::centred);
        }
    }
    
    // 绘制分贝标签
    for (auto db : decibels) {
        float y = decibelsToY(db, bounds);
        if (y >= 0 && y <= bounds.getHeight()) {
            juce::String label = juce::String(db, 0) + " dB";
            g.drawText(label, static_cast<int>(bounds.getX() + 2), 
                      static_cast<int>(bounds.getY() + y - 6), 40, 12, juce::Justification::left);
        }
    }
    
    // 绘制标题
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.drawText("Input Spectrum", bounds.toNearestInt(), juce::Justification::centredTop);
}

void SpectrumAnalyser::resized()
{
    // 组件大小改变时重新创建频谱路径
    createSpectrumPath();
}

void SpectrumAnalyser::timerCallback()
{
    if (!isActive) return;
    
    // 检查是否有足够的数据进行FFT分析
    if (abstractFifo.getNumReady() >= fftSize) {
        performFFT();
        createSpectrumPath();
        repaint();
    }
}

void SpectrumAnalyser::performFFT()
{
    // 从FIFO读取数据
    int start1, block1, start2, block2;
    abstractFifo.prepareToRead(fftSize, start1, block1, start2, block2);
    
    // 复制数据到FFT缓冲区
    fftBuffer.clear();
    
    if (block1 > 0)
        fftBuffer.copyFrom(0, 0, audioFifo.getReadPointer(0, start1), block1);
    
    if (block2 > 0)
        fftBuffer.copyFrom(0, block1, audioFifo.getReadPointer(0, start2), block2);
    
    abstractFifo.finishedRead(block1 + block2);
    
    // 应用窗函数
    window.multiplyWithWindowingTable(fftBuffer.getWritePointer(0), fftSize);
    
    // 执行FFT
    fft->performFrequencyOnlyForwardTransform(fftBuffer.getWritePointer(0));
}

void SpectrumAnalyser::createSpectrumPath()
{
    juce::ScopedLock lock(pathLock);
    
    auto bounds = getLocalBounds().toFloat().reduced(10.0f);
    spectrumPath.clear();
    
    if (fftBuffer.getNumSamples() == 0) return;
    
    const auto* fftData = fftBuffer.getReadPointer(0);
    const int numBins = fftSize / 2;
    
    // 创建频谱路径
    bool started = false;
    
    for (int i = 1; i < numBins; ++i) {
        float frequency = static_cast<float>((sampleRate * i) / fftSize);
        
        // 只显示指定频率范围内的数据
        if (frequency < minFrequency || frequency > maxFrequency) continue;
        
        float x = frequencyToX(frequency, bounds);
        float magnitude = fftData[i];
        float decibels = juce::Decibels::gainToDecibels(magnitude, static_cast<float>(minDecibels));
        float y = decibelsToY(decibels, bounds);
        
        if (!started) {
            spectrumPath.startNewSubPath(x, y);
            started = true;
        } else {
            spectrumPath.lineTo(x, y);
        }
    }
}

float SpectrumAnalyser::frequencyToX(float frequency, const juce::Rectangle<float>& bounds) const
{
    // 对数频率映射
    float logMin = std::log(minFrequency);
    float logMax = std::log(maxFrequency);
    float logFreq = std::log(frequency);
    
    return juce::jmap(logFreq, logMin, logMax, 0.0f, bounds.getWidth());
}

float SpectrumAnalyser::decibelsToY(float decibels, const juce::Rectangle<float>& bounds) const
{
    return juce::jmap(decibels, minDecibels, maxDecibels, bounds.getHeight(), 0.0f);
}

} // namespace tremolo