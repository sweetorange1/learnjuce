#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "VolumeMeter.h"
#include "Defaults.h"
#include "XYSkinAnimator.h"
#include "JjAnimator.h"

#include <atomic>
#include <thread>

namespace tremolo {

// 触发源：用于指示灯与XY动画（Audio阈值 / MIDI输入 / 宿主BPM）
enum class TriggerSource : int { Audio = 0, Midi = 1, Bpm = 2 };

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

    // 触发源（通过一个按钮循环切换：Audio -> MIDI -> BPM -> Audio）
    void setTriggerSource(TriggerSource source);
    void setTriggerSourceChangedCallback(std::function<void(TriggerSource)> callback);

    // BPM触发频率（音符时值索引）
    void setBpmDivisionIndex(int index);
    void setBpmDivisionChangedCallback(std::function<void(int)> callback);

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

    void updateTriggerModeButtonText();

    juce::Label titleLabel;
    VolumeMeter volumeMeter; // 音量表组件
    juce::Label volumeLabel; // 音量标签
    juce::ToggleButton waveformToggle; // 波形显示切换按钮

    juce::TextButton triggerModeButton; // 循环切换触发源
    TriggerSource triggerSource{TriggerSource::Audio};
    std::function<void(TriggerSource)> triggerSourceChangedCallback;

    juce::Label bpmDivisionLabel; // BPM触发频率
    juce::Slider bpmDivisionSlider; // 0..(count-1)
    juce::Label bpmDivisionValueLabel;
    std::function<void(int)> bpmDivisionChangedCallback;

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

  tremolo::defaults::XYSkinId currentXYSkin{tremolo::defaults::xyDefaultSkin};
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

  // BPM触发模式：指示灯与XY动画由宿主BPM驱动
  bool bpmModeEnabled{false};
  uint64_t lastBpmTriggerSequence{0};
  double bpmFlashTimerSec{0.0};
  int bpmHistoryPulseFramesRemaining{0};

  // XY皮肤预设动画管理（HCR/GGGG/WB/DS/ZSZ），从PluginEditor中剥离
  XYSkinAnimator xySkinAnimator;

  // JJ预设动画：单独文件，当前默认禁用（仅保留结构）
  JjAnimator jjAnimator;

  // UI：切换皮肤加载提示（由xySkinAnimator维护）

  // 图片动画相关变量
  // 旧JJ动画变量已迁移至 JjAnimator（当前默认禁用）
  
  // 设置面板相关变量
  SettingsPanel settingsPanel;
  bool isSettingsPanelVisible = false;
  
  // JJ动画实现已迁移至 JjAnimator

  // XY皮肤切换与动画驱动
  void setXYSkin(tremolo::defaults::XYSkinId newSkin);
  void updateXYSkinVisualsForIndicator(bool shouldFlash, bool retriggered, double dtSec);

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