#pragma once

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

class PluginEditor : public juce::AudioProcessorEditor {
public:
  explicit PluginEditor(PluginProcessor&);
  ~PluginEditor() override;

  void resized() override;

private:
  juce::ImageComponent background;
  juce::ImageComponent logo;

  juce::Label bypassLabel{"bypass label", "BYPASS"};
  juce::ToggleButton bypassButton{"BYPASSED"};
  juce::ButtonParameterAttachment bypassAttachment;

  XYController xyController; // XY控制器组件
  MessageOnClick about;

  CustomLookAndFeel lookAndFeel;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
}  // namespace tremolo
