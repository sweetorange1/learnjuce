#include "../include/Tremolo/PluginEditor.h"
#include "../include/Tremolo/Defaults.h"
#include <TremoloPluginAssets.h>

#include <array>

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

// HCR：001为底图；002-006为指示灯闪烁时的逐帧动画
const BinaryImage kHcrBase{assets::_001_png, assets::_001_pngSize};
const std::array<BinaryImage, 5> kHcrFlashFrames{{
    {assets::_002_png, assets::_002_pngSize},
    {assets::_003_png, assets::_003_pngSize},
    {assets::_004_png, assets::_004_pngSize},
    {assets::_005_png, assets::_005_pngSize},
    {assets::_006_png, assets::_006_pngSize},
}};
const BinaryImage kHcrTone{assets::tone_png2, assets::tone_png2Size};
}  // namespace

// SettingsPanel类的实现
SettingsPanel::SettingsPanel() {
    // 设置标题标签
    titleLabel.setText("Settings", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE6E6E6));
    addAndMakeVisible(titleLabel);

    volumeLabel.setText("Input Level", juce::dontSendNotification);
    volumeLabel.setJustificationType(juce::Justification::centredLeft);
    volumeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFD0D0D0));
    addAndMakeVisible(volumeLabel);

    waveformToggle.setButtonText("Scrolling Curve");
    waveformToggle.setToggleState(true, juce::dontSendNotification);
    waveformToggle.onClick = [this]() {
        setVolumeMeterMode(waveformToggle.getToggleState());
    };
    addAndMakeVisible(waveformToggle);

    volumeMeter.setDisplayMode(true);
    addAndMakeVisible(volumeMeter);

    levelWindowLabel.setText("Level Window", juce::dontSendNotification);
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
      // gainAttachment：将增益参数与控制条绑定
      gainAttachment{p.getParameterRefs().gain, gainSlider},
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

  // 设置增益标签
  gainLabel.setJustificationType(juce::Justification::centred);
  gainLabel.setMinimumHorizontalScale(1.f);
  gainLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(14.0f)));
  gainLabel.setColour(juce::Label::textColourId, sideFontColor);
  gainLabel.setText("MAX GAIN", juce::dontSendNotification); // 明确表示是最大增益
  addAndMakeVisible(gainLabel);

  // 设置增益控制条
  gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  gainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
  gainSlider.setRange(0.1, 10.0, 0.1); // 增益范围从0.1到10.0，步进0.1
  addAndMakeVisible(gainSlider);

  // // 设置指示灯标签
  // indicatorLabel.setJustificationType(juce::Justification::centred);
  // indicatorLabel.setMinimumHorizontalScale(1.f);
  // indicatorLabel.setFont(lookAndFeel.getSideLabelsFont());
  // indicatorLabel.setColour(juce::Label::textColourId, sideFontColor);
  // indicatorLabel.setText("PEAK", juce::dontSendNotification);
  // addAndMakeVisible(indicatorLabel);

  // 绑定增益参数（在成员初始化列表中初始化）

  // 设置XY控制器的值变化回调函数
  xyController.setValueChangeCallback([&p](float x, float y) {
    // 更新音频处理器的X和Y参数值
    p.getParameterRefs().xValue.setValueNotifyingHost(x);
    p.getParameterRefs().yValue.setValueNotifyingHost(y);
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
    } else {
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
    }

    // 强制刷新布局与绘制
    resized();
    repaint();
}

void PluginEditor::startHcrFrameAnimation() {
    hcrFrameAnimActive = true;
    hcrFrameIndex = 0;
    hcrFrameTimeAccSec = 0.0;

    const auto frame = loadImageFromBinary(kHcrFlashFrames[0]);
    if (frame.isValid()) {
        btImage.setImage(frame);
    }
}

void PluginEditor::stopHcrFrameAnimation() {
    hcrFrameAnimActive = false;
    hcrFrameIndex = 0;
    hcrFrameTimeAccSec = 0.0;
}

void PluginEditor::updateXYSkinVisualsForIndicator(bool shouldFlash, double dtSec) {
    // 边沿检测：只在“本次闪烁开始”触发一次
    const bool risingEdge = shouldFlash && !wasIndicatorFlashing;
    wasIndicatorFlashing = shouldFlash;

    if (currentXYSkin == tremolo::defaults::XYSkinId::BT) {
        // BT：使用jj.png上下往复动画
        if (shouldFlash && !isAnimating) {
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

    // HCR：底图001，闪烁时按帧率顺序显示002..006
    if (risingEdge) {
        startHcrFrameAnimation();
    }

    if (!shouldFlash) {
        // 闪烁结束：恢复到底图001
        if (hcrFrameAnimActive) {
            const auto base = loadImageFromBinary(kHcrBase);
            if (base.isValid()) {
                btImage.setImage(base);
            }
            stopHcrFrameAnimation();
        }
        return;
    }

    if (!hcrFrameAnimActive) {
        return;
    }

    const auto fps = juce::jmax(1.0, static_cast<double>(tremolo::defaults::hcrIndicatorAnimFps));
    const double frameDuration = 1.0 / fps;
    hcrFrameTimeAccSec += dtSec;

    // 一次闪烁只播放一轮（002->006），播完后停在最后一帧
    while (hcrFrameTimeAccSec >= frameDuration && hcrFrameIndex < static_cast<int>(kHcrFlashFrames.size()) - 1) {
        hcrFrameTimeAccSec -= frameDuration;
        ++hcrFrameIndex;

        const auto frame = loadImageFromBinary(kHcrFlashFrames[static_cast<size_t>(hcrFrameIndex)]);
        if (frame.isValid()) {
            btImage.setImage(frame);
        }
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
    
    // 更新指示灯状态：传入时间增量（1/60秒）
    // 指示灯根据音频信号的峰值决定是否闪烁
    audioProcessor.getTremolo().updateIndicatorState(1.0f / 60.0f);

    // 获取指示灯当前是否应该闪烁的状态
    bool shouldFlash = audioProcessor.getTremolo().shouldFlashIndicator();

    // 将输入信号实时推送给设置面板中的滚动电平窗。
    settingsPanel.updateVolumeLevel(audioProcessor.getLatestInputLevel());
    settingsPanel.setThresholdDb(audioProcessor.getTriggerThresholdDb());
    settingsPanel.setLevelCaptureWindowMs(audioProcessor.getLevelCaptureWindowMs());
    settingsPanel.setInputFilterFrequencies(audioProcessor.getInputHighpassHz(),
                                            audioProcessor.getInputLowpassHz());

    // 设置指示灯组件的闪烁状态
    indicatorLight.setFlashing(shouldFlash);
    
    // 根据当前皮肤，驱动XY区域的“指示灯联动动画”
    updateXYSkinVisualsForIndicator(shouldFlash, 1.0 / 60.0);
}

// paint方法：绘制编辑器背景
void PluginEditor::paint(juce::Graphics& g) {
    // 调用基类的paint方法
    AudioProcessorEditor::paint(g);
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