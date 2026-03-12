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
    
    // 计算当前位置
    float xPos = xValue * bounds.getWidth();
    float yPos = yValue * bounds.getHeight();
    
    // 绘制当前位置指示器（白色圆点）
    g.setColour(juce::Colours::white);
    g.fillEllipse(xPos - 5.0f, yPos - 5.0f, 10.0f, 10.0f);
    
    // 绘制指示器边框
    g.setColour(juce::Colours::black);
    g.drawEllipse(xPos - 5.0f, yPos - 5.0f, 10.0f, 10.0f, 2.0f);
    
    // 在右下角显示X和Y值
    juce::String valueText = juce::String("X: ") + juce::String(xValue, 2) + 
                            juce::String(" Y: ") + juce::String(yValue, 2);
    
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

  // 设置XY控制器的值变化回调函数
  xyController.setValueChangeCallback([&p](float x, float y) {
    // 更新音频处理器的X和Y参数值
    p.getParameterRefs().xValue.setValueNotifyingHost(x);
    p.getParameterRefs().yValue.setValueNotifyingHost(y);
  });
  
  // 将XY控制器添加到界面
  addAndMakeVisible(xyController);

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

// resized方法：当组件大小改变时自动调用，用于重新布局子组件
void PluginEditor::resized() {
  // 获取组件的本地边界（相对于父组件的坐标和大小）
  const auto bounds = getLocalBounds();

  // 设置背景图片覆盖整个边界
  background.setBounds(bounds);

  // // 设置Logo的位置和大小：左上角(16,16)，宽105，高24
  // logo.setBounds({16, 16, 105, 24});

  // 计算XY控制器的边界：正方形，位于界面中央，大小为300x300像素
  auto xyBounds = bounds.withSizeKeepingCentre(600, 600);
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
}

// 命名空间结束
}  // namespace tremolo