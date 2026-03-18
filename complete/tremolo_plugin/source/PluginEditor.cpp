// tremolo命名空间：C++中使用命名空间来组织代码，避免命名冲突
namespace tremolo {

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
    
    // 计算X和Y值（范围0.0到1.0）
    xValue = juce::jlimit(0.0f, 1.0f, position.x / bounds.getWidth());
    yValue = juce::jlimit(0.0f, 1.0f, position.y / bounds.getHeight());
    
    // 触发重绘
    repaint();
    
    // 如果有回调函数，调用它
    if (valueChangeCallback) {
        valueChangeCallback(xValue, yValue);
    }
}

// 绘制XY控制器
void XYController::paint(juce::Graphics& g) {
    // 获取组件边界
    auto bounds = getLocalBounds().toFloat();
    
    // 绘制背景（浅灰色网格）
    g.setColour(juce::Colour(0x00000000));
    g.fillRect(bounds);
    
    // // 绘制网格线
    // g.setColour(juce::Colour(0xFF555555));
    //
    // // 水平网格线
    // for (int i = 1; i < 4; ++i) {
    //     float y = bounds.getHeight() * i / 4.0f;
    //     g.drawLine(0.0f, y, bounds.getWidth(), y, 1.0f);
    // }
    //
    // // 垂直网格线
    // for (int i = 1; i < 4; ++i) {
    //     float x = bounds.getWidth() * i / 4.0f;
    //     g.drawLine(x, 0.0f, x, bounds.getHeight(), 1.0f);
    // }
    //
    // 绘制边框
    g.setColour(juce::Colour(0xFF888888));
    g.drawRect(bounds, 2.0f);
    
    // 目标点坐标（与Tremolo.h中保持一致）
    constexpr float targetX = 0.5f;
    constexpr float targetY = 0.38f;
    
    // 绘制目标点标记（绿色十字）
    float targetXPos = targetX * bounds.getWidth();
    float targetYPos = targetY * bounds.getHeight();
    g.setColour(juce::Colour(0xFF9A9A9A));
    g.drawLine(targetXPos - 8.0f, targetYPos, targetXPos + 8.0f, targetYPos, 2.0f);
    g.drawLine(targetXPos, targetYPos - 8.0f, targetXPos, targetYPos + 8.0f, 2.0f);
    
    // 计算当前位置
    float xPos = xValue * bounds.getWidth();
    float yPos = yValue * bounds.getHeight();
    
    // 绘制当前位置指示器（白色圆点）
    g.setColour(juce::Colour(0xFFE6E6E6));
    g.fillEllipse(xPos - 5.0f, yPos - 5.0f, 10.0f, 10.0f);
    
    // 绘制指示器边框
    g.setColour(juce::Colours::black);
    g.drawEllipse(xPos - 5.0f, yPos - 5.0f, 10.0f, 10.0f, 2.0f);
    
    // 计算距离和增益信息
    const auto distanceToTarget = std::sqrt(std::pow(xValue - targetX, 2.0f) + std::pow(yValue - targetY, 2.0f));
    const auto gainBoost = juce::jmap(distanceToTarget, 0.0f, std::sqrt(0.5f), 4.0f, 1.0f);
    
    // 在右下角显示X和Y值以及增益信息
    juce::String valueText = juce::String("X: ") + juce::String(xValue, 2) + 
                            juce::String(" Y: ") + juce::String(yValue, 2) +
                            juce::String("\nGain: ") + juce::String(gainBoost, 2) + "x";
    
    // 设置字体和颜色
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(14.0f)));
    g.setColour(juce::Colour(0xFFE6E6E6));
    
    // 计算文本位置（右下角，留出边距）
    auto textBounds = bounds.withTrimmedRight(10).withTrimmedBottom(10);
    g.drawText(valueText, textBounds, juce::Justification::bottomRight, true);
}

// PluginEditor类的构造函数：这是创建插件编辑器界面的入口点
// AudioProcessorEditor(&p)：继承自JUCE框架的音频处理器编辑器基类
// 参数p是PluginProcessor的引用，表示这个编辑器对应的音频处理器
PluginEditor::PluginEditor(PluginProcessor& p)
    // 初始化列表：C++中用于初始化成员变量的高效方式
    // AudioProcessorEditor(&p)：调用基类构造函数，传入音频处理器指针
    : AudioProcessorEditor(&p),
      // bypassAttachment：将旁路参数与按钮绑定
      bypassAttachment{p.getParameterRefs().bypassed, bypassButton},
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

  // 设置jj.png图片：从内存中加载jj图片资源（BinaryData），避免依赖开发机磁盘路径
  const auto jjImg = juce::ImageCache::getFromMemory(assets::jj_png, assets::jj_pngSize);
  if (jjImg.isValid()) {
      jjImage.setImage(jjImg);
  }
  // 在第一次指示灯亮起前，将jj图片设置为隐藏状态
  jjImage.setVisible(false);
  // 将jj图片组件添加到界面
  addAndMakeVisible(jjImage);

  // 设置设置按钮：从内存中加载设置图标（BinaryData），避免依赖开发机磁盘路径
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

  // 设置旁路标签的对齐方式为左对齐
  // bypassLabel.setJustificationType(juce::Justification::left);
  // 设置最小水平缩放比例
  // bypassLabel.setMinimumHorizontalScale(1.f);
  // 设置旁路标签的字体
  // bypassLabel.setFont(lookAndFeel.getSideLabelsFont());
  // 设置旁路标签的文本颜色
  // bypassLabel.setColour(juce::Label::textColourId, sideFontColor);
  // 将旁路标签添加到界面
  // addAndMakeVisible(bypassLabel);

  // 设置旁路按钮的点击事件处理函数（使用lambda表达式）
  bypassButton.onClick = [this]() {
    // 根据按钮的切换状态设置按钮文本
    bypassButton.setButtonText(bypassButton.getToggleState() ? "Bypassed"
                                                             : "Off");
  };
  // 立即执行一次点击事件，确保初始状态正确
  bypassButton.onClick();
  // 将旁路按钮添加到界面
  // addAndMakeVisible(bypassButton);

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

  // 设置指示灯组件
  indicatorLight.setInterceptsMouseClicks(false, false); // 不接收鼠标事件
  addAndMakeVisible(indicatorLight);

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
  
  // 将XY控制器添加到界面
  addAndMakeVisible(xyController);

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
    
  // 动画触发逻辑：当指示灯亮起且当前没有动画运行时，开始新动画
  if (shouldFlash && !isAnimating) {
      // 如果是第一次指示灯亮起，显示图片并重置状态
      if (isFirstIndicatorFlash) {
          jjImage.setVisible(true); // 显示图片
          isFirstIndicatorFlash = false; // 标记为已显示过
      }
      
      // 设置动画状态标志
      isAnimating = true;
      isMovingUp = true; // 初始运动方向：向上
      animationProgress = 0.0f; // 重置动画进度
      
      // 计算动画持续时间：单程运动时间是指示灯亮起时间的一半
      // 这样确保动画在指示灯熄灭前完成往返运动
      float indicatorDuration = audioProcessor.getTremolo().getIndicatorDuration();
      animationDuration = indicatorDuration / 2.0f;
      
      // 确保每次动画都从正确的初始位置开始
      // 获取当前的基础边界，确保初始位置计算准确
      auto bounds = getLocalBounds();
const auto jjW = juce::jmax(1, juce::roundToInt(51.0f * tremolo::defaults::jjImageScale));
const auto jjH = juce::jmax(1, juce::roundToInt(325.0f * tremolo::defaults::jjImageScale));
      auto baseBounds = bounds.withSizeKeepingCentre(jjW, jjH).translated(0, 200);

      
      // 设置运动参数：从“起始偏移”向上移动到“最高点偏移”
      startYPosition = tremolo::defaults::jjAnimationStartYOffsetPx;
      targetYPosition = tremolo::defaults::jjAnimationPeakYOffsetPx;
      
      // 强制设置图片到“起始偏移”位置，确保动画起点准确
      jjImage.setBounds(baseBounds.translated(
          0, -static_cast<int>(tremolo::defaults::jjAnimationStartYOffsetPx)));
  }
    
    // 更新动画状态：无论是否触发新动画，都需要更新当前动画
    updateAnimation();
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

  
  // 设置设置面板的位置：覆盖整个界面，但留出边距
  auto settingsPanelBounds = bounds.reduced(50);
  settingsPanel.setBounds(settingsPanelBounds);

  // // 设置Logo的位置和大小：左上角(16,16)，宽105，高24
  // logo.setBounds({16, 16, 105, 24});

  // 计算增益控制区域的边界：顶部区域，高度80像素
  auto gainArea = bounds.removeFromTop(40);
  
  // // 设置增益标签：左侧，宽度60像素
  // auto gainLabelBounds = gainArea.removeFromLeft(60);
  // gainLabel.setBounds(gainLabelBounds);
  
  // // 设置增益控制条：剩余区域，左右留出20像素边距
  // gainArea.reduce(20, 0);
  // gainSlider.setBounds(gainArea);

  // XY 控制器：固定长宽，通过左上角偏移确定位置
  xyController.setBounds(tremolo::defaults::xyControllerLeftPx,
                         tremolo::defaults::xyControllerTopPx,
                         tremolo::defaults::xyControllerWidthPx,
                         tremolo::defaults::xyControllerHeightPx);

  // 计算旁路按钮的边界：右上角区域
  // auto bypassButtonBounds = bounds;
  // bypassButtonBounds.removeFromTop(0);
  // bypassButtonBounds.removeFromRight(0);
  // bypassButtonBounds.removeFromBottom(660);
  // bypassButtonBounds.removeFromLeft(560);
  // bypassButton.setBounds(bypassButtonBounds);

  // 计算旁路标签的边界：旁路按钮上方
  // auto bypassLabelBounds = bounds;
  // bypassLabelBounds.removeFromTop(48);
  // bypassLabelBounds.removeFromRight(104);
  // bypassLabelBounds.removeFromBottom(206);
  // bypassLabelBounds.removeFromLeft(396);
  // bypassLabel.setBounds(bypassLabelBounds);

  // 设置指示灯：固定长宽，通过左上角偏移确定位置
  indicatorLight.setBounds(tremolo::defaults::indicatorLightLeftPx,
                           tremolo::defaults::indicatorLightTopPx,
                           tremolo::defaults::indicatorLightWidthPx,
                           tremolo::defaults::indicatorLightHeightPx);

  // 设置jj.png图片的位置和大小：基准尺寸(51x325) * 缩放比
const auto jjW = juce::jmax(1, juce::roundToInt(51.0f * tremolo::defaults::jjImageScale));
const auto jjH = juce::jmax(1, juce::roundToInt(325.0f * tremolo::defaults::jjImageScale));
  auto baseBounds = bounds.withSizeKeepingCentre(jjW, jjH).translated(0, 200);

  
  // 图片位置管理逻辑：根据动画状态和第一次指示灯状态决定图片显示和位置
  // 如果正在动画中，使用动画系统设置位置；否则根据第一次指示灯状态处理
  if (isAnimating) {
    // 动画进行中：调用updateAnimation()函数更新图片位置
    // updateAnimation()会根据当前动画进度和缓动函数计算精确位置
    updateAnimation(); // 更新动画位置
  } else {
    // 动画未进行：根据第一次指示灯状态处理图片
    if (isFirstIndicatorFlash) {
      // 第一次指示灯未亮起：保持图片隐藏状态
      jjImage.setVisible(false);
    } else {
      // 第一次指示灯已亮起：显示图片并确保在初始位置（Y偏移为0）
      jjImage.setVisible(true);
      jjImage.setBounds(baseBounds.translated(
          0, -static_cast<int>(tremolo::defaults::jjAnimationStartYOffsetPx)));
    }
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
            auto bounds = getLocalBounds();
const auto jjW = juce::jmax(1, juce::roundToInt(51.0f * tremolo::defaults::jjImageScale));
const auto jjH = juce::jmax(1, juce::roundToInt(325.0f * tremolo::defaults::jjImageScale));
            auto baseBounds = bounds.withSizeKeepingCentre(jjW, jjH).translated(0, 200);
            jjImage.setBounds(baseBounds.translated(
                0, -static_cast<int>(tremolo::defaults::jjAnimationStartYOffsetPx)));
            return; // 直接返回，不再执行后续位置计算

        }
    }
    
    // 更新图片位置：根据当前动画状态计算Y轴偏移量
    auto bounds = getLocalBounds();
const auto jjW = juce::jmax(1, juce::roundToInt(51.0f * tremolo::defaults::jjImageScale));
const auto jjH = juce::jmax(1, juce::roundToInt(325.0f * tremolo::defaults::jjImageScale));
    auto baseBounds = bounds.withSizeKeepingCentre(jjW, jjH).translated(0, 200);

    
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