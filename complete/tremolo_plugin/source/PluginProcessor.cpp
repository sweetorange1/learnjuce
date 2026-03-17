#include <cmath>

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

  const auto inputChannelCount = juce::jmax(1, getTotalNumInputChannels());
  // 修复：避免使用assign方法，改用resize和循环初始化
  detectionHighpassFilters.resize(static_cast<size_t>(inputChannelCount));
  detectionLowpassFilters.resize(static_cast<size_t>(inputChannelCount));
  activeInputHighpassHz = inputHighpassHz.load(std::memory_order_relaxed);
  activeInputLowpassHz = inputLowpassHz.load(std::memory_order_relaxed);
  updateDetectionFilterCoefficients(activeInputHighpassHz, activeInputLowpassHz);

  latestInputLevel.store(0.0f, std::memory_order_relaxed);
  latestWindowedInputPeak.store(0.0f, std::memory_order_relaxed);
  triggerThresholdDb.store(-12.0f, std::memory_order_relaxed);

  resetLevelCaptureState();
}

// 释放资源函数：在播放停止时清理资源
void PluginProcessor::releaseResources() {
  // 当播放停止时，您可以使用此机会释放任何空闲内存等
  tremolo.reset();
  bypassTransitionSmoother.reset();
  detectionHighpassFilters.clear();
  detectionLowpassFilters.clear();
  latestInputLevel.store(0.0f, std::memory_order_relaxed);
  latestWindowedInputPeak.store(0.0f, std::memory_order_relaxed);
  resetLevelCaptureState();
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

  const auto desiredHighpassHz = inputHighpassHz.load(std::memory_order_relaxed);
  const auto desiredLowpassHz = inputLowpassHz.load(std::memory_order_relaxed);
  if (desiredHighpassHz != activeInputHighpassHz ||
      desiredLowpassHz != activeInputLowpassHz) {
    updateDetectionFilterCoefficients(desiredHighpassHz, desiredLowpassHz);
  }

  // 统一电平捕捉：跨多个block累计samples，累计到目标窗口长度后计算一次peak。
  pushFilteredSamplesAndMaybeUpdateWindowPeak(buffer);

  const auto windowedPeak = latestWindowedInputPeak.load(std::memory_order_relaxed);
  latestInputLevel.store(windowedPeak, std::memory_order_relaxed);
  tremolo.setThresholdDb(triggerThresholdDb.load(std::memory_order_relaxed));
  tremolo.updateDetectionPeak(windowedPeak);

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

  // 更新最大增益值
  tremolo.setMaxGain(parameters.gain.get());
  
  // 更新XY控制器参数（根据XY位置计算实际增益）
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

  // 设置旁路状态
  bypassTransitionSmoother.setBypassForced(parameters.bypassed);
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



// 线程安全地获取采样率：在多线程环境中安全获取当前采样率
double PluginProcessor::getSampleRateThreadSafe() const noexcept {
  return currentSampleRate;
}

float PluginProcessor::getLatestInputLevel() const noexcept {
  return latestInputLevel.load(std::memory_order_relaxed);
}

float PluginProcessor::getLatestWindowedInputPeak() const noexcept {
  return latestWindowedInputPeak.load(std::memory_order_relaxed);
}

void PluginProcessor::setLevelCaptureWindowMs(float windowMs) noexcept {
  const auto normalized = parameters.levelCaptureWindowMs.convertTo0to1(windowMs);
  parameters.levelCaptureWindowMs.setValueNotifyingHost(normalized);
}

float PluginProcessor::getLevelCaptureWindowMs() const noexcept {
  return parameters.levelCaptureWindowMs.get();
}

void PluginProcessor::setTriggerThresholdDb(float thresholdDb) noexcept {
  triggerThresholdDb.store(juce::jlimit(-60.0f, 0.0f, thresholdDb),
                           std::memory_order_relaxed);
}

float PluginProcessor::getTriggerThresholdDb() const noexcept {
  return triggerThresholdDb.load(std::memory_order_relaxed);
}

void PluginProcessor::setInputFilterFrequencies(float highpassHz,
                                                float lowpassHz) noexcept {
  const auto sampleRate = juce::jmax(1.0, getSampleRateThreadSafe());
  const auto maxCutoffHz = static_cast<float>(
      juce::jlimit(40.0, 20000.0, sampleRate * 0.5 - 20.0));

  auto clampedHighpassHz = juce::jlimit(20.0f, maxCutoffHz - 20.0f, highpassHz);
  auto clampedLowpassHz = juce::jlimit(40.0f, maxCutoffHz, lowpassHz);

  if (clampedHighpassHz >= clampedLowpassHz) {
    clampedHighpassHz = juce::jmin(clampedHighpassHz, clampedLowpassHz - 20.0f);
    clampedLowpassHz = juce::jmax(clampedLowpassHz, clampedHighpassHz + 20.0f);
  }

  inputHighpassHz.store(clampedHighpassHz, std::memory_order_relaxed);
  inputLowpassHz.store(clampedLowpassHz, std::memory_order_relaxed);
}

float PluginProcessor::getInputHighpassHz() const noexcept {
  return inputHighpassHz.load(std::memory_order_relaxed);
}

float PluginProcessor::getInputLowpassHz() const noexcept {
  return inputLowpassHz.load(std::memory_order_relaxed);
}

void PluginProcessor::updateDetectionFilterCoefficients(float highpassHz,
                                                        float lowpassHz) noexcept {
  const auto sampleRate = juce::jmax(1.0, getSampleRateThreadSafe());
  const auto maxCutoffHz = static_cast<float>(
      juce::jlimit(40.0, 20000.0, sampleRate * 0.5 - 20.0));
  activeInputHighpassHz = juce::jlimit(20.0f, maxCutoffHz - 20.0f, highpassHz);
  activeInputLowpassHz = juce::jlimit(activeInputHighpassHz + 20.0f,
                                      maxCutoffHz, lowpassHz);

  const auto highpassCoefficients =
      juce::IIRCoefficients::makeHighPass(sampleRate, activeInputHighpassHz);
  const auto lowpassCoefficients =
      juce::IIRCoefficients::makeLowPass(sampleRate, activeInputLowpassHz);

  for (auto& filter : detectionHighpassFilters) {
    filter.setCoefficients(highpassCoefficients);
    filter.reset();
  }

  for (auto& filter : detectionLowpassFilters) {
    filter.setCoefficients(lowpassCoefficients);
    filter.reset();
  }
}

float PluginProcessor::analyseFilteredInputPeak(
    const juce::AudioBuffer<float>& buffer) noexcept {
  const auto inputChannelCount = juce::jmin(
      buffer.getNumChannels(), static_cast<int>(detectionHighpassFilters.size()));

  if (inputChannelCount <= 0 || buffer.getNumSamples() <= 0) {
    return 0.0f;
  }

  float detectedPeak = 0.0f;

  for (int channel = 0; channel < inputChannelCount; ++channel) {
    const auto* readPointer = buffer.getReadPointer(channel);
    auto& highpassFilter = detectionHighpassFilters[static_cast<size_t>(channel)];
    auto& lowpassFilter = detectionLowpassFilters[static_cast<size_t>(channel)];

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
      const auto highpassed = highpassFilter.processSingleSampleRaw(readPointer[sample]);
      const auto bandLimited = lowpassFilter.processSingleSampleRaw(highpassed);
      detectedPeak = juce::jmax(detectedPeak, std::abs(bandLimited));
    }
  }

  return detectedPeak;
}

void PluginProcessor::resetLevelCaptureState() noexcept {
  levelCaptureAccumulatedSamples = 0;
  levelCaptureRunningPeak = 0.0f;
  levelCaptureTargetSamples = 0;
}

void PluginProcessor::pushFilteredSamplesAndMaybeUpdateWindowPeak(
    const juce::AudioBuffer<float>& buffer) noexcept {
  const auto inputChannelCount = juce::jmin(
      buffer.getNumChannels(), static_cast<int>(detectionHighpassFilters.size()));

  const auto numSamples = buffer.getNumSamples();
  if (inputChannelCount <= 0 || numSamples <= 0) {
    return;
  }

  const auto sampleRate = juce::jmax(1.0, getSampleRateThreadSafe());
  const auto windowMs = parameters.levelCaptureWindowMs.get();
  const auto windowSeconds = juce::jmax(0.001, static_cast<double>(windowMs) / 1000.0);
  const auto targetSamples = juce::jmax(1, static_cast<int>(std::llround(windowSeconds * sampleRate)));

  if (targetSamples != levelCaptureTargetSamples) {
    levelCaptureTargetSamples = targetSamples;
    levelCaptureAccumulatedSamples = 0;
    levelCaptureRunningPeak = 0.0f;
  }

  // 在本block中逐sample分析（带通滤波后取abs peak），累计到窗口长度后“结算”一次peak。
  for (int sample = 0; sample < numSamples; ++sample) {
    float detectedAbs = 0.0f;

    for (int channel = 0; channel < inputChannelCount; ++channel) {
      const auto* readPointer = buffer.getReadPointer(channel);
      auto& highpassFilter = detectionHighpassFilters[static_cast<size_t>(channel)];
      auto& lowpassFilter = detectionLowpassFilters[static_cast<size_t>(channel)];

      const auto highpassed = highpassFilter.processSingleSampleRaw(readPointer[sample]);
      const auto bandLimited = lowpassFilter.processSingleSampleRaw(highpassed);
      detectedAbs = juce::jmax(detectedAbs, std::abs(bandLimited));
    }

    levelCaptureRunningPeak = juce::jmax(levelCaptureRunningPeak, detectedAbs);
    ++levelCaptureAccumulatedSamples;

    if (levelCaptureAccumulatedSamples >= levelCaptureTargetSamples) {
      latestWindowedInputPeak.store(levelCaptureRunningPeak,
                                    std::memory_order_relaxed);
      levelCaptureAccumulatedSamples = 0;
      levelCaptureRunningPeak = 0.0f;
    }
  }
}

// 命名空间结束
}  // namespace tremolo

// 这创建插件的新实例
// 此函数定义必须在全局命名空间中：JUCE框架调用此函数创建插件实例
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  // 创建并返回新的PluginProcessor实例
  return new tremolo::PluginProcessor();
}