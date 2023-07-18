#pragma once

#include "video.hpp"
#include <gav1/decoder.h>

namespace Coil
{
  class Av1DecodeStream final : public VideoStream
  {
  public:
    Av1DecodeStream(PacketInputStream& inputStream);

    // VideoStream's methods
    VideoFrame ReadFrame() override;

  private:
    static VideoFrame::Format GetFormat(libgav1::ImageFormat format);
    static VideoFrame::ColorRange GetColorRange(libgav1::ColorRange colorRange);

    PacketInputStream& _inputStream;
    libgav1::Decoder _decoder;
  };
}
