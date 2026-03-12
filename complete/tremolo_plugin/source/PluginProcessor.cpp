// 命名空间定义：将插件相关的代码组织在tremolo命名空间中，避免全局命名冲突
namespace tremolo {

// 构造函数：初始化音频处理器，设置输入输出音频总线配置
PluginProcessor::PluginProcessor()
    // 调用基类AudioProcessor的构造函数，传入音频总线配置
    : AudioProcessor(
          // BusesProperties用于定义插件的音频输入输出配置
          BusesProperties()
              // 设置输入总线：名称为"Input"，立体声通道，启用状态
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              // 设置输出总线：名称为"Output"，立体声通道，启用状态
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

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
}

// 释放资源函数：在播放停止时清理资源
void PluginProcessor::releaseResources() {
  // 当播放停止时，您可以使用此机会释放任何空闲内存等
  tremolo.reset();
  bypassTransitionSmoother.reset();
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
      parameters.bypassed.get() && !bypassTransitionSmoother.isTransitioning();
  // 根据状态决定是否应用平滑处理
  const auto applySmoothing =
      bypassedAndNotTransitioning ? ApplySmoothing::no : ApplySmoothing::yes;

  // 更新参数
  // 如果完全旁路，跳过平滑处理以避免LFO波形变形
  // 当参数在旁路开启状态下更改时
  // 例如，如果LFO波形是正弦波，而用户在旁路开启状态下选择三角波
  // 在切换旁路关闭时，他们将看到弯曲的三角波斜率，这是意外的
  tremolo.setModulationRateHz(parameters.rate, applySmoothing);
  tremolo.setLfoWaveform(
      static_cast<Tremolo::LfoWaveform>(parameters.waveform.getIndex()),
      applySmoothing);
  
  // 更新XY控制器参数
  tremolo.setXYValues(parameters.xValue.get(), parameters.yValue.get());

  // 设置旁路状态到过渡平滑器
  bypassTransitionSmoother.setBypass(parameters.bypassed);

  // 如果插件完全旁路，避免处理音频数据
  if (bypassedAndNotTransitioning) {
    return;
  }

  // 设置干信号缓冲区（原始输入信号）
  bypassTransitionSmoother.setDryBuffer(buffer);

  // 应用颤音效果到音频缓冲区
  tremolo.process(buffer);

  // 将处理后的湿信号与干信号混合
  bypassTransitionSmoother.mixToWetBuffer(buffer);
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
  // 使用JSON序列化器保存参数状态
  JsonSerializer::serialize(parameters, outputStream);
}

// 从内存块加载插件状态：反序列化参数状态
void PluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
  // 创建内存输入流读取序列化数据
  juce::MemoryInputStream inputStream{data, static_cast<size_t>(sizeInBytes),
                                      false};
  // 使用JSON序列化器加载参数状态
  const auto result = JsonSerializer::deserialize(inputStream, parameters);

  // 检查反序列化是否成功
  if (result.failed()) {
    // 通知用户读取参数失败
    // 目前，我们只是将错误消息写入标准错误流
    DBG(result.getErrorMessage());
  }

  // 跳过平滑处理以避免LFO波形变形
  // 当加载项目或预设时
  // 例如，默认的LFO波形是正弦波。如果项目或预设选择了三角波
  // 用户在加载时将看到弯曲的三角波斜率，这是意外的
  bypassTransitionSmoother.setBypassForced(parameters.bypassed);
  tremolo.setLfoWaveform(
      static_cast<Tremolo::LfoWaveform>(parameters.waveform.getIndex()),
      ApplySmoothing::no);
  tremolo.setModulationRateHz(parameters.rate, ApplySmoothing::no);
}

// 获取参数引用：返回参数管理对象的引用
Parameters& PluginProcessor::getParameterRefs() noexcept {
  return parameters;
}

// 获取旁路参数：返回旁路参数的指针
juce::AudioProcessorParameter* PluginProcessor::getBypassParameter()
    const noexcept {
  return &parameters.bypassed;
}

// 读取所有LFO样本到缓冲区：用于可视化LFO波形
void PluginProcessor::readAllLfoSamples(
    juce::AudioBuffer<float>& bufferToFill) {
  tremolo.readAllLfoSamples(bufferToFill);
}

// 线程安全地获取采样率：在多线程环境中安全获取当前采样率
double PluginProcessor::getSampleRateThreadSafe() const noexcept {
  return currentSampleRate;
}

// 命名空间结束
}  // namespace tremolo

// 这创建插件的新实例
// 此函数定义必须在全局命名空间中：JUCE框架调用此函数创建插件实例
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  // 创建并返回新的PluginProcessor实例
  return new tremolo::PluginProcessor();
}