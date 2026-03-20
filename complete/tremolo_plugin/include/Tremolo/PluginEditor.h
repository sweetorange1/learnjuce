#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "VolumeMeter.h"
#include "Defaults.h"

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
    
    // MIDI触发模式开关（用于把指示灯/XY动画触发器切换到MIDI输入）
    void setMidiMode(bool enabled);
    void setMidiModeChangedCallback(std::function<void(bool)> callback);
    
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
    
    juce::TextButton midiModeButton; // switch midi mod
    bool midiModeEnabled{false};
    std::function<void(bool)> midiModeChangedCallback;

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
  void paintOverChildren(juce::Graphics& g) override;
   void timerCallback() override;

 private:
  juce::ImageComponent background;
  juce::ImageComponent logo;

  // XY控制器视觉与交互分离：
  // - xyContainer: 负责XY区域的统一布局/裁剪/层级
  // - btImage: XY底图（bt.png）
  // - jjClipper + jjImage: jj动画图（jj.png），越界部分自动裁剪
  // - toneImage: XY控制点（tone.png）
  juce::Component xyContainer;
  juce::ImageComponent btImage;
  juce::Component jjClipper; // 用于裁剪jj.png显示范围（限制在XY控制器内部）
  juce::ImageComponent jjImage;
  juce::ImageComponent toneImage;

  juce::ImageButton settingsButton; // 设置按钮
  juce::TextButton skinsButton;   // skins按钮（切换XY皮肤）

  tremolo::defaults::XYSkinId currentXYSkin{tremolo::defaults::XYSkinId::BT};
  bool xySkinInitialized{false};

  // 指示灯状态边沿检测（用于触发“每次闪烁”动画）
  bool wasIndicatorFlashing{false};

  // 指示灯“阈值触发序列号”追踪：用于识别快速连续触发并立刻重播动画
  uint64_t lastIndicatorTriggerSequence{0};
  
  // MIDI触发模式：指示灯与XY动画由MIDI输入驱动
  bool midiModeEnabled{false};
  uint64_t lastMidiTriggerSequence{0};
  double midiFlashTimerSec{0.0};
  int midiHistoryPulseFramesRemaining{0};

  // HCR皮肤：逐帧动画状态（002->006）
  bool hcrFrameAnimActive{false};
  int hcrFrameIndex{0};
  double hcrFrameTimeAccSec{0.0};

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

  // XY皮肤切换与动画驱动
  void setXYSkin(tremolo::defaults::XYSkinId newSkin);
  void updateXYSkinVisualsForIndicator(bool shouldFlash, bool retriggered, double dtSec);
  void startHcrFrameAnimation();
  void stopHcrFrameAnimation();

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
      juce::Colour indicatorColor = shouldFlash ? juce::Colour(0xFFE0E0E0)
                                                : juce::Colour(0xFF3A3A3A);
      
      // 绘制指示灯背景（圆形）
      g.setColour(indicatorColor);
      g.fillEllipse(bounds);
      
      // 绘制指示灯边框
      g.setColour(juce::Colour(0xFF8A8A8A));
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