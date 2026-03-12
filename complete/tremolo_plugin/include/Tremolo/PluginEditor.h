#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace tremolo {

// XY控制器组件：用于通过鼠标交互获取X和Y坐标值
class XYController : public juce::Component {
public:
    XYController();
    
    // 鼠标事件处理
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
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
