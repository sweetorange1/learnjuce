// tremolo命名空间：C++中使用命名空间来组织代码，避免命名冲突
namespace tremolo {

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
    g.setColour(juce::Colours::green);
    g.drawLine(targetXPos - 8.0f, targetYPos, targetXPos + 8.0f, targetYPos, 2.0f);
    g.drawLine(targetXPos, targetYPos - 8.0f, targetXPos, targetYPos + 8.0f, 2.0f);
    
    // 计算当前位置
    float xPos = xValue * bounds.getWidth();
    float yPos = yValue * bounds.getHeight();
    
    // 绘制当前位置指示器（白色圆点）
    g.setColour(juce::Colours::white);
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
    g.setFont(juce::Font(14.0f));
    g.setColour(juce::Colours::white);
    
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
  background.setImage(juce::ImageCache::getFromMemory(
      assets::Background_png, assets::Background_pngSize));
  // 将背景组件添加到界面并使其可见
  addAndMakeVisible(background);

  // 设置Logo图片：从内存中加载Logo图片资源
  // logo.setImage(
  //     juce::ImageCache::getFromMemory(assets::Logo_png, assets::Logo_pngSize));
  // // 将Logo组件添加到界面
  // addAndMakeVisible(logo);

  // 定义侧边标签的字体颜色：使用JUCE的颜色系统（字的颜色）
  const auto sideFontColor = juce::Colour{0xFF6EA0C7};

  // 设置旁路标签的对齐方式为左对齐
  bypassLabel.setJustificationType(juce::Justification::left);
  // 设置最小水平缩放比例
  bypassLabel.setMinimumHorizontalScale(1.f);
  // 设置旁路标签的字体
  bypassLabel.setFont(lookAndFeel.getSideLabelsFont());
  // 设置旁路标签的文本颜色
  bypassLabel.setColour(juce::Label::textColourId, sideFontColor);
  // 将旁路标签添加到界面
  addAndMakeVisible(bypassLabel);

  // 设置旁路按钮的点击事件处理函数（使用lambda表达式）
  bypassButton.onClick = [this]() {
    // 根据按钮的切换状态设置按钮文本
    bypassButton.setButtonText(bypassButton.getToggleState() ? "Bypassed"
                                                             : "Off");
  };
  // 立即执行一次点击事件，确保初始状态正确
  bypassButton.onClick();
  // 将旁路按钮添加到界面
  addAndMakeVisible(bypassButton);

  // 设置增益标签
  gainLabel.setJustificationType(juce::Justification::centred);
  gainLabel.setMinimumHorizontalScale(1.f);
  gainLabel.setFont(lookAndFeel.getSideLabelsFont());
  gainLabel.setColour(juce::Label::textColourId, sideFontColor);
  gainLabel.setText("MAX GAIN", juce::dontSendNotification); // 明确表示是最大增益
  addAndMakeVisible(gainLabel);

  // 设置增益控制条
  gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  gainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
  gainSlider.setRange(0.1, 10.0, 0.1); // 增益范围从0.1到10.0，步进0.1
  gainSlider.setValue(1.0); // 默认增益为1.0
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
  
  // 将XY控制器添加到界面
  addAndMakeVisible(xyController);

  // 启动定时器用于更新指示灯状态（每秒30帧）
  startTimerHz(30);

  // 设置自定义外观：将lookAndFeel对象设置为当前组件的外观
  setLookAndFeel(&lookAndFeel);

  // 注释：确保在构造函数完成前设置编辑器的大小
  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
  // 设置编辑器大小为700x700像素
  setSize(700, 700);
}

// PluginEditor类的析构函数：在对象销毁时自动调用
PluginEditor::~PluginEditor() {
  // 移除自定义外观设置，恢复默认外观
  setLookAndFeel(nullptr);
}

// timerCallback方法：定时器回调，用于更新指示灯状态
void PluginEditor::timerCallback() {
    // 获取音频处理器引用（使用基类的processor成员）
    auto& audioProcessor = dynamic_cast<PluginProcessor&>(processor);
    
    // 更新指示灯状态（deltaTime = 1/30秒）
    audioProcessor.getTremolo().updateIndicatorState(1.0f / 30.0f);
    
    // 更新指示灯组件的闪烁状态
    indicatorLight.setFlashing(audioProcessor.getTremolo().shouldFlashIndicator());
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

  // // 设置Logo的位置和大小：左上角(16,16)，宽105，高24
  // logo.setBounds({16, 16, 105, 24});

  // 计算增益控制区域的边界：顶部区域，高度80像素
  auto gainArea = bounds.removeFromTop(40);
  
  // 设置增益标签：左侧，宽度60像素
  auto gainLabelBounds = gainArea.removeFromLeft(60);
  gainLabel.setBounds(gainLabelBounds);
  
  // 设置增益控制条：剩余区域，左右留出20像素边距
  gainArea.reduce(20, 0);
  gainSlider.setBounds(gainArea);

  // 计算XY控制器的边界：正方形，位于界面中央向上150像素，大小为600x600像素
  auto xyBounds = bounds.withSizeKeepingCentre(600, 600).translated(0, -20);
  xyController.setBounds(xyBounds);

  // 计算旁路按钮的边界：右上角区域
  auto bypassButtonBounds = bounds;
  bypassButtonBounds.removeFromTop(0);
  bypassButtonBounds.removeFromRight(0);
  bypassButtonBounds.removeFromBottom(660);
  bypassButtonBounds.removeFromLeft(560);
  bypassButton.setBounds(bypassButtonBounds);

  // 计算旁路标签的边界：旁路按钮上方
  auto bypassLabelBounds = bounds;
  bypassLabelBounds.removeFromTop(48);
  bypassLabelBounds.removeFromRight(104);
  bypassLabelBounds.removeFromBottom(206);
  bypassLabelBounds.removeFromLeft(396);
  bypassLabel.setBounds(bypassLabelBounds);

  // 计算指示灯区域的边界：顶部区域，在增益控制条下方
  auto indicatorArea = bounds.removeFromTop(120);
  indicatorArea.removeFromTop(80); // 移除增益控制区域
  
  // 设置指示灯标签：左侧，宽度60像素
  auto indicatorLabelBounds = indicatorArea.removeFromLeft(60);
  indicatorLabel.setBounds(indicatorLabelBounds);
  
  // 设置指示灯：右侧，圆形，直径40像素
  auto indicatorLightBounds = indicatorArea.removeFromRight(60);
  indicatorLightBounds = indicatorLightBounds.withSizeKeepingCentre(40, 40);
  indicatorLight.setBounds(indicatorLightBounds);
}

// 命名空间结束
}  // namespace tremolo