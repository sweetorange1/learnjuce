#pragma once

namespace tremolo {
/** 单生产者、单消费者FIFO队列，用于从音频线程检索单通道样本 */
template <typename SampleType>
class SampleFifo {
public:
  void prepare(double sampleRate) {
    // 我们希望提供足够的容量，以便在低帧率下不会丢失样本
    const auto sampleCapacity = static_cast<int>(1.0 * sampleRate);

    buffer.setSize(1, sampleCapacity);
    buffer.clear();
    fifo.setTotalSize(sampleCapacity);
  }

  void push(SampleType sample) {
    const auto scope = fifo.write(1);

    if (scope.blockSize1 > 0) {
      buffer.setSample(0, scope.startIndex1, sample);
    } else if (scope.blockSize2 > 0) {
      buffer.setSample(0, scope.startIndex2, sample);
    }
  }

  void popAll(juce::AudioBuffer<SampleType>& bufferToFill) {
    const auto sampleCount = fifo.getNumReady();

    // avoidReallocating = true，以避免在缓冲区大小不增加时重新分配
    bufferToFill.setSize(1, sampleCount, false, false, true);

    const auto scope = fifo.read(sampleCount);
    const auto* samplesToReadPtr = buffer.getReadPointer(0);
    auto* samplesToWritePtr = bufferToFill.getWritePointer(0);
    if (scope.blockSize1 > 0) {
      std::copy_n(samplesToReadPtr + scope.startIndex1, scope.blockSize1,
                  samplesToWritePtr);
    }

    if (scope.blockSize2 > 0) {
      std::copy_n(samplesToReadPtr + scope.startIndex2, scope.blockSize2,
                  samplesToWritePtr + scope.blockSize1);
    }
  }

  void reset() {
    fifo.reset();
    buffer.clear();
  }

private:
  static constexpr auto initialCapacity = 1024;
  juce::AbstractFifo fifo{initialCapacity};
  juce::AudioBuffer<SampleType> buffer{1, initialCapacity};
};
}  // namespace tremolo