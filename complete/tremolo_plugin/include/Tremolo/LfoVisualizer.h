#pragma once

namespace tremolo {
class LfoVisualizer : public juce::Component {
public:
  using ReadAllLfoSamples = std::function<void(juce::AudioBuffer<float>&)>;
  using GetCurrentSampleRate = std::function<double()>;
  using IsBypassed = std::function<bool()>;

  LfoVisualizer(ReadAllLfoSamples readSamples,
                GetCurrentSampleRate getRate,
                IsBypassed getIsBypassed);

  void paint(juce::Graphics& g) override;

  void setCurveWidth(float w);
  void setCurveColor(juce::Colour c);

  void setBackgroundColor(juce::Colour c);

private:
  static constexpr auto pointsOnPath = 22050u;
  static constexpr auto periodsToPlotOf1HzWaveform = 4u;

  void update(double timestampSeconds);

  void updateLfoCurve(double timestampSeconds);

  void updateSamplesQueue(double timestampSeconds);

  [[nodiscard]] size_t getStride() const;

  void samplesToPath();

  /** @brief 创建一个变换，将当前LFO曲线映射到组件边界
   *
   * @detail 变换基于以下点映射：
   *
   *   (0,ylim)                        -> (0,0) (左上角)
   *   (0,-ylim)                       -> (0, height) (左下角)
   *   (曲线结束X坐标, -ylim) -> (组件宽度, 组件高度)
   *                                      (右下角)
   */
  [[nodiscard]] juce::AffineTransform getLfoCurveTransform() const;

  float curveWidth{4.f};
  juce::Colour curveColor{juce::Colours::black};
  juce::Colour backgroundColour{juce::Colours::white};
  ReadAllLfoSamples readAllLfoSamples;
  GetCurrentSampleRate getCurrentSampleRate;
  IsBypassed isBypassed;
  juce::AudioBuffer<float> buffer;
  juce::Path lfoCurve;

  detail::StridedQueue<float, pointsOnPath> lfoSamplesToPlot;

  std::optional<double> lastTimestampSeconds;
  juce::VBlankAttachment vblankAttachment{
      this, [this](double timestampSeconds) { update(timestampSeconds); }};
};
}  // namespace tremolo