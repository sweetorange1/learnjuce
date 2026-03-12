#pragma once

namespace tremolo {
class JsonSerializer {
public:
  static void serialize(const Parameters&, juce::OutputStream&);

  /** @return 失败时返回错误信息；成功时返回空字符串。
   *           如果发生错误，不会更新任何参数。 */
  static juce::Result deserialize(juce::InputStream&, Parameters&);
};
}  // namespace tremolo
