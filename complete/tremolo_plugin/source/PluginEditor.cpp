#include "../include/Tremolo/PluginEditor.h"
#include "../include/Tremolo/Defaults.h"
#include <TremoloPluginAssets.h>
#include <TremoloPluginAssetsGGGG.h>
#include <TremoloPluginAssetsWB.h>
#include <TremoloPluginAssetsDS.h>
#include <TremoloPluginAssetsZSZ.h>

#include <array>
#include <atomic>
#include <limits>
#include <thread>

// tremolo命名空间：C++中使用命名空间来组织代码，避免命名冲突
namespace tremolo {

namespace {
struct BinaryImage {
  const char* data{};
  int size{};
};

inline juce::Image loadImageFromBinary(const BinaryImage& img) {
  return juce::ImageCache::getFromMemory(img.data, img.size);
}

inline float computeGainBoostForXY(float x, float y) {
  constexpr float targetX = tremolo::defaults::xyTargetX;
  constexpr float targetY = tremolo::defaults::xyTargetY;

  const auto distanceToTarget = std::sqrt(std::pow(x - targetX, 2.0f) +
                                          std::pow(y - targetY, 2.0f));

  const auto gainThreshold = juce::jmax(0.0001f, tremolo::defaults::gainBoostDistanceThreshold);
  const auto configuredMaxGain = juce::jmax(1.0f, tremolo::defaults::maxGain);

  if (distanceToTarget >= gainThreshold) {
    return 1.0f;
  }

  auto gainBoost = juce::jmap(static_cast<float>(distanceToTarget),
                             0.0f, gainThreshold,
                             configuredMaxGain, 1.0f);
  return juce::jlimit(1.0f, configuredMaxGain, gainBoost);
}

inline float computeSawWetForXY(float x, float y) {
  constexpr float targetX = tremolo::defaults::xyTargetX;
  constexpr float targetY = tremolo::defaults::xyTargetY;

  const auto distanceToTarget = std::sqrt(std::pow(x - targetX, 2.0f) +
                                          std::pow(y - targetY, 2.0f));

  const auto threshold = juce::jmax(0.0001f, tremolo::defaults::sawWetDistanceThreshold);
  return juce::jlimit(0.0f, 1.0f, 1.0f - (static_cast<float>(distanceToTarget) / threshold));
}

inline tremolo::defaults::XYSkinId nextXYSkin(tremolo::defaults::XYSkinId current) {

  const auto& order = tremolo::defaults::xySkinCycleOrder;
  if (order.empty()) {
    return current;
  }

  for (size_t i = 0; i < order.size(); ++i) {
    if (order[i] == current) {
      return order[(i + 1) % order.size()];
    }
  }
  return order[0];
}

const BinaryImage kBtBase{assets::bt_png, assets::bt_pngSize};
const BinaryImage kBtJj{assets::jj_png, assets::jj_pngSize};
const BinaryImage kBtTone{assets::tone_png, assets::tone_pngSize};

// HCR：001为底图；HCR_pnglist.png为指示灯闪烁时的sprite sheet（22帧）
const BinaryImage kHcrBase{assets::_001_png, assets::_001_pngSize};
const BinaryImage kHcrSpriteSheet{assets::HCR_pnglist_png, assets::HCR_pnglist_pngSize};
const BinaryImage kHcrTone{assets::tone_png2, assets::tone_png2Size};

// GGGG：GGGG_pnglist.png为底图/指示灯共用的sprite sheet（25帧）
const BinaryImage kGgggSpriteSheet{assets_gggg::GGGG_pnglist_png, assets_gggg::GGGG_pnglist_pngSize};
const BinaryImage kGgggTone{assets_gggg::tone_png, assets_gggg::tone_pngSize};

// WB：一张sprite sheet（1行×64列，每帧550×550），指示灯闪烁时按规则切换显示帧
const BinaryImage kWbSpriteSheet{assets_wb::WB_pnglist_png, assets_wb::WB_pnglist_pngSize};
const BinaryImage kWbTone{assets_wb::tone_png, assets_wb::tone_pngSize};

// DS：DS_pnglist.png为底图/指示灯共用的sprite sheet（57帧）
const BinaryImage kDsSpriteSheet{assets_ds::DS_pnglist_png, assets_ds::DS_pnglist_pngSize};
const BinaryImage kDsTone{assets_ds::tone_png, assets_ds::tone_pngSize};

// ZSZ：ZSZ_pnglist.png为底图/指示灯共用的sprite sheet（6帧）
const BinaryImage kZszSpriteSheet{assets_zsz::ZSZ_pnglist_png, assets_zsz::ZSZ_pnglist_pngSize};
const BinaryImage kZszTone{assets_zsz::tone_png, assets_zsz::tone_pngSize};

constexpr int kWbFrameWidthPx = 550;
constexpr int kWbFrameHeightPx = 550;
constexpr int kWbFrameCount = 64;
constexpr int kWbFramesPerTrigger = 32;

constexpr int kDsFrameWidthPx = 550;
constexpr int kDsFrameHeightPx = 550;
constexpr int kDsFrameCount = 57;

constexpr int kZszFrameWidthPx = 550;
constexpr int kZszFrameHeightPx = 550;
constexpr int kZszFrameCount = 6;
}  // namespace

// SettingsPanel类的实现
SettingsPanel::SettingsPanel() {
    // 设置标题标签
    titleLabel.setText("Audio Trigger Setting", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE6E6E6));
    addAndMakeVisible(titleLabel);

    volumeLabel.setText("Thresh", juce::dontSendNotification);
    volumeLabel.setJustificationType(juce::Justification::centredLeft);
    volumeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFD0D0D0));
    addAndMakeVisible(volumeLabel);

    waveformToggle.setButtonText("Display Switching");
    waveformToggle.setToggleState(true, juce::dontSendNotification);
    waveformToggle.onClick = [this]() {
        setVolumeMeterMode(waveformToggle.getToggleState());
    };
    addAndMakeVisible(waveformToggle);

    volumeMeter.setDisplayMode(true);
    addAndMakeVisible(volumeMeter);

    // trigger mod：一个按钮循环切换触发源（Audio -> MIDI -> BPM -> Audio）
    triggerModeButton.setButtonText("switch to midi mod");
    triggerModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1E1E1E));
    triggerModeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF303030));
    triggerModeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE6E6E6));
    triggerModeButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFE6E6E6));
    triggerModeButton.onClick = [this]() {
        TriggerSource next = TriggerSource::Audio;
        switch (triggerSource) {
            case TriggerSource::Audio: next = TriggerSource::Midi; break;
            case TriggerSource::Midi:  next = TriggerSource::Bpm;  break;
            case TriggerSource::Bpm:   next = TriggerSource::Audio; break;
        }

        setTriggerSource(next);
        if (triggerSourceChangedCallback) {
            triggerSourceChangedCallback(triggerSource);
        }
    };
    addAndMakeVisible(triggerModeButton);

    // BPM触发频率（仅BPM模式显示）
    bpmDivisionLabel.setText("BPM Rate", juce::dontSendNotification);
    bpmDivisionLabel.setJustificationType(juce::Justification::centredLeft);
    bpmDivisionLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFD0D0D0));
    addAndMakeVisible(bpmDivisionLabel);

    bpmDivisionSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmDivisionSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    bpmDivisionSlider.setRange(0, tremolo::defaults::bpmDivisionCount - 1, 1);
    bpmDivisionSlider.setValue(tremolo::defaults::bpmDivisionIndexDefault,
                               juce::dontSendNotification);
    bpmDivisionSlider.onValueChange = [this]() {
        const auto idx = static_cast<int>(std::llround(bpmDivisionSlider.getValue()));
        setBpmDivisionIndex(idx);
        if (bpmDivisionChangedCallback) {
            bpmDivisionChangedCallback(idx);
        }
    };
    addAndMakeVisible(bpmDivisionSlider);

    bpmDivisionValueLabel.setJustificationType(juce::Justification::centredLeft);
    bpmDivisionValueLabel.setColour(juce::Label::textColourId,
                                    juce::Colour(0xFFB0B0B0));
    addAndMakeVisible(bpmDivisionValueLabel);

    // 初始隐藏：仅BPM模式显示
    bpmDivisionLabel.setVisible(false);
    bpmDivisionSlider.setVisible(false);
    bpmDivisionValueLabel.setVisible(false);

    levelWindowLabel.setText("Detail", juce::dontSendNotification);
    levelWindowLabel.setJustificationType(juce::Justification::centredLeft);
    levelWindowLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFD0D0D0));
    addAndMakeVisible(levelWindowLabel);

    levelWindowSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    levelWindowSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    levelWindowSlider.setRange(tremolo::defaults::levelCaptureWindowMsMin,
                               tremolo::defaults::levelCaptureWindowMsMax,
                               tremolo::defaults::levelCaptureWindowMsStep);
    levelWindowSlider.setSkewFactorFromMidPoint(
        tremolo::defaults::levelCaptureWindowMsSkewMid);
    levelWindowSlider.setValue(tremolo::defaults::levelCaptureWindowMs,
                               juce::dontSendNotification);
    levelWindowSlider.onValueChange = [this]() {
        const auto windowMs = static_cast<float>(levelWindowSlider.getValue());
        updateLevelWindowText(windowMs);
        if (levelWindowChangedCallback) {
            levelWindowChangedCallback(windowMs);
        }
    };
    addAndMakeVisible(levelWindowSlider);

    levelWindowValueLabel.setJustificationType(juce::Justification::centredLeft);
    levelWindowValueLabel.setColour(juce::Label::textColourId,
                                    juce::Colour(0xFFB0B0B0));
    addAndMakeVisible(levelWindowValueLabel);

    inputFilterLabel.setText("Input Filter", juce::dontSendNotification);
    inputFilterLabel.setJustificationType(juce::Justification::centredLeft);
    inputFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFD0D0D0));
    addAndMakeVisible(inputFilterLabel);

    inputFilterSlider.setSliderStyle(juce::Slider::TwoValueHorizontal);
    inputFilterSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    inputFilterSlider.setRange(20.0, 20000.0, 1.0);
    inputFilterSlider.setSkewFactorFromMidPoint(1000.0);
    inputFilterSlider.setMinAndMaxValues(20.0, 20000.0, juce::dontSendNotification);
    inputFilterSlider.onValueChange = [this]() {
        const auto highpassHz = static_cast<float>(inputFilterSlider.getMinValue());
        const auto lowpassHz = static_cast<float>(inputFilterSlider.getMaxValue());
        updateInputFilterText(highpassHz, lowpassHz);

        if (inputFilterChangedCallback) {
            inputFilterChangedCallback(highpassHz, lowpassHz);
        }
    };
    addAndMakeVisible(inputFilterSlider);

    inputFilterValueLabel.setJustificationType(juce::Justification::centredLeft);
    inputFilterValueLabel.setColour(juce::Label::textColourId,
                                    juce::Colour(0xFFB0B0B0));
    addAndMakeVisible(inputFilterValueLabel);

    updateInputFilterText(tremolo::defaults::inputHighpassHz,
                          tremolo::defaults::inputLowpassHz);
    updateLevelWindowText(tremolo::defaults::levelCaptureWindowMs);
    setBpmDivisionIndex(tremolo::defaults::bpmDivisionIndexDefault);
}

void SettingsPanel::updateTriggerModeButtonText() {
    // 一个按钮循环切换，按钮文字显示“下一次点击会切到哪个模式”
    switch (triggerSource) {
        case TriggerSource::Audio: triggerModeButton.setButtonText("switch to midi mod"); break;
        case TriggerSource::Midi:  triggerModeButton.setButtonText("switch to bpm mod");  break;
        case TriggerSource::Bpm:   triggerModeButton.setButtonText("switch to audio mod"); break;
    }
}

void SettingsPanel::setTriggerSource(TriggerSource source) {
    triggerSource = source;
    updateTriggerModeButtonText();

    const bool specialMode = (triggerSource != TriggerSource::Audio);

    // 模式下：电平检测标题切换；dB提示隐藏
    if (triggerSource == TriggerSource::Midi) {
        volumeLabel.setText("Midi In", juce::dontSendNotification);
    } else if (triggerSource == TriggerSource::Bpm) {
        volumeLabel.setText("BPM", juce::dontSendNotification);
    } else {
        volumeLabel.setText("Thresh", juce::dontSendNotification);
    }
    volumeMeter.setShowDbScale(!specialMode);

    // Audio控制条仅在Audio模式显示
    const bool showAudioControls = !specialMode;
    levelWindowLabel.setVisible(showAudioControls);
    levelWindowSlider.setVisible(showAudioControls);
    levelWindowValueLabel.setVisible(showAudioControls);
    inputFilterLabel.setVisible(showAudioControls);
    inputFilterSlider.setVisible(showAudioControls);
    inputFilterValueLabel.setVisible(showAudioControls);

    // BPM控制条仅在BPM模式显示
    const bool showBpmControls = (triggerSource == TriggerSource::Bpm);
    bpmDivisionLabel.setVisible(showBpmControls);
    bpmDivisionSlider.setVisible(showBpmControls);
    bpmDivisionValueLabel.setVisible(showBpmControls);

    resized();
    repaint();
}

void SettingsPanel::setTriggerSourceChangedCallback(std::function<void(TriggerSource)> callback) {
    triggerSourceChangedCallback = std::move(callback);
}

void SettingsPanel::setBpmDivisionIndex(int index) {
    const int clamped = juce::jlimit(0, tremolo::defaults::bpmDivisionCount - 1, index);
    bpmDivisionSlider.setValue(clamped, juce::dontSendNotification);

    static constexpr std::array<const char*, tremolo::defaults::bpmDivisionCount> kNames = {
        "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"};

    bpmDivisionValueLabel.setText("Rate " + juce::String{kNames[static_cast<size_t>(clamped)]},
                                 juce::dontSendNotification);
}

void SettingsPanel::setBpmDivisionChangedCallback(std::function<void(int)> callback) {
    bpmDivisionChangedCallback = std::move(callback);
}

void SettingsPanel::paint(juce::Graphics& g) {
    // 绘制设置面板背景（半透明黑色）
    g.setColour(juce::Colour(0xCC000000));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
    
    // 绘制边框
    g.setColour(juce::Colour(0xFF6A6A6A));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 10.0f, 2.0f);
}

void SettingsPanel::resized() {
    // 设置标题标签的位置（顶部居中）
    auto content = getLocalBounds().reduced(16);
    auto titleBounds = content.removeFromTop(36);
    titleLabel.setBounds(titleBounds);

    content.removeFromTop(12);
    auto controls = content.removeFromTop(26);
    volumeLabel.setBounds(controls.removeFromLeft(120));
    waveformToggle.setBounds(controls.removeFromLeft(170));

    content.removeFromTop(10);
    volumeMeter.setBounds(content.removeFromTop(160));

    const bool specialMode = (triggerSource != TriggerSource::Audio);

    if (!specialMode) {
        content.removeFromTop(12);
        levelWindowLabel.setBounds(content.removeFromTop(24));

        content.removeFromTop(6);
        levelWindowSlider.setBounds(content.removeFromTop(28));

        content.removeFromTop(4);
        levelWindowValueLabel.setBounds(content.removeFromTop(20));

        content.removeFromTop(12);
        inputFilterLabel.setBounds(content.removeFromTop(24));

        content.removeFromTop(6);
        inputFilterSlider.setBounds(content.removeFromTop(28));

        content.removeFromTop(4);
        inputFilterValueLabel.setBounds(content.removeFromTop(20));
    }

    if (triggerSource == TriggerSource::Bpm) {
        content.removeFromTop(12);
        bpmDivisionLabel.setBounds(content.removeFromTop(24));

        content.removeFromTop(6);
        bpmDivisionSlider.setBounds(content.removeFromTop(28));

        content.removeFromTop(4);
        bpmDivisionValueLabel.setBounds(content.removeFromTop(20));
    }

    // 底部按钮（单按钮循环切换）
    content.removeFromTop(14);
    auto bottomRow = content.removeFromBottom(30);
    triggerModeButton.setBounds(bottomRow.withSizeKeepingCentre(220, 28));
}

void SettingsPanel::setVisible(bool shouldBeVisible) {
    Component::setVisible(shouldBeVisible);
    
    // 如果设置为可见，将其置于最顶层
    if (shouldBeVisible) {
        toFront(false);
    }
}

void SettingsPanel::updateVolumeLevel(float level) {
    volumeMeter.updateLevel(level);
}

void SettingsPanel::setVolumeMeterMode(bool useWaveform) {
    waveformToggle.setToggleState(useWaveform, juce::dontSendNotification);
    volumeMeter.setDisplayMode(useWaveform);
}

void SettingsPanel::setThresholdDb(float thresholdDb) {
    volumeMeter.setThresholdDb(thresholdDb);
}

void SettingsPanel::setThresholdChangedCallback(std::function<void(float)> callback) {
    volumeMeter.setThresholdChangedCallback(callback);
}

void SettingsPanel::setLevelCaptureWindowMs(float windowMs) {
    levelWindowSlider.setValue(windowMs, juce::dontSendNotification);
    updateLevelWindowText(windowMs);
}

void SettingsPanel::setLevelCaptureWindowChangedCallback(std::function<void(float)> callback) {
    levelWindowChangedCallback = callback;
}

void SettingsPanel::setInputFilterFrequencies(float highpassHz, float lowpassHz) {
    inputFilterSlider.setMinAndMaxValues(highpassHz, lowpassHz,
                                         juce::dontSendNotification);
    updateInputFilterText(highpassHz, lowpassHz);
}

void SettingsPanel::setInputFilterChangedCallback(
    std::function<void(float, float)> callback) {
    inputFilterChangedCallback = callback;
}

void SettingsPanel::updateInputFilterText(float highpassHz, float lowpassHz) {
    auto formatFrequency = [](float frequencyHz) {
        return frequencyHz >= 1000.0f
                   ? juce::String(frequencyHz / 1000.0f, 1) + " kHz"
                   : juce::String(frequencyHz, 0) + " Hz";
    };

    inputFilterValueLabel.setText("HP " + formatFrequency(highpassHz) +
                                      "   LP " + formatFrequency(lowpassHz),
                                  juce::dontSendNotification);
}

void SettingsPanel::updateLevelWindowText(float windowMs) {
    levelWindowValueLabel.setText("Window " + juce::String(windowMs, 0) + " ms",
                                  juce::dontSendNotification);
}

// XYController类的实现
XYController::XYController() {
    // 设置XY控制器可以接收鼠标事件
    setInterceptsMouseClicks(true, true);
}

// 鼠标按下事件处理
void XYController::mouseDown(const juce::MouseEvent& event) {
    // 获取鼠标在组件内的相对位置
    auto position = event.getPosition().toFloat();
    updatePosition(position);
}

// 鼠标拖动事件处理
void XYController::mouseDrag(const juce::MouseEvent& event) {
    // 获取鼠标在组件内的相对位置
    auto position = event.getPosition().toFloat();
    updatePosition(position);
}

// 外部同步（例如从宿主恢复参数后刷新UI）
void XYController::setValues(float newXValue, float newYValue, bool sendCallback) {
    xValue = juce::jlimit(0.0f, 1.0f, newXValue);
    yValue = juce::jlimit(0.0f, 1.0f, newYValue);

    repaint();

    if (sendCallback && valueChangeCallback) {
        valueChangeCallback(xValue, yValue);
    }
}

// 更新位置并触发回调
void XYController::updatePosition(juce::Point<float> position) {
    // 获取组件大小
    auto bounds = getLocalBounds().toFloat();

    // 以控制点图片的“中心点”作为坐标含义，并限制中心点不要让图片跑出边界
    const float halfW = tremolo::defaults::xyToneMarkerWidthPx * 0.5f;
    const float halfH = tremolo::defaults::xyToneMarkerHeightPx * 0.5f;

    const float minX = bounds.getX() + halfW;
    const float maxX = bounds.getRight() - halfW;
    const float minY = bounds.getY() + halfH;
    const float maxY = bounds.getBottom() - halfH;

    const float cx = juce::jlimit(minX, maxX, position.x);
    const float cy = juce::jlimit(minY, maxY, position.y);

    const float denomW = juce::jmax(1.0f, maxX - minX);
    const float denomH = juce::jmax(1.0f, maxY - minY);

    // 计算X和Y值（范围0.0到1.0）
    xValue = juce::jlimit(0.0f, 1.0f, (cx - minX) / denomW);
    yValue = juce::jlimit(0.0f, 1.0f, (cy - minY) / denomH);

    // 触发重绘
    repaint();

    // 如果有回调函数，调用它
    if (valueChangeCallback) {
        valueChangeCallback(xValue, yValue);
    }
}

// 绘制XY控制器
void XYController::paint(juce::Graphics& g) {
    // 视觉由外部组件（bt.png / jj.png / tone.png）负责渲染。
    // 这里保持透明，仅用于鼠标交互与坐标计算。
    juce::ignoreUnused(g);
}

// PluginEditor类的构造函数：这是创建插件编辑器界面的入口点
// AudioProcessorEditor(&p)：继承自JUCE框架的音频处理器编辑器基类
// 参数p是PluginProcessor的引用，表示这个编辑器对应的音频处理器
PluginEditor::PluginEditor(PluginProcessor& p)
    // 初始化列表：C++中用于初始化成员变量的高效方式
    // AudioProcessorEditor(&p)：调用基类构造函数，传入音频处理器指针
    : AudioProcessorEditor(&p),
      // about：关于信息组件，显示插件信息
      about{*this, logo,
            // 字符串拼接：使用预定义宏组合插件信息
            JucePlugin_Manufacturer "\n" JucePlugin_Name "\n" __DATE__
                                    "\n" __TIME__
                                    "\nv" JucePlugin_VersionString} {
  
  // 设置背景图片：从内存中加载背景图片资源
  // juce::ImageCache::getFromMemory：JUCE的图片缓存系统，从二进制数据创建图片
  const auto bgImage = juce::ImageCache::getFromMemory(
      assets::Background_png, assets::Background_pngSize);
  background.setImage(bgImage);
  // 将背景组件添加到界面并使其可见
  addAndMakeVisible(background);

  // 让编辑器尺寸自动跟随背景图实际尺寸（便于替换背景图时无需手动改setSize）
  if (bgImage.isValid() && bgImage.getWidth() > 0 && bgImage.getHeight() > 0) {
      setSize(bgImage.getWidth(), bgImage.getHeight());
  } else {
      setSize(700, 700);
  }

  // 设置按钮：从内存中加载设置图标（BinaryData），避免依赖开发机磁盘路径
  const auto settingsIcon = juce::ImageCache::getFromMemory(assets::setting_png, assets::setting_pngSize);
  if (settingsIcon.isValid()) {
      settingsButton.setImages(false, true, true,
          settingsIcon, // 正常状态图片
          1.0f, // 正常状态图片不透明度
          juce::Colours::transparentBlack, // 正常状态覆盖颜色
          settingsIcon, // 悬停状态图片
          1.0f, // 悬停状态图片不透明度
          juce::Colours::white.withAlpha(0.3f), // 悬停状态覆盖颜色
          settingsIcon, // 按下状态图片
          1.0f, // 按下状态图片不透明度
          juce::Colours::white.withAlpha(0.5f) // 按下状态覆盖颜色
      );
  }

  // 设置按钮点击事件
  settingsButton.onClick = [this]() {
      // 切换设置面板的可见状态
      isSettingsPanelVisible = !isSettingsPanelVisible;
      settingsPanel.setVisible(isSettingsPanelVisible);
      
      // 如果设置面板可见，将其置于最顶层
      if (isSettingsPanelVisible) {
          settingsPanel.toFront(false);
      }
  };
  // 将设置按钮添加到界面
  addAndMakeVisible(settingsButton);

  // skins按钮：用于切换XY控制器皮肤
  skinsButton.setButtonText("Skins");
  // 统一黑灰主题（尽量简单，避免默认主题色）
  skinsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1E1E1E));
  skinsButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF303030));
  skinsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE6E6E6));
  skinsButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFE6E6E6));
  skinsButton.onClick = [this]() {
      setXYSkin(nextXYSkin(currentXYSkin));
  };
  addAndMakeVisible(skinsButton);

  // 设置指示灯组件
  indicatorLight.setInterceptsMouseClicks(false, false); // 不接收鼠标事件
  addAndMakeVisible(indicatorLight);

  // XY 区域容器：内部包含bt底图、jj动画、tone控制点和透明交互层
  xyContainer.setInterceptsMouseClicks(false, true);
  addAndMakeVisible(xyContainer);

  // bt.png：XY控制器底图（具体内容由皮肤系统决定）
  btImage.setInterceptsMouseClicks(false, false);
  xyContainer.addAndMakeVisible(btImage);

  // jj.png：动画图（放在裁剪容器里，越界自动裁剪）。注意：某些皮肤会禁用该动画。
  jjImage.setInterceptsMouseClicks(false, false);
  jjImage.setVisible(true); // 默认显示（动画未触发时保持静止）

  jjClipper.setInterceptsMouseClicks(false, false);
  xyContainer.addAndMakeVisible(jjClipper);
  jjClipper.addAndMakeVisible(jjImage);

  // tone.png：XY控制点（独立组件，绘制在最上层；不同皮肤可使用不同图片）
  toneImage.setInterceptsMouseClicks(false, false);
  xyContainer.addAndMakeVisible(toneImage);

  // 透明交互层（XYController）：仅处理鼠标，不负责绘制
  xyContainer.addAndMakeVisible(xyController);

  // XY内部层级：bt(底) -> jj(中) -> tone(上) -> xyController(最上用于接收鼠标)
  btImage.toBack();
  jjClipper.toFront(false);
  toneImage.toFront(false);
  xyController.toFront(false);

  // 启动时应用默认皮肤（来自Defaults配置），避免必须点击skins按钮才加载XY资源
  setXYSkin(tremolo::defaults::xyDefaultSkin);

  // 初始化设置面板
  settingsPanel.setVisible(false); // 初始状态为隐藏
  settingsPanel.setThresholdDb(p.getTriggerThresholdDb());
  settingsPanel.setThresholdChangedCallback([&p](float thresholdDb) {
      p.setTriggerThresholdDb(thresholdDb);
  });

  settingsPanel.setTriggerSource(TriggerSource::Audio);

  settingsPanel.setBpmDivisionIndex(p.getBpmDivisionIndex());
  settingsPanel.setBpmDivisionChangedCallback([&p](int idx) {
      p.setBpmDivisionIndex(idx);
  });

  auto applyTriggerSource = [this, &p](TriggerSource src) {
      midiModeEnabled = (src == TriggerSource::Midi);
      bpmModeEnabled = (src == TriggerSource::Bpm);

      p.setMidiModeEnabled(midiModeEnabled);
      p.setBpmModeEnabled(bpmModeEnabled);

      settingsPanel.setTriggerSource(src);

      // 进入/退出模式时重置一次状态，避免跨模式“沿检测”残留
      wasIndicatorFlashing = false;
      isAnimating = false;
      animationProgress = 0.0f;
      stopHcrFrameAnimation();

      if (src == TriggerSource::Midi) {
          lastMidiTriggerSequence = p.getMidiTriggerSequence();
          midiFlashTimerSec = 0.0;
          midiHistoryPulseFramesRemaining = 0;

          settingsPanel.setThresholdChangedCallback({});
          settingsPanel.setThresholdDb(-60.0f);
          settingsPanel.setVolumeMeterMode(true);
          return;
      }

      if (src == TriggerSource::Bpm) {
          lastBpmTriggerSequence = p.getBpmTriggerSequence();
          bpmFlashTimerSec = 0.0;
          bpmHistoryPulseFramesRemaining = 0;

          settingsPanel.setThresholdChangedCallback({});
          settingsPanel.setThresholdDb(-60.0f);
          settingsPanel.setVolumeMeterMode(true);
          return;
      }

      // Audio模式
      settingsPanel.setThresholdChangedCallback([&p](float thresholdDb) {
          p.setTriggerThresholdDb(thresholdDb);
      });
      settingsPanel.setThresholdDb(p.getTriggerThresholdDb());
  };

  settingsPanel.setTriggerSourceChangedCallback([applyTriggerSource](TriggerSource src) {
      applyTriggerSource(src);
  });

  // 启动时根据宿主持久化状态恢复（BPM优先于MIDI，避免互斥状态冲突）
  if (p.getBpmModeEnabled()) {
      applyTriggerSource(TriggerSource::Bpm);
  } else if (p.getMidiModeEnabled()) {
      applyTriggerSource(TriggerSource::Midi);
  } else {
      applyTriggerSource(TriggerSource::Audio);
  }

  settingsPanel.setLevelCaptureWindowMs(p.getLevelCaptureWindowMs());
  settingsPanel.setLevelCaptureWindowChangedCallback([&p](float windowMs) {
      p.setLevelCaptureWindowMs(windowMs);
  });

  settingsPanel.setInputFilterFrequencies(p.getInputHighpassHz(),
                                          p.getInputLowpassHz());
  settingsPanel.setInputFilterChangedCallback(
      [&p](float highpassHz, float lowpassHz) {
          p.setInputFilterFrequencies(highpassHz, lowpassHz);
      });
  addChildComponent(settingsPanel); // 作为子组件添加，但不立即显示

  // 初始化动画状态：设置动画系统的初始值
  // isAnimating：动画是否正在进行中，false表示初始状态为静止
  isAnimating = false;
  // isMovingUp：当前运动方向，true表示向上运动，false表示向下运动
  isMovingUp = true;
  // animationProgress：动画进度，范围0.0到1.0，表示动画完成的比例
  animationProgress = 0.0f;
  // animationDuration：动画总持续时间（秒），根据指示灯亮起时间动态计算
  animationDuration = 0.0f;
  // startYPosition：动画起始Y轴位置（像素），从初始位置开始运动
  startYPosition = tremolo::defaults::jjAnimationStartYOffsetPx;
  // targetYPosition：动画目标Y轴位置（像素），向上移动到“最高点偏移”
  targetYPosition = tremolo::defaults::jjAnimationPeakYOffsetPx;

  // 设置Logo图片：从内存中加载Logo图片资源
  // logo.setImage(
  //     juce::ImageCache::getFromMemory(assets::Logo_png, assets::Logo_pngSize));
  // // 将Logo组件添加到界面
  // addAndMakeVisible(logo);

  // 定义侧边标签的字体颜色：使用JUCE的颜色系统（字的颜色）
  const auto sideFontColor = juce::Colour{0xFFC8C8C8};

  // // 设置指示灯标签
  // indicatorLabel.setJustificationType(juce::Justification::centred);
  // indicatorLabel.setMinimumHorizontalScale(1.f);
  // indicatorLabel.setFont(lookAndFeel.getSideLabelsFont());
  // indicatorLabel.setColour(juce::Label::textColourId, sideFontColor);
  // indicatorLabel.setText("PEAK", juce::dontSendNotification);
  // addAndMakeVisible(indicatorLabel);

  // 设置XY控制器的值变化回调函数
  xyController.setValueChangeCallback([this, &p](float x, float y) {
    // 更新音频处理器的X和Y参数值
    p.getParameterRefs().xValue.setValueNotifyingHost(x);
    p.getParameterRefs().yValue.setValueNotifyingHost(y);

    if constexpr (tremolo::defaults::showDebugGainOverlay) {
      repaint();
    }
  });

  // 从处理器参数同步一次XY控制器初始位置（支持宿主恢复状态）
  xyController.setValues(p.getParameterRefs().xValue.get(),
                         p.getParameterRefs().yValue.get(),
                         false);

  // 启动定时器用于更新指示灯状态（每秒30帧）
  startTimerHz(60);

  // 设置自定义外观：将lookAndFeel对象设置为当前组件的外观
  setLookAndFeel(&lookAndFeel);

  // 注释：编辑器大小在上方根据Background图片实际尺寸自动设置
  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
}

// PluginEditor类的析构函数：在对象销毁时自动调用
PluginEditor::~PluginEditor() {
  // 取消并等待WB sprite sheet异步加载线程结束（避免std::terminate）
  wbSpriteSheetLoadCancel.store(true);
  if (wbSpriteSheetLoadThread.joinable()) {
    wbSpriteSheetLoadThread.join();
  }

  // 移除自定义外观设置，恢复默认外观
  setLookAndFeel(nullptr);
}

void PluginEditor::setXYSkin(tremolo::defaults::XYSkinId newSkin) {
    // 第一次进入必须执行资源绑定；之后如果皮肤相同可早退
    if (xySkinInitialized && currentXYSkin == newSkin) {
        return;
    }

    currentXYSkin = newSkin;
    xySkinInitialized = true;

    // 切换皮肤时，停止所有皮肤相关动画，避免状态串台
    isAnimating = false;
    animationProgress = 0.0f;
    stopHcrFrameAnimation();
    stopGgggFrameAnimation();
    stopDsFrameAnimation();
    stopZszFrameAnimation();
    stopWbFrameAnimation();

    // 皮肤加载提示：默认隐藏；仅WB异步加载时显示
    skinLoadingOverlayVisible = false;

    // 如果正在异步加载WB资源，而这次切走WB，则取消加载
    if (newSkin != tremolo::defaults::XYSkinId::WB) {
        wbSpriteSheetLoadCancel.store(true);
    }

    // 统一恢复到“未闪烁”状态的基础底图
    if (currentXYSkin == tremolo::defaults::XYSkinId::BT) {
        const auto base = loadImageFromBinary(kBtBase);
        if (base.isValid()) {
            btImage.setImage(base);
        }

        const auto jj = loadImageFromBinary(kBtJj);
        if (jj.isValid()) {
            jjImage.setImage(jj);
        }
        jjClipper.setVisible(true);
        jjImage.setVisible(true);

        const auto tone = loadImageFromBinary(kBtTone);
        if (tone.isValid()) {
            toneImage.setImage(tone);
        }
    } else if (currentXYSkin == tremolo::defaults::XYSkinId::HCR) {
        const auto base = loadImageFromBinary(kHcrBase);
        if (base.isValid()) {
            btImage.setImage(base);
        }

        // HCR皮肤不使用jj上下往复动画
        jjImage.setVisible(false);
        jjClipper.setVisible(false);

        const auto tone = loadImageFromBinary(kHcrTone);
        if (tone.isValid()) {
            toneImage.setImage(tone);
        }
    } else if (currentXYSkin == tremolo::defaults::XYSkinId::GGGG) {
        // GGGG：使用图集第0帧作为底图
        if (!ggggSpriteSheet.isValid()) {
            ggggSpriteSheet = loadImageFromBinary(kGgggSpriteSheet);
        }

        stopGgggFrameAnimation();
        if (ggggSpriteSheet.isValid()) {
            setGgggFrameIndex(0);
        }

        // GGGG皮肤不使用jj上下往复动画
        jjImage.setVisible(false);
        jjClipper.setVisible(false);

        const auto tone = loadImageFromBinary(kGgggTone);
        if (tone.isValid()) {
            toneImage.setImage(tone);
        }
    } else if (currentXYSkin == tremolo::defaults::XYSkinId::DS) {
        // DS：使用图集第0帧作为底图
        dsTriggerProgramIndex = 0;

        if (!dsSpriteSheet.isValid()) {
            dsSpriteSheet = loadImageFromBinary(kDsSpriteSheet);
        }

        stopDsFrameAnimation();
        if (dsSpriteSheet.isValid()) {
            setDsFrameIndex(0);
        }

        // DS皮肤不使用jj上下往复动画
        jjImage.setVisible(false);
        jjClipper.setVisible(false);

        const auto tone = loadImageFromBinary(kDsTone);
        if (tone.isValid()) {
            toneImage.setImage(tone);
        }
    } else if (currentXYSkin == tremolo::defaults::XYSkinId::ZSZ) {
        // ZSZ：使用图集第0帧作为底图
        zszTriggerProgramIndex = 0;

        if (!zszSpriteSheet.isValid()) {
            zszSpriteSheet = loadImageFromBinary(kZszSpriteSheet);
        }

        stopZszFrameAnimation();
        if (zszSpriteSheet.isValid()) {
            setZszFrameIndex(0);
        }

        // ZSZ皮肤不使用jj上下往复动画
        jjImage.setVisible(false);
        jjClipper.setVisible(false);

        const auto tone = loadImageFromBinary(kZszTone);
        if (tone.isValid()) {
            toneImage.setImage(tone);
        }
    } else {
        // WB

        // 每次切到WB都从配置的第一段开始计数
        wbTriggerProgramIndex = 0;

        // 切换到WB时，使用异步加载避免界面卡顿，并显示loading提示。
        beginLoadWbSpriteSheetAsync();
        if (wbSpriteSheet.isValid()) {
            setWbFrameIndex(0);
        }

        // WB皮肤不使用jj上下往复动画
        jjImage.setVisible(false);
        jjClipper.setVisible(false);

        const auto tone = loadImageFromBinary(kWbTone);
        if (tone.isValid()) {
            toneImage.setImage(tone);
        }
    }

    // 强制刷新布局与绘制
    resized();
    repaint();
}

void PluginEditor::startHcrFrameAnimation() {
    // HCR：每次触发严格按 Defaults.h 的 hcrTriggerFrameProgram 播放（目前仅一段 0->21）
    if (!hcrSpriteSheet.isValid()) {
        hcrSpriteSheet = loadImageFromBinary(kHcrSpriteSheet);
        if (!hcrSpriteSheet.isValid()) {
            return;
        }
    }

    const auto& program = tremolo::defaults::hcrTriggerFrameProgram;
    const auto seg = program[0];

    const int from = seg.fromFrame;
    const int to = seg.toFrame;

    hcrTriggerStep = (from <= to) ? +1 : -1;
    hcrTriggerEndFrame = to;

    hcrFrameAnimActive = true;
    hcrFrameIndex = from;
    hcrFrameTimeAccSec = 0.0;

    setHcrFrameIndex(from);
}

void PluginEditor::stopHcrFrameAnimation() {
    hcrFrameAnimActive = false;
    hcrFrameTimeAccSec = 0.0;

    // 停留在段末帧，不在这里回到底图001.png
    hcrTriggerStep = +1;
    hcrTriggerEndFrame = hcrFrameIndex;
}

void PluginEditor::setHcrFrameIndex(int newIndex) {
    if (!hcrSpriteSheet.isValid()) {
        return;
    }

    // HCR图集：与XY区域一致（550×550），横向22帧
    constexpr int kHcrFrameWidthPx = 550;
    constexpr int kHcrFrameHeightPx = 550;
    constexpr int kHcrFrameCount = 22;

    hcrFrameIndex = juce::jlimit(0, kHcrFrameCount - 1, newIndex);

    const int x = hcrFrameIndex * kHcrFrameWidthPx;
    const auto clipped = hcrSpriteSheet.getClippedImage(
        juce::Rectangle<int>{x, 0, kHcrFrameWidthPx, kHcrFrameHeightPx});

    if (clipped.isValid()) {
        btImage.setImage(clipped);
    }
}

void PluginEditor::startGgggFrameAnimation() {
    // GGGG：每次触发严格按 Defaults.h 的 ggggTriggerFrameProgram 播放一段，并在段末帧停留
    if (!ggggSpriteSheet.isValid()) {
        ggggSpriteSheet = loadImageFromBinary(kGgggSpriteSheet);
        if (!ggggSpriteSheet.isValid()) {
            return;
        }
    }

    const auto& program = tremolo::defaults::ggggTriggerFrameProgram;
    if (program.empty()) {
        return;
    }

    constexpr int kGgggFrameCount = 25;

    const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, ggggTriggerProgramIndex);
    const auto seg = program[static_cast<size_t>(segIndex)];

    const int from = juce::jlimit(0, kGgggFrameCount - 1, seg.fromFrame);
    const int to = juce::jlimit(0, kGgggFrameCount - 1, seg.toFrame);

    ggggTriggerStep = (from <= to) ? +1 : -1;
    ggggTriggerEndFrame = to;

    ggggFrameAnimActive = true;
    ggggFrameIndex = from;
    ggggFrameTimeAccSec = 0.0;

    setGgggFrameIndex(from);

    // 下一次触发播放下一段
    ggggTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
}

void PluginEditor::stopGgggFrameAnimation() {
    ggggFrameAnimActive = false;
    ggggFrameTimeAccSec = 0.0;

    // 停留在段末帧，不在这里强制回到第0帧
    ggggTriggerStep = +1;
    ggggTriggerEndFrame = ggggFrameIndex;
}

void PluginEditor::setGgggFrameIndex(int newIndex) {
    if (!ggggSpriteSheet.isValid()) {
        return;
    }

    // GGGG图集：与XY区域一致（550×550），横向25帧
    constexpr int kGgggFrameWidthPx = 550;
    constexpr int kGgggFrameHeightPx = 550;
    constexpr int kGgggFrameCount = 25;

    ggggFrameIndex = juce::jlimit(0, kGgggFrameCount - 1, newIndex);

    const int x = ggggFrameIndex * kGgggFrameWidthPx;
    const auto clipped = ggggSpriteSheet.getClippedImage(
        juce::Rectangle<int>{x, 0, kGgggFrameWidthPx, kGgggFrameHeightPx});

    if (clipped.isValid()) {
        btImage.setImage(clipped);
    }
}

void PluginEditor::startDsFrameAnimation() {
    // DS：每次触发严格按 Defaults.h 的 dsTriggerFrameProgram 播放一段，并在段末帧停留
    if (!dsSpriteSheet.isValid()) {
        dsSpriteSheet = loadImageFromBinary(kDsSpriteSheet);
        if (!dsSpriteSheet.isValid()) {
            return;
        }
    }

    const auto& program = tremolo::defaults::dsTriggerFrameProgram;
    if (program.empty()) {
        return;
    }

    const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, dsTriggerProgramIndex);
    const auto seg = program[static_cast<size_t>(segIndex)];

    const int from = juce::jlimit(0, kDsFrameCount - 1, seg.fromFrame);
    const int to = juce::jlimit(0, kDsFrameCount - 1, seg.toFrame);

    dsTriggerStep = (from <= to) ? +1 : -1;
    dsTriggerEndFrame = to;

    dsFrameAnimActive = true;
    dsFrameIndex = from;
    dsFrameTimeAccSec = 0.0;

    setDsFrameIndex(from);

    // 下一次触发播放下一段
    dsTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
}

void PluginEditor::stopDsFrameAnimation() {
    dsFrameAnimActive = false;
    dsFrameTimeAccSec = 0.0;

    // 停留在段末帧，不在这里强制回到第0帧
    dsTriggerStep = +1;
    dsTriggerEndFrame = dsFrameIndex;
}

void PluginEditor::setDsFrameIndex(int newIndex) {
    if (!dsSpriteSheet.isValid()) {
        return;
    }

    dsFrameIndex = juce::jlimit(0, kDsFrameCount - 1, newIndex);

    const int x = dsFrameIndex * kDsFrameWidthPx;
    const auto clipped = dsSpriteSheet.getClippedImage(
        juce::Rectangle<int>{x, 0, kDsFrameWidthPx, kDsFrameHeightPx});

    if (clipped.isValid()) {
        btImage.setImage(clipped);
    }
}

void PluginEditor::startZszFrameAnimation() {
    // ZSZ：每次触发严格按 Defaults.h 的 zszTriggerFrameProgram 播放（0->5），并在段末帧停留
    if (!zszSpriteSheet.isValid()) {
        zszSpriteSheet = loadImageFromBinary(kZszSpriteSheet);
        if (!zszSpriteSheet.isValid()) {
            return;
        }
    }

    const auto& program = tremolo::defaults::zszTriggerFrameProgram;
    if (program.empty()) {
        return;
    }

    // 目前仅一段，但仍保留“program index”结构，方便未来扩展
    const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, zszTriggerProgramIndex);
    const auto seg = program[static_cast<size_t>(segIndex)];

    const int from = juce::jlimit(0, kZszFrameCount - 1, seg.fromFrame);
    const int to = juce::jlimit(0, kZszFrameCount - 1, seg.toFrame);

    zszTriggerStep = (from <= to) ? +1 : -1;
    zszTriggerEndFrame = to;

    zszFrameAnimActive = true;
    zszFrameIndex = from;
    zszFrameTimeAccSec = 0.0;

    setZszFrameIndex(from);

    // 下一次触发播放下一段（目前等价于一直为0）
    zszTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
}

void PluginEditor::stopZszFrameAnimation() {
    zszFrameAnimActive = false;
    zszFrameTimeAccSec = 0.0;

    // 停留在段末帧，不在这里强制回到第0帧
    zszTriggerStep = +1;
    zszTriggerEndFrame = zszFrameIndex;
}

void PluginEditor::setZszFrameIndex(int newIndex) {
    if (!zszSpriteSheet.isValid()) {
        return;
    }

    zszFrameIndex = juce::jlimit(0, kZszFrameCount - 1, newIndex);

    const int x = zszFrameIndex * kZszFrameWidthPx;
    const auto clipped = zszSpriteSheet.getClippedImage(
        juce::Rectangle<int>{x, 0, kZszFrameWidthPx, kZszFrameHeightPx});

    if (clipped.isValid()) {
        btImage.setImage(clipped);
    }
}

void PluginEditor::stopWbFrameAnimation() {
    wbFrameAnimActive = false;
    wbFramesRemaining = 0;
    wbFrameTimeAccSec = 0.0;

    // 本次分段播放结束后，保持停留在“段末帧”，不在这里改wbFrameIndex。
    wbTriggerStep = +1;
    wbTriggerEndFrame = wbFrameIndex;
}

void PluginEditor::setWbFrameIndex(int newIndex) {
    if (!wbSpriteSheet.isValid()) {
        return;
    }

    wbFrameIndex = juce::jlimit(0, kWbFrameCount - 1, newIndex);

    const int x = wbFrameIndex * kWbFrameWidthPx;
    const auto clipped = wbSpriteSheet.getClippedImage(
        juce::Rectangle<int>{x, 0, kWbFrameWidthPx, kWbFrameHeightPx});

    if (clipped.isValid()) {
        btImage.setImage(clipped);
    }
}

void PluginEditor::beginLoadWbSpriteSheetAsync() {
    // 已经有有效图片则无需加载
    if (wbSpriteSheet.isValid()) {
        skinLoadingOverlayVisible = false;
        return;
    }

    // 如果已有线程在跑，就不重复启动，但要确保loading提示可见
    if (wbSpriteSheetLoading.load()) {
        skinLoadingOverlayVisible = true;
        skinLoadingOverlayText = "Loading WB skin...";
        repaint();
        return;
    }

    wbSpriteSheetLoading.store(true);
    wbSpriteSheetLoadCancel.store(false);

    skinLoadingOverlayVisible = true;
    skinLoadingOverlayText = "Loading WB skin...";
    repaint();

    // 若之前线程对象还joinable（理论上不应发生），先join
    if (wbSpriteSheetLoadThread.joinable()) {
        wbSpriteSheetLoadCancel.store(true);
        wbSpriteSheetLoadThread.join();
        wbSpriteSheetLoadCancel.store(false);
    }

    wbSpriteSheetLoadThread = std::thread([this]() {
        // 注意：ImageCache::getFromMemory 通常线程安全，但具体依赖JUCE实现；这里仍尽量把UI更新放回消息线程。
        auto img = loadImageFromBinary(kWbSpriteSheet);

        if (wbSpriteSheetLoadCancel.load()) {
            return;
        }

        juce::MessageManager::callAsync([this, img]() mutable {
            if (wbSpriteSheetLoadCancel.load()) {
                return;
            }

            wbSpriteSheet = img;
            wbSpriteSheetLoading.store(false);
            skinLoadingOverlayVisible = false;

            if (wbSpriteSheet.isValid()) {
                // 切到WB后默认显示第0帧（或保持现有帧索引也可以，这里用0帧更直观）
                setWbFrameIndex(0);
            }

            resized();
            repaint();
        });
    });
}

void PluginEditor::updateXYSkinVisualsForIndicator(bool shouldFlash, bool retriggered, double dtSec) {

    // 边沿检测：首次亮起，或“重触发”时都视为一次新的触发
    const bool risingEdge = shouldFlash && (!wasIndicatorFlashing || retriggered);
    wasIndicatorFlashing = shouldFlash;

    if (currentXYSkin == tremolo::defaults::XYSkinId::BT) {
        // BT：使用jj.png上下往复动画
        if (shouldFlash && (!isAnimating || retriggered)) {
            auto& audioProcessor = dynamic_cast<PluginProcessor&>(processor);

            isAnimating = true;
            isMovingUp = true;
            animationProgress = 0.0f;

            float indicatorDuration = audioProcessor.getTremolo().getIndicatorDuration();
            animationDuration = indicatorDuration / 2.0f;

            startYPosition = tremolo::defaults::jjAnimationStartYOffsetPx;
            targetYPosition = tremolo::defaults::jjAnimationPeakYOffsetPx;

            const auto baseBounds = xyContainer.getLocalBounds()
                                       .withSizeKeepingCentre(
                                           juce::jmax(1, juce::roundToInt(51.0f * tremolo::defaults::jjImageScale)),
                                           juce::jmax(1, juce::roundToInt(325.0f * tremolo::defaults::jjImageScale)))
                                       .translated(0, 200);
            jjImage.setBounds(baseBounds.translated(
                0, -static_cast<int>(tremolo::defaults::jjAnimationStartYOffsetPx)));
        }

        updateAnimation();
        return;
    }

    if (currentXYSkin == tremolo::defaults::XYSkinId::HCR) {
        // HCR：底图001；触发时按图集播放 0->21（一轮），播完停留在最后一帧
        if (risingEdge) {
            startHcrFrameAnimation();
        }

        if (!shouldFlash) {
            // 未闪烁：始终显示底图001（这是默认皮肤，启动就应立即可见）
            const auto base = loadImageFromBinary(kHcrBase);
            if (base.isValid()) {
                btImage.setImage(base);
            }
            stopHcrFrameAnimation();
            return;
        }

        if (!hcrFrameAnimActive) {
            return;
        }

        const double frameDuration = juce::jmax(1.0e-6, tremolo::defaults::hcrIndicatorFrameDurationSec);
        hcrFrameTimeAccSec += dtSec;

        while (hcrFrameTimeAccSec >= frameDuration && hcrFrameAnimActive) {
            hcrFrameTimeAccSec -= frameDuration;

            if (hcrFrameIndex == hcrTriggerEndFrame) {
                stopHcrFrameAnimation();
                break;
            }

            const int prev = hcrFrameIndex;
            const int candidate = prev + hcrTriggerStep;

            // HCR图集：0..21
            const int next = juce::jlimit(0, 21, candidate);
            setHcrFrameIndex(next);

            if (next == prev || next == hcrTriggerEndFrame) {
                if (next == hcrTriggerEndFrame) {
                    setHcrFrameIndex(hcrTriggerEndFrame);
                }
                stopHcrFrameAnimation();
                break;
            }
        }

        return;
    }

    if (currentXYSkin == tremolo::defaults::XYSkinId::GGGG) {
        // GGGG：底图使用第0帧；触发时按 Defaults.h 的 ggggTriggerFrameProgram 分段播放，并停留在段末帧
        if (risingEdge) {
            startGgggFrameAnimation();
        }

        if (!shouldFlash) {
            // 未闪烁：不强制回到第0帧；保持停留在“上一次演出结束帧”
            stopGgggFrameAnimation();
            return;
        }

        if (!ggggFrameAnimActive) {
            return;
        }

        const double frameDuration = juce::jmax(1.0e-6, tremolo::defaults::ggggIndicatorFrameDurationSec);
        ggggFrameTimeAccSec += dtSec;

        while (ggggFrameTimeAccSec >= frameDuration && ggggFrameAnimActive) {
            ggggFrameTimeAccSec -= frameDuration;

            if (ggggFrameIndex == ggggTriggerEndFrame) {
                stopGgggFrameAnimation();
                break;
            }

            const int prev = ggggFrameIndex;
            const int candidate = prev + ggggTriggerStep;

            // GGGG图集：0..24
            const int next = juce::jlimit(0, 24, candidate);
            setGgggFrameIndex(next);

            if (next == prev || next == ggggTriggerEndFrame) {
                if (next == ggggTriggerEndFrame) {
                    setGgggFrameIndex(ggggTriggerEndFrame);
                }
                stopGgggFrameAnimation();
                break;
            }
        }

        return;
    }

    if (currentXYSkin == tremolo::defaults::XYSkinId::DS) {
        // DS：底图使用第0帧；触发时按 Defaults.h 的 dsTriggerFrameProgram 分段往复播放，并停留在段末帧
        if (risingEdge) {
            startDsFrameAnimation();
        }

        if (!shouldFlash) {
            // 未闪烁：不强制回到第0帧；保持停留在“上一次演出结束帧”
            stopDsFrameAnimation();
            return;
        }

        if (!dsFrameAnimActive) {
            return;
        }

        const double frameDuration = juce::jmax(1.0e-6, tremolo::defaults::dsIndicatorFrameDurationSec);
        dsFrameTimeAccSec += dtSec;

        while (dsFrameTimeAccSec >= frameDuration && dsFrameAnimActive) {
            dsFrameTimeAccSec -= frameDuration;

            if (dsFrameIndex == dsTriggerEndFrame) {
                stopDsFrameAnimation();
                break;
            }

            const int prev = dsFrameIndex;
            const int candidate = prev + dsTriggerStep;

            // DS图集：0..56
            const int next = juce::jlimit(0, kDsFrameCount - 1, candidate);
            setDsFrameIndex(next);

            if (next == prev || next == dsTriggerEndFrame) {
                if (next == dsTriggerEndFrame) {
                    setDsFrameIndex(dsTriggerEndFrame);
                }
                stopDsFrameAnimation();
                break;
            }
        }

        return;
    }

    if (currentXYSkin == tremolo::defaults::XYSkinId::ZSZ) {
        // ZSZ：底图使用第0帧；触发时按 Defaults.h 的 zszTriggerFrameProgram 从头播到尾（0->5）并停留
        if (risingEdge) {
            startZszFrameAnimation();
        }

        if (!shouldFlash) {
            // 未闪烁：不强制回到第0帧；保持停留在“上一次演出结束帧”
            stopZszFrameAnimation();
            return;
        }

        if (!zszFrameAnimActive) {
            return;
        }

        const double frameDuration = juce::jmax(1.0e-6, tremolo::defaults::zszIndicatorFrameDurationSec);
        zszFrameTimeAccSec += dtSec;

        while (zszFrameTimeAccSec >= frameDuration && zszFrameAnimActive) {
            zszFrameTimeAccSec -= frameDuration;

            if (zszFrameIndex == zszTriggerEndFrame) {
                stopZszFrameAnimation();
                break;
            }

            const int prev = zszFrameIndex;
            const int candidate = prev + zszTriggerStep;

            // ZSZ图集：0..5
            const int next = juce::jlimit(0, kZszFrameCount - 1, candidate);
            setZszFrameIndex(next);

            if (next == prev || next == zszTriggerEndFrame) {
                if (next == zszTriggerEndFrame) {
                    setZszFrameIndex(zszTriggerEndFrame);
                }
                stopZszFrameAnimation();
                break;
            }
        }

        return;
    }

    if (currentXYSkin == tremolo::defaults::XYSkinId::WB) {
        // WB：sprite sheet（1行×64列），每次触发按 Defaults.h 的 wbTriggerFrameProgram 播放一段，并停留在该段的最后一帧
        if (risingEdge) {
            // sprite sheet未准备好则触发异步加载，并保持loading提示
            if (!wbSpriteSheet.isValid()) {
                beginLoadWbSpriteSheetAsync();
                return;
            }

            const auto& program = tremolo::defaults::wbTriggerFrameProgram;
            if (program.empty()) {
                return;
            }

            const int segIndex = juce::jlimit(0, static_cast<int>(program.size()) - 1, wbTriggerProgramIndex);
            const auto seg = program[static_cast<size_t>(segIndex)];

            const int from = juce::jlimit(0, kWbFrameCount - 1, seg.fromFrame);
            const int to = juce::jlimit(0, kWbFrameCount - 1, seg.toFrame);

            wbTriggerStep = (from <= to) ? +1 : -1;
            wbTriggerEndFrame = to;

            // 每次触发严格从配置的起始帧开始播放
            setWbFrameIndex(from);

            wbFrameAnimActive = true;
            wbFramesRemaining = std::numeric_limits<int>::max();
            wbFrameTimeAccSec = 0.0;

            // 下一次触发播放下一段
            wbTriggerProgramIndex = (segIndex + 1) % static_cast<int>(program.size());
        }

        if (!wbFrameAnimActive) {
            return;
        }

        const double frameDuration = juce::jmax(1.0e-6, tremolo::defaults::wbIndicatorFrameDurationSec);
        wbFrameTimeAccSec += dtSec;

        while (wbFrameTimeAccSec >= frameDuration && wbFrameAnimActive) {
            wbFrameTimeAccSec -= frameDuration;

            // 到达段末帧：停留并停止（完全按照配置中一段动画的最后一帧）
            if (wbFrameIndex == wbTriggerEndFrame) {
                stopWbFrameAnimation();
                break;
            }

            const int prev = wbFrameIndex;
            const int candidate = prev + wbTriggerStep;
            // 安全夹紧：避免配置越界导致崩溃
            const int next = juce::jlimit(0, kWbFrameCount - 1, candidate);

            setWbFrameIndex(next);

            // 如果因为夹紧导致无法继续前进，也停止
            if (next == prev || next == wbTriggerEndFrame) {
                if (next == wbTriggerEndFrame) {
                    // 确保最后一帧已显示
                    setWbFrameIndex(wbTriggerEndFrame);
                }
                stopWbFrameAnimation();
                break;
            }
        }

        return;
    }

}

// timerCallback方法：定时器回调函数，每秒调用30次（30fps）
// 功能：更新指示灯状态、检测动画触发条件、管理动画生命周期
// 调用机制：由JUCE框架自动调用，频率由startTimerHz(30)设置
void PluginEditor::timerCallback() {
    // 获取音频处理器引用，用于访问Tremolo效果器的状态信息
    // dynamic_cast：安全类型转换，确保processor确实是PluginProcessor类型
    auto& audioProcessor = dynamic_cast<PluginProcessor&>(processor);

    // 同步XY控制器UI到当前参数值（避免宿主自动化/恢复后UI停留在默认值）
    const auto xParam = audioProcessor.getParameterRefs().xValue.get();
    const auto yParam = audioProcessor.getParameterRefs().yValue.get();
    if (!juce::approximatelyEqual(xParam, xyController.getXValue()) ||
        !juce::approximatelyEqual(yParam, xyController.getYValue())) {
        xyController.setValues(xParam, yParam, false);

        if constexpr (tremolo::defaults::showDebugGainOverlay) {
          repaint();
        }
    }

    // 根据XY控制器数值刷新tone控制点位置
    {
        const auto area = xyContainer.getLocalBounds().toFloat();
        const float halfW = tremolo::defaults::xyToneMarkerWidthPx * 0.5f;
        const float halfH = tremolo::defaults::xyToneMarkerHeightPx * 0.5f;

        const float minX = area.getX() + halfW;
        const float maxX = area.getRight() - halfW;
        const float minY = area.getY() + halfH;
        const float maxY = area.getBottom() - halfH;

        const float xPos = minX + xyController.getXValue() * juce::jmax(1.0f, maxX - minX);
        const float yPos = minY + xyController.getYValue() * juce::jmax(1.0f, maxY - minY);

        toneImage.setBounds(juce::Rectangle<int>{
            static_cast<int>(std::round(xPos - halfW)),
            static_cast<int>(std::round(yPos - halfH)),
            tremolo::defaults::xyToneMarkerWidthPx,
            tremolo::defaults::xyToneMarkerHeightPx});
    }
    
    constexpr double dtSec = 1.0 / 60.0;

    // 将一些参数实时推送给设置面板（无论哪种模式都需要同步这些UI）
    settingsPanel.setLevelCaptureWindowMs(audioProcessor.getLevelCaptureWindowMs());
    settingsPanel.setInputFilterFrequencies(audioProcessor.getInputHighpassHz(),
                                            audioProcessor.getInputLowpassHz());

    // BPM触发模式：指示灯与XY动画由宿主BPM驱动
    if (bpmModeEnabled) {
        const auto bpmSeq = audioProcessor.getBpmTriggerSequence();
        const bool retriggered = (bpmSeq != lastBpmTriggerSequence);
        if (retriggered) {
            lastBpmTriggerSequence = bpmSeq;
            bpmFlashTimerSec = tremolo::defaults::indicatorFlashDurationSecDefault;
            bpmHistoryPulseFramesRemaining = 3; // 约50ms脉冲，便于在历史图里看清
        }

        bpmFlashTimerSec = juce::jmax(0.0, bpmFlashTimerSec - dtSec);
        const bool shouldFlash = bpmFlashTimerSec > 0.0;

        const bool pulse = (bpmHistoryPulseFramesRemaining > 0);
        settingsPanel.updateVolumeLevel(pulse ? 1.0f : 0.0f);
        if (bpmHistoryPulseFramesRemaining > 0) {
            --bpmHistoryPulseFramesRemaining;
        }

        indicatorLight.setFlashing(retriggered ? false : shouldFlash);
        updateXYSkinVisualsForIndicator(shouldFlash, retriggered, dtSec);
        return;
    }

    // MIDI触发模式：指示灯与XY动画由MIDI输入驱动；电平图显示MIDI输入历史
    if (midiModeEnabled) {
        const auto midiSeq = audioProcessor.getMidiTriggerSequence();
        const bool retriggered = (midiSeq != lastMidiTriggerSequence);
        if (retriggered) {
            lastMidiTriggerSequence = midiSeq;
            midiFlashTimerSec = tremolo::defaults::indicatorFlashDurationSecDefault;
            midiHistoryPulseFramesRemaining = 3; // 约50ms脉冲，便于在历史图里看清
        }

        midiFlashTimerSec = juce::jmax(0.0, midiFlashTimerSec - dtSec);
        const bool shouldFlash = midiFlashTimerSec > 0.0;

        // 电平历史：用0/1脉冲表示“本帧附近是否有MIDI进入”
        const bool pulse = (midiHistoryPulseFramesRemaining > 0);
        settingsPanel.updateVolumeLevel(pulse ? 1.0f : 0.0f);
        if (midiHistoryPulseFramesRemaining > 0) {
            --midiHistoryPulseFramesRemaining;
        }

        indicatorLight.setFlashing(retriggered ? false : shouldFlash);
        updateXYSkinVisualsForIndicator(shouldFlash, retriggered, dtSec);
        return;
    }

    // 更新指示灯状态：传入时间增量（1/60秒）
    // 指示灯根据音频信号的峰值决定是否闪烁
    auto& trem = audioProcessor.getTremolo();
    trem.updateIndicatorState(static_cast<float>(dtSec));

    // 读取“阈值触发序列号”，用于识别快速连续触发
    const auto triggerSeq = trem.getIndicatorTriggerSequence();
    const bool retriggered = (triggerSeq != lastIndicatorTriggerSequence);
    if (retriggered) {
        lastIndicatorTriggerSequence = triggerSeq;
    }

    // 获取指示灯当前是否应该闪烁的状态
    bool shouldFlash = trem.shouldFlashIndicator();

    // 将输入信号实时推送给设置面板中的滚动电平窗。
    settingsPanel.updateVolumeLevel(audioProcessor.getLatestInputLevel());
    settingsPanel.setThresholdDb(audioProcessor.getTriggerThresholdDb());
    settingsPanel.setLevelCaptureWindowMs(audioProcessor.getLevelCaptureWindowMs());
    settingsPanel.setInputFilterFrequencies(audioProcessor.getInputHighpassHz(),
                                            audioProcessor.getInputLowpassHz());

    // 设置指示灯组件的闪烁状态
    // 如果发生“重触发”，则本帧强制熄灭一次，下一帧会继续亮起，形成“快速灭亮一次”的提示
    indicatorLight.setFlashing(retriggered ? false : shouldFlash);
    
    // 根据当前皮肤，驱动XY区域的“指示灯联动动画”
    updateXYSkinVisualsForIndicator(shouldFlash, retriggered, dtSec);
}

// paint方法：绘制编辑器背景
void PluginEditor::paint(juce::Graphics& g) {
    // 调用基类的paint方法
    AudioProcessorEditor::paint(g);
}

void PluginEditor::paintOverChildren(juce::Graphics& g) {
    // 1) 调试增益Overlay（可选）
    if constexpr (tremolo::defaults::showDebugGainOverlay) {
        auto& audioProcessor = dynamic_cast<PluginProcessor&>(processor);
        const float x = audioProcessor.getParameterRefs().xValue.get();
        const float y = audioProcessor.getParameterRefs().yValue.get();
        const float gainBoost = computeGainBoostForXY(x, y);
        const float sawWet = computeSawWetForXY(x, y);

        const auto gainText = juce::String{"GAIN x"} + juce::String{gainBoost, 2};
        const auto wetText = juce::String{"WET "} + juce::String{sawWet * 100.0f, 1} + "%";

        auto area = getLocalBounds().toFloat().reduced(8.0f);
        auto box = area.removeFromTop(44.0f).removeFromRight(140.0f);

        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRoundedRectangle(box, 6.0f);

        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.setFont(juce::Font(13.0f, juce::Font::bold));

        auto line1 = box;
        auto line2 = line1.removeFromBottom(line1.getHeight() * 0.5f);
        line1 = line1.removeFromTop(line1.getHeight());

        g.drawFittedText(gainText, line1.toNearestInt(), juce::Justification::centred, 1);
        g.drawFittedText(wetText, line2.toNearestInt(), juce::Justification::centred, 1);
    }

    // 2) 皮肤加载提示（居中显示）
    if (!skinLoadingOverlayVisible) {
        return;
    }

    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRect(bounds);

    auto panel = bounds.withSizeKeepingCentre(260.0f, 80.0f);
    g.setColour(juce::Colours::black.withAlpha(0.65f));
    g.fillRoundedRectangle(panel, 10.0f);

    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawFittedText(skinLoadingOverlayText, panel.toNearestInt(), juce::Justification::centred, 2);
}

// resized方法：当组件大小改变时自动调用，用于重新布局子组件
void PluginEditor::resized() {

  // 获取组件的本地边界（相对于父组件的坐标和大小）
  auto bounds = getLocalBounds();

  // 设置背景图片覆盖整个边界
  background.setBounds(bounds);

  // 设置设置按钮的位置：固定长宽，通过左上角偏移确定位置
  settingsButton.setBounds(tremolo::defaults::settingsButtonLeftPx,
                           tremolo::defaults::settingsButtonTopPx,
                           tremolo::defaults::settingsButtonWidthPx,
                           tremolo::defaults::settingsButtonHeightPx);

  // skins按钮位置：在设置按钮右侧
  skinsButton.setBounds(tremolo::defaults::skinsButtonLeftPx,
                        tremolo::defaults::skinsButtonTopPx,
                        tremolo::defaults::skinsButtonWidthPx,
                        tremolo::defaults::skinsButtonHeightPx);

  // 设置设置面板的位置：覆盖整个界面，但留出边距
  auto settingsPanelBounds = bounds.reduced(50);
  settingsPanel.setBounds(settingsPanelBounds);

  // // 设置Logo的位置和大小：左上角(16,16)，宽105，高24
  // logo.setBounds({16, 16, 105, 24});

  // 计算增益控制区域的边界：顶部区域，高度80像素
  auto gainArea = bounds.removeFromTop(40);

  // XY区域：固定长宽，通过左上角偏移确定位置
  xyContainer.setBounds(tremolo::defaults::xyControllerLeftPx,
                        tremolo::defaults::xyControllerTopPx,
                        tremolo::defaults::xyControllerWidthPx,
                        tremolo::defaults::xyControllerHeightPx);

  // XY区域内部子组件布局
  btImage.setBounds(xyContainer.getLocalBounds());
  jjClipper.setBounds(xyContainer.getLocalBounds());
  xyController.setBounds(xyContainer.getLocalBounds());

  // 设置指示灯：固定长宽，通过左上角偏移确定位置
  indicatorLight.setBounds(tremolo::defaults::indicatorLightLeftPx,
                           tremolo::defaults::indicatorLightTopPx,
                           tremolo::defaults::indicatorLightWidthPx,
                           tremolo::defaults::indicatorLightHeightPx);

  // tone控制点位置
  {
      const auto area = xyContainer.getLocalBounds().toFloat();
      const float halfW = tremolo::defaults::xyToneMarkerWidthPx * 0.5f;
      const float halfH = tremolo::defaults::xyToneMarkerHeightPx * 0.5f;

      const float minX = area.getX() + halfW;
      const float maxX = area.getRight() - halfW;
      const float minY = area.getY() + halfH;
      const float maxY = area.getBottom() - halfH;

      const float xPos = minX + xyController.getXValue() * juce::jmax(1.0f, maxX - minX);
      const float yPos = minY + xyController.getYValue() * juce::jmax(1.0f, maxY - minY);

      toneImage.setBounds(juce::Rectangle<int>{
          static_cast<int>(std::round(xPos - halfW)),
          static_cast<int>(std::round(yPos - halfH)),
          tremolo::defaults::xyToneMarkerWidthPx,
          tremolo::defaults::xyToneMarkerHeightPx});
  }

  // jj.png 基础位置（未动画时保持静止）
  if (!isAnimating) {
      const auto baseBounds = xyContainer.getLocalBounds()
                                 .withSizeKeepingCentre(
                                     juce::jmax(1, juce::roundToInt(51.0f * tremolo::defaults::jjImageScale)),
                                     juce::jmax(1, juce::roundToInt(325.0f * tremolo::defaults::jjImageScale)))
                                 .translated(0, 200);
      jjImage.setBounds(baseBounds.translated(
          0, -static_cast<int>(tremolo::defaults::jjAnimationStartYOffsetPx)));
  }

  // 确保整体层级：背景最底，设置按钮/指示灯在上，XY区域最后绘制
  background.toBack();
  settingsButton.toFront(false);
  indicatorLight.toFront(false);
  xyContainer.toFront(false);

  // 设置面板如果可见，永远在最顶层
  if (isSettingsPanelVisible) {
      settingsPanel.toFront(false);
  }
}

// 缓动函数：向上运动（先快后慢）
// 参数t：动画进度，范围0.0到1.0，表示动画完成的比例
// 返回值：缓动后的进度值，用于计算平滑的运动效果
// 数学原理：二次贝塞尔曲线，t<0.5时加速，t>=0.5时减速
float PluginEditor::easeInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

// 缓动函数：向下运动（由慢变快）
// 参数t：动画进度，范围0.0到1.0，表示动画完成的比例
// 返回值：缓动后的进度值，用于计算平滑的运动效果
// 数学原理：三次贝塞尔曲线，t<0.5时缓慢开始，t>=0.5时快速结束
float PluginEditor::easeOutInQuad(float t) {
    return t < 0.5f ? 0.5f * (1.0f - std::pow(1.0f - 2.0f * t, 3.0f)) : 
                     0.5f * (1.0f + std::pow(2.0f * t - 1.0f, 3.0f));
}

// 动画更新函数：管理jj.png图片的上下运动动画
// 功能：根据动画状态更新图片位置，实现平滑的上下运动效果
// 调用时机：由定时器每秒调用30次，确保动画流畅
void PluginEditor::updateAnimation() {
    // 检查动画是否正在进行，如果未激活则直接返回
    if (!isAnimating) return;

    // 计算动画进度：每次调用增加1/30秒的进度（假设30fps）
    // animationDuration：动画总持续时间（秒）
    // 60.0f：假设60fps的更新频率，确保动画速度准确
    animationProgress += 1.0f / (animationDuration * 60.0f); // 30fps
    
    // 检查动画是否完成（进度达到或超过1.0）
    if (animationProgress >= 1.0f) {
        // 动画完成，根据当前运动方向决定下一步动作
        if (isMovingUp) {
            // 向上运动完成，切换到向下运动状态
            isMovingUp = false;
            animationProgress = 0.0f; // 重置进度
            startYPosition = tremolo::defaults::jjAnimationPeakYOffsetPx; // 当前在最高点
            targetYPosition = tremolo::defaults::jjAnimationStartYOffsetPx; // 目标位置：归位到起始位置
        } else {
            // 向下运动完成，停止整个动画过程
            isAnimating = false;
            animationProgress = 0.0f;
            
            // 重置运动参数，确保下次动画从正确的初始位置开始
            startYPosition = tremolo::defaults::jjAnimationStartYOffsetPx;
            targetYPosition = tremolo::defaults::jjAnimationPeakYOffsetPx;
            
            // 强制设置图片回到初始位置，确保归位准确
            const auto baseBounds = xyContainer.getLocalBounds()
                                       .withSizeKeepingCentre(
                                           juce::jmax(1, juce::roundToInt(51.0f * tremolo::defaults::jjImageScale)),
                                           juce::jmax(1, juce::roundToInt(325.0f * tremolo::defaults::jjImageScale)))
                                       .translated(0, 200);
            jjImage.setBounds(baseBounds.translated(
                0, -static_cast<int>(tremolo::defaults::jjAnimationStartYOffsetPx)));
            return; // 直接返回，不再执行后续位置计算

        }
    }

    const auto baseBounds = xyContainer.getLocalBounds()
                               .withSizeKeepingCentre(
                                   juce::jmax(1, juce::roundToInt(51.0f * tremolo::defaults::jjImageScale)),
                                   juce::jmax(1, juce::roundToInt(325.0f * tremolo::defaults::jjImageScale)))
                               .translated(0, 200);

    float currentYOffset = 0.0f;
    if (isMovingUp) {
        // 向上运动阶段：使用先快后慢的缓动函数
        float easedProgress = easeInOutQuad(animationProgress);
        currentYOffset = startYPosition + (targetYPosition - startYPosition) * easedProgress;
    } else {
        // 向下运动阶段：使用由慢变快的缓动函数
        float easedProgress = easeOutInQuad(animationProgress);
        currentYOffset = startYPosition + (targetYPosition - startYPosition) * easedProgress;
    }
    
    // 应用计算出的Y轴偏移量，设置图片最终位置
    auto jjImageBounds = baseBounds.translated(0, static_cast<int>(-currentYOffset));
    jjImage.setBounds(jjImageBounds);
}

// 命名空间结束
}  // namespace tremolo