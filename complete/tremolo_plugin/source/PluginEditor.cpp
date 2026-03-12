// tremolo命名空间：C++中使用命名空间来组织代码，避免命名冲突
namespace tremolo {

// PluginEditor类的构造函数：这是创建插件编辑器界面的入口点
// AudioProcessorEditor(&p)：继承自JUCE框架的音频处理器编辑器基类
// 参数p是PluginProcessor的引用，表示这个编辑器对应的音频处理器
PluginEditor::PluginEditor(PluginProcessor& p)
    // 初始化列表：C++中用于初始化成员变量的高效方式
    // AudioProcessorEditor(&p)：调用基类构造函数，传入音频处理器指针
    : AudioProcessorEditor(&p),
      // waveformAttachment：将波形选择参数与下拉框组件绑定
      waveformAttachment{p.getParameterRefs().waveform, waveformComboBox},
      // rateAttachment：将频率参数与旋钮滑块绑定
      rateAttachment{p.getParameterRefs().rate, rateSlider},
      // bypassAttachment：将旁路参数与按钮绑定
      bypassAttachment{p.getParameterRefs().bypassed, bypassButton},
      // lfoVisualizer：低频振荡器可视化组件，使用lambda表达式初始化
      lfoVisualizer{
          // lambda表达式：匿名函数，用于读取LFO采样数据
          [&p](juce::AudioBuffer<float>& b) { p.readAllLfoSamples(b); },
          // 获取采样率的lambda函数
          [&p] { return p.getSampleRateThreadSafe(); },
          // 获取旁路状态的lambda函数
          [&p] { return p.getParameterRefs().bypassed.get(); }},
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

  // 设置波形标签的对齐方式为左对齐
  waveformLabel.setJustificationType(juce::Justification::left);
  // 设置最小水平缩放比例为1.0，防止文本被压缩
  waveformLabel.setMinimumHorizontalScale(1.f);
  // 设置字体为自定义外观的侧边标签字体
  waveformLabel.setFont(lookAndFeel.getSideLabelsFont());
  // 设置标签文本颜色
  waveformLabel.setColour(juce::Label::textColourId, sideFontColor);
  // 将波形标签添加到界面
  addAndMakeVisible(waveformLabel);

  // 为波形下拉框添加选项：从参数中获取可选的波形类型
  waveformComboBox.addItemList(p.getParameterRefs().waveform.choices, 1);
  // 发送初始更新，确保界面与参数状态同步
  waveformAttachment.sendInitialUpdate();
  // 将波形下拉框添加到界面
  addAndMakeVisible(waveformComboBox);

  // 设置频率滑块的样式为旋转式（旋钮）
  rateSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
  // 设置文本框样式：不显示文本框
  rateSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox,
                             true, 0, 0);
  // 设置数值后缀为" Hz"，表示赫兹单位
  rateSlider.setTextValueSuffix(" Hz");
  // 启用弹出显示：鼠标悬停时显示当前值
  rateSlider.setPopupDisplayEnabled(true, true, this);
  // 将频率滑块添加到界面
  addAndMakeVisible(rateSlider);

  // 设置频率标签的对齐方式为居中对齐
  rateLabel.setJustificationType(juce::Justification::centred);
  // 设置标签不拦截鼠标点击事件
  rateLabel.setInterceptsMouseClicks(false, false);
  // 设置频率标签的字体
  rateLabel.setFont(lookAndFeel.getRateLabelFont());
  // 将频率标签添加到界面
  addAndMakeVisible(rateLabel);

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

  // 设置LFO可视化器的曲线宽度为2像素
  lfoVisualizer.setCurveWidth(2.f);
  // 设置曲线颜色为自定义外观的橙色
  lfoVisualizer.setCurveColor(
      lookAndFeel.getColor(CustomLookAndFeel::Colors::orange));
  // 设置背景颜色为透明黑色
  lfoVisualizer.setBackgroundColor(juce::Colours::transparentBlack);
  // 将LFO可视化器添加到界面
  addAndMakeVisible(lfoVisualizer);

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

  // 计算LFO可视化器的边界：整体边界缩小18像素，然后去掉顶部122像素
  auto lfoVisualizerBounds = bounds.reduced(18, 27);
  lfoVisualizerBounds.removeFromTop(122);
  lfoVisualizer.setBounds(lfoVisualizerBounds);

  // 计算频率滑块的边界：整体边界缩小230像素，然后去掉底部110像素
  auto rateSliderBounds = bounds.reduced(230, 40);
  rateSliderBounds.removeFromBottom(110);
  rateSlider.setBounds(rateSliderBounds);
  // 频率标签使用与滑块相同的边界
  rateLabel.setBounds(rateSliderBounds);

  // 计算波形下拉框的边界：去掉顶部66像素，右侧392像素，底部176像素，左侧16像素
  auto waveformComboBoxBounds = bounds;
  waveformComboBoxBounds.removeFromTop(66);
  waveformComboBoxBounds.removeFromRight(392);
  waveformComboBoxBounds.removeFromBottom(176);
  waveformComboBoxBounds.removeFromLeft(16);
  waveformComboBox.setBounds(waveformComboBoxBounds);

  // 计算波形标签的边界：去掉顶部48像素
  auto waveformLabelBounds = bounds;
  waveformLabelBounds.removeFromTop(48);

  // 注释：这里比Figma设计留更多空间以避免插入省略号
  // we make more space here than in Figma to avoid ellipsis insertion
  waveformLabelBounds.removeFromRight(461);

  waveformLabelBounds.removeFromBottom(206);
  waveformLabelBounds.removeFromLeft(20);

  waveformLabel.setBounds(waveformLabelBounds);

  // 计算旁路按钮的边界：去掉顶部66像素，右侧16像素，底部176像素，左侧392像素
  auto bypassButtonBounds = bounds;
  bypassButtonBounds.removeFromTop(66);
  bypassButtonBounds.removeFromRight(16);
  bypassButtonBounds.removeFromBottom(176);
  bypassButtonBounds.removeFromLeft(392);
  bypassButton.setBounds(bypassButtonBounds);

  // 计算旁路标签的边界：去掉顶部48像素
  auto bypassLabelBounds = bounds;
  bypassLabelBounds.removeFromTop(48);

  // 注释：这里比Figma设计留更多空间以避免插入省略号
  // we make more space here than in Figma to avoid ellipsis insertion
  bypassLabelBounds.removeFromRight(104);

  bypassLabelBounds.removeFromBottom(206);
  bypassLabelBounds.removeFromLeft(396);

  bypassLabel.setBounds(bypassLabelBounds);
}

// 命名空间结束
}  // namespace tremolo