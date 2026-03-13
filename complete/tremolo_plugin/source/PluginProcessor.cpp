// 命名空间定义：将插件相关的代码组织在tremolo命名空间中，避免全局命名冲突
namespace tremolo {

// 创建参数布局：定义插件的所有参数及其属性
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // 旁路参数
    layout.add(std::make_unique<juce::AudioParameterBool>("bypassed", "Bypass", false));
    
    // 增益参数
    layout.add(std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.1f, 10.0f, 4.0f));
    
    // X值参数
    layout.add(std::make_unique<juce::AudioParameterFloat>("xValue", "X Value", 0.0f, 1.0f, 0.5f));
    
    // Y值参数
    layout.add(std::make_unique<juce::AudioParameterFloat>("yValue", "Y Value", 0.0f, 1.0f, 0.5f));
    
    return layout;
}

// 析构函数：清理资源
PluginProcessor::~PluginProcessor()
{
    // 析构函数体为空，因为所有成员变量都是自动管理的
    // JUCE框架会自动清理AudioProcessorValueTreeState等资源
}

// 构造函数：初始化音频处理器，设置输入输出音频总线配置
PluginProcessor::PluginProcessor()
    // 调用基类AudioProcessor的构造函数，传入音频总线配置
    : AudioProcessor(
          // BusesProperties用于定义插件的音频输入输出配置
          BusesProperties()
              // 设置输入总线：名称为"Input"，立体声通道，启用状态
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              // 设置输出总线：名称为"Output"，立体声通道，启用状态
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      // 初始化参数管理器
      parameters(*this, nullptr, "Parameters", createParameterLayout()),
      // 初始化频谱分析器FIFO（缓冲区大小：8192采样点）
      analysisFifo(8192)
{
    // 初始化输入分析缓冲区
    inputBufferForAnalysis.setSize(2, 8192); // 立体声，8192采样点
}

// 获取插件名称：返回插件的标识名称，JUCE框架调用此函数获取插件信息
const juce::String PluginProcessor::getName() const {
  // TREMOLO_PLUGIN_NAME是预定义的宏，包含插件的名称
  return TREMOLO_PLUGIN_NAME;
}

// 检查是否接受MIDI输入：返回false表示此插件不处理MIDI消息
bool PluginProcessor::acceptsMidi() const {
  return false;
}

// 检查是否产生MIDI输出：返回false表示此插件不生成MIDI消息
bool PluginProcessor::producesMidi() const {
  return false;
}

// 检查是否是MIDI效果器：返回false表示这是音频效果器而非MIDI效果器
bool PluginProcessor::isMidiEffect() const {
  return false;
}

// 获取尾音长度：返回0.0表示没有尾音效果（立即停止）
double PluginProcessor::getTailLengthSeconds() const {
  return 0.0;
}

// 获取程序数量：返回预设程序的数量
int PluginProcessor::getNumPrograms() {
  // 某些宿主程序在被告知有0个程序时处理不佳，因此即使您没有真正实现程序功能，
  // 这里也应该至少返回1
  return 1;
}

// 获取当前程序索引：返回当前选中的程序编号
int PluginProcessor::getCurrentProgram() {
  return 0;
}

// 设置当前程序：根据索引切换程序
void PluginProcessor::setCurrentProgram(int index) {
  // ignoreUnused宏用于消除未使用参数的编译器警告
  juce::ignoreUnused(index);
}

// 获取程序名称：根据程序索引返回对应的程序名称
const juce::String PluginProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  // 返回空字符串表示没有具体的程序名称
  return {};
}

// 更改程序名称：允许用户修改程序名称
void PluginProcessor::changeProgramName(int index,
                                        const juce::String& newName) {
  juce::ignoreUnused(index, newName);
}

// 音频处理准备函数，在播放开始前调用：初始化音频处理所需的各种资源
void PluginProcessor::prepareToPlay(double sampleRate,
                                    int expectedMaxFramesPerBlock) {
  // 保存当前采样率，供其他函数使用
  currentSampleRate = sampleRate;

  // 准备颤音效果器：传入采样率和最大块大小
  tremolo.prepare(sampleRate, expectedMaxFramesPerBlock);

  // 准备旁路过渡平滑器：配置平滑过渡参数
  bypassTransitionSmoother.prepare(
      // 使用初始化列表配置平滑器参数
      {.sampleRate = sampleRate,
       .maximumBlockSize = static_cast<uint32_t>(expectedMaxFramesPerBlock),
       .numChannels = static_cast<uint32_t>(juce::jmax(
           getTotalNumInputChannels(), getTotalNumOutputChannels()))});

  // 重置频谱分析器FIFO
  analysisFifo.reset();
  inputBufferForAnalysis.clear();
}

// 释放资源函数：在播放停止时清理资源
void PluginProcessor::releaseResources() {
  // 当播放停止时，您可以使用此机会释放任何空闲内存等
  tremolo.reset();
  bypassTransitionSmoother.reset();
  
  // 清空频谱分析器缓冲区
  analysisFifo.reset();
  inputBufferForAnalysis.clear();
}

// 检查音频总线布局是否受支持：验证宿主程序提供的音频配置是否兼容
bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  // 这里是检查布局是否受支持的地方
  // 在此模板代码中，我们仅支持单声道或立体声
  // 某些插件宿主（如某些GarageBand版本）只会加载支持立体声总线布局的插件
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
    return false;
  }

  // 检查输入布局是否与输出布局匹配：确保输入输出通道数一致
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) {
    return false;
  }

  return true;
}

// 主要的音频处理函数，对每个音频块调用：这是插件的核心处理逻辑
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages) {
  // 忽略MIDI消息，因为此插件不处理MIDI
  juce::ignoreUnused(midiMessages);

  // ScopedNoDenormals用于禁用非规格化浮点数处理，提高性能
  juce::ScopedNoDenormals noDenormals;
  // 获取输入和输出通道总数
  const auto totalNumInputChannels = getTotalNumInputChannels();
  const auto totalNumOutputChannels = getTotalNumOutputChannels();

  // 如果我们有比输入更多的输出通道，此代码会清除任何不包含输入数据的输出通道
  // （因为这些通道不保证为空 - 它们可能包含垃圾数据）
  // 这是为了避免人们在首次编译插件时获得尖叫反馈
  // 但如果您的算法总是覆盖所有输出通道，显然不需要保留此代码
  for (const auto channelToClear :
       std::views::iota(totalNumInputChannels, totalNumOutputChannels)) {
    buffer.clear(channelToClear, 0, buffer.getNumSamples());
  }

  // 检查是否处于完全旁路状态且没有过渡过程
  const auto bypassedAndNotTransitioning =
      getParameterRefs().bypassed.get();

  // 更新最大增益值
  tremolo.setMaxGain(getParameterRefs().gain.get());
  
  // 更新XY控制器参数（根据XY位置计算实际增益）
  tremolo.setXYValues(getParameterRefs().xValue.get(), getParameterRefs().yValue.get());

  // 设置旁路状态到过渡平滑器
  bypassTransitionSmoother.setWetMixProportion(getParameterRefs().bypassed.get() ? 0.0f : 1.0f);

  // 如果频谱分析器激活，保存输入音频数据用于分析
  if (spectrumAnalyserActive && totalNumInputChannels > 0) {
    const int numSamples = buffer.getNumSamples();
    
    // 检查FIFO是否有足够空间
    if (analysisFifo.getFreeSpace() >= numSamples) {
      int start1, block1, start2, block2;
      analysisFifo.prepareToWrite(numSamples, start1, block1, start2, block2);
      
      // 写入左声道数据
      if (block1 > 0)
        inputBufferForAnalysis.copyFrom(0, start1, buffer.getReadPointer(0), block1);
      if (block2 > 0)
        inputBufferForAnalysis.copyFrom(0, start2, buffer.getReadPointer(0, block1), block2);
      
      // 写入右声道数据（如果存在）
      if (totalNumInputChannels > 1) {
        if (block1 > 0)
          inputBufferForAnalysis.addFrom(1, start1, buffer.getReadPointer(1), block1);
        if (block2 > 0)
          inputBufferForAnalysis.addFrom(1, start2, buffer.getReadPointer(1, block1), block2);
      }
      
      analysisFifo.finishedWrite(block1 + block2);
    }
  }

  // 如果插件完全旁路，避免处理音频数据
  if (bypassedAndNotTransitioning) {
    return;
  }

  // 设置干信号缓冲区（原始输入信号）
  juce::dsp::AudioBlock<float> dryBlock(buffer);
  bypassTransitionSmoother.pushDrySamples(dryBlock);

  // 应用颤音效果到音频缓冲区
  tremolo.process(buffer);

  // 将处理后的湿信号与干信号混合
  juce::dsp::AudioBlock<float> wetBlock(buffer);
  bypassTransitionSmoother.mixWetSamples(wetBlock);
}

// 频谱分析器相关方法实现
void PluginProcessor::getInputAudioForAnalysis(juce::AudioBuffer<float>& buffer, int maxSamples)
{
    if (!spectrumAnalyserActive) return;
    
    const int numReady = analysisFifo.getNumReady();
    const int samplesToRead = juce::jmin(maxSamples, numReady);
    
    if (samplesToRead > 0) {
        buffer.setSize(1, samplesToRead); // 只返回单声道数据
        buffer.clear();
        
        int start1, block1, start2, block2;
        analysisFifo.prepareToRead(samplesToRead, start1, block1, start2, block2);
        
        // 读取左声道数据
        if (block1 > 0)
            buffer.copyFrom(0, 0, inputBufferForAnalysis.getReadPointer(0, start1), block1);
        if (block2 > 0)
            buffer.copyFrom(0, block1, inputBufferForAnalysis.getReadPointer(0, start2), block2);
        
        // 如果存在右声道，混合到单声道
        if (inputBufferForAnalysis.getNumChannels() > 1) {
            if (block1 > 0)
                buffer.addFrom(0, 0, inputBufferForAnalysis.getReadPointer(1, start1), block1);
            if (block2 > 0)
                buffer.addFrom(0, block1, inputBufferForAnalysis.getReadPointer(1, start2), block2);
            
            // 平均左右声道
            buffer.applyGain(0.5f);
        }
        
        analysisFifo.finishedRead(block1 + block2);
    }
}

void PluginProcessor::setSpectrumAnalyserActive(bool shouldBeActive)
{
    spectrumAnalyserActive = shouldBeActive;
    
    if (!shouldBeActive) {
        // 停用时清空缓冲区
        analysisFifo.reset();
        inputBufferForAnalysis.clear();
    }
}

// 检查是否有自定义编辑器：返回true表示插件有图形界面
bool PluginProcessor::hasEditor() const {
  return true;
}

// 此函数将被调用来创建编辑器实例：返回插件编辑器的指针
juce::AudioProcessorEditor* PluginProcessor::createEditor() {
  // 创建新的PluginEditor实例，传入当前处理器的引用
  return new PluginEditor(*this);
}

// 保存插件状态到内存块：将参数状态序列化保存
void PluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
  // 创建内存输出流用于序列化数据
  juce::MemoryOutputStream outputStream{destData, true};
  // 创建Parameters实例并传递给JSON序列化器
  Parameters params(*this);
  JsonSerializer::serialize(params, outputStream);
}

// 从内存块加载插件状态：反序列化参数状态
void PluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
  // 创建内存输入流读取序列化数据
  juce::MemoryInputStream inputStream{data, static_cast<size_t>(sizeInBytes),
                                      false};
  // 创建Parameters实例并传递给JSON序列化器
  Parameters params(*this);
  const auto result = JsonSerializer::deserialize(inputStream, params);

  // 检查反序列化是否成功
  if (result.failed()) {
    // 通知用户读取参数失败
    // 目前，我们只是将错误消息写入标准错误流
    DBG(result.getErrorMessage());
  }

  // 设置旁路状态
  bypassTransitionSmoother.setWetMixProportion(getParameterRefs().bypassed.get() ? 0.0f : 1.0f);
}

// 获取参数引用：返回参数引用结构体
PluginProcessor::ParameterRefs& PluginProcessor::getParameterRefs() {
  static ParameterRefs refs{
    .bypassed = *dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter("bypassed")),
    .gain = *dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("gain")),
    .xValue = *dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("xValue")),
    .yValue = *dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("yValue"))
  };
  return refs;
}

// 获取旁路参数：返回旁路参数的指针
juce::AudioProcessorParameter* PluginProcessor::getBypassParameter()
    const noexcept {
  return parameters.getParameter("bypassed");
}

// 命名空间结束
}  // namespace tremolo

// 这创建插件的新实例
// 此函数定义必须在全局命名空间中：JUCE框架调用此函数创建插件实例
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  // 创建并返回新的PluginProcessor实例
  return new tremolo::PluginProcessor();
}