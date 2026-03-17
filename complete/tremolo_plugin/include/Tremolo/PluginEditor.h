#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "VolumeMeter.h"

namespace tremolo {

// XY控制器组件：用于通过鼠标交互获取X和Y坐标值
class XYController : public juce::Component {
public:
    XYController();
    
    // 鼠标事件处理
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    // 外部同步（例如从宿主恢复参数后刷新UI）
    void setValues(float newXValue, float newYValue, bool sendCallback = false);
    
    // 获取当前X和Y值（范围0.0到1.0）
    float getXValue() const { return xValue; }
    float getYValue() const { return yValue; }
    
    // 设置值变化回调函数
    void setValueChangeCallback(std::function<void(float, float)> callback) {
        valueChangeCallback = callback;
    }
    
    // 绘制组件
    void paint(juce::Graphics& g) override;
    
private:
    float xValue{0.5f}; // X值，范围0.0到1.0
    float yValue{0.5f}; // Y值，范围0.0到1.0
    std::function<void(float, float)> valueChangeCallback;
    
    // 更新位置并触发回调
    void updatePosition(juce::Point<float> position);
};

// 设置面板组件
class SettingsPanel : public juce::Component {
public:
    SettingsPanel();
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setVisible(bool shouldBeVisible) override;
    
    // 更新音量电平
    void updateVolumeLevel(float level);
    
    // 设置显示模式
    void setVolumeMeterMode(bool useWaveform);

    void setThresholdDb(float thresholdDb);
    void setThresholdChangedCallback(std::function<void(float)> callback);

    void setLevelCaptureWindowMs(float windowMs);
    void setLevelCaptureWindowChangedCallback(std::function<void(float)> callback);

    void setInputFilterFrequencies(float highpassHz, float lowpassHz);
    void setInputFilterChangedCallback(
        std::function<void(float, float)> callback);

private:
    void updateInputFilterText(float highpassHz, float lowpassHz);
    void updateLevelWindowText(float windowMs);

    juce::Label titleLabel;
    VolumeMeter volumeMeter; // 音量表组件
    juce::Label volumeLabel; // 音量标签
    juce::ToggleButton waveformToggle; // 波形显示切换按钮

    juce::Label levelWindowLabel; // 电平捕捉窗口标签
    juce::Slider levelWindowSlider; // 电平捕捉窗口控制条
    juce::Label levelWindowValueLabel; // 电平捕捉窗口数值显示
    std::function<void(float)> levelWindowChangedCallback;

    juce::Label inputFilterLabel; // 输入检测滤波器标签
    juce::Slider inputFilterSlider; // 双端点频率范围控制器
    juce::Label inputFilterValueLabel; // 当前滤波器频率文本
    std::function<void(float, float)> inputFilterChangedCallback;
};

class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit PluginEditor(PluginProcessor&);
  ~PluginEditor() override;

  void resized() override;
  void paint(juce::Graphics& g) override;
  void timerCallback() override;

private:
  juce::ImageComponent background;
  juce::ImageComponent logo;
  juce::ImageComponent jjImage;
  juce::ImageButton settingsButton; // 设置按钮

  // 图片动画相关变量
  bool isAnimating = false;
  bool isMovingUp = true;
  float animationProgress = 0.0f;
  float animationDuration = 0.0f;
  float startYPosition = 0.0f;
  float targetYPosition = 0.0f;
  bool isFirstIndicatorFlash = true; // 是否是第一次指示灯亮起，用于控制图片初始隐藏状态
  
  // 设置面板相关变量
  SettingsPanel settingsPanel;
  bool isSettingsPanelVisible = false;
  
  // 缓动函数：实现先快后慢和由慢变快的效果
  float easeInOutQuad(float t);
  float easeOutInQuad(float t);
  
  // 动画更新函数
  void updateAnimation();

  juce::Label bypassLabel{"bypass label", "BYPASS"};
  juce::ToggleButton bypassButton{"BYPASSED"};
  juce::ButtonParameterAttachment bypassAttachment;

  juce::Label gainLabel{"gain label", "GAIN"}; // 增益标签
  juce::Slider gainSlider; // 增益控制条
  juce::SliderParameterAttachment gainAttachment; // 增益参数附件

  XYController xyController; // XY控制器组件
  MessageOnClick about;
  
  juce::Label indicatorLabel{"indicator label", "PEAK"}; // 指示灯标签
  
  // 自定义指示灯组件
  class IndicatorLight : public juce::Component {
  public:
    IndicatorLight() = default;
    
    void paint(juce::Graphics& g) override {
      auto bounds = getLocalBounds().toFloat();
      
      // 根据状态设置指示灯颜色
      juce::Colour indicatorColor = shouldFlash ? juce::Colours::red : juce::Colours::darkgrey;
      
      // 绘制指示灯背景（圆形）
      g.setColour(indicatorColor);
      g.fillEllipse(bounds);
      
      // 绘制指示灯边框
      g.setColour(juce::Colours::white);
      g.drawEllipse(bounds, 2.0f);
    }
    
    void setFlashing(bool flashing) {
      shouldFlash = flashing;
      repaint();
    }
    
  private:
    bool shouldFlash = false;
  };
  
  IndicatorLight indicatorLight; // 指示灯组件

  CustomLookAndFeel lookAndFeel;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
}  // namespace tremolo