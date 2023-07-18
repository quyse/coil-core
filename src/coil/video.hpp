#pragma once

#include "image.hpp"

namespace Coil
{
  // image format specialized for video frames
  // only 2D, supports multiple planes of different dimensions
  struct VideoFrame
  {
    static constexpr size_t maxPlanesCount = 3;

    enum class Format
    {
      Unknown,
      YUV420,
    };

    enum class ColorRange
    {
      Unknown,
      Studio, // Y [16..235], UV [16..240]
      Full, // [0..255]
    };

    Format format = Format::Unknown;
    ColorRange colorRange = ColorRange::Unknown;

    struct Plane
    {
      void* data = nullptr;
      int32_t width = 0;
      int32_t height = 0;
      int32_t pitch = 0;
    };
    Plane planes[maxPlanesCount];

    operator bool() const;

    RawImage2D<xvec<uint8_t, 3, 0>> GetImage() const;
  };

  // video input stream
  class VideoStream
  {
  public:
    virtual VideoFrame ReadFrame() = 0;
  };
}
