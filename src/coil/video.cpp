#include "video.hpp"

namespace
{
  using namespace Coil;

  // only support BT.709 for now
  constexpr mat4x4 transformYUV2RGB =
    mat4x4
    {
      255.0f / 2.0f, 0,             0,             255.0f / 2.0f,
      0,             255.0f / 2.0f, 0,             255.0f / 2.0f,
      0,             0,             255.0f / 2.0f, 255.0f / 2.0f,
      0,             0,             0,             1,
    } *
    mat4x4
    {
      1.0f,     0.0f,     1.5748f, 0,
      1.0f,    -0.1873f, -0.4681f, 0,
      1.0f,     1.8556f,  0.0f,    0,
      0,        0,        0,       1,
    } *
    mat4x4
    {
      2.0f / 255.0f, 0,             0,             -1,
      0,             2.0f / 255.0f, 0,             -1,
      0,             0,             2.0f / 255.0f, -1,
      0,             0,             0,              1,
    }
  ;
  // transform ColorRange::Studio to full range
  constexpr mat4x4 transformStudioToFull =
  {
    255.0f / (235.0f - 16.0f), 0,                         0,                         -16.0f * 255.0f / (235.0f - 16.0f),
    0,                         255.0f / (240.0f - 16.0f), 0,                         -16.0f * 255.0f / (240.0f - 16.0f),
    0,                         0,                         255.0f / (240.0f - 16.0f), -16.0f * 255.0f / (240.0f - 16.0f),
    0,                         0,                         0,                         1,
  };
}

namespace Coil
{
  VideoFrame::operator bool() const
  {
    return format != Format::Unknown;
  }

  RawImage2D<xvec<uint8_t, 3, 0>> VideoFrame::GetImage() const
  {
    switch(format)
    {
    case Format::YUV420:
      {
        mat4x4 transform;

        switch(colorRange)
        {
        case ColorRange::Studio:
          {
            static constinit mat4x4 t = transformYUV2RGB * transformStudioToFull;
            transform = t;
          }
          break;
        case ColorRange::Full:
          {
            transform = transformYUV2RGB;
          }
          break;
        default:
          throw Exception("unsupported video frame color range for getting image");
        }

        auto clamp = [](float value) -> uint8_t
        {
          return (uint8_t)std::min(std::max(value, 0.0f), 255.0f);
        };

        int32_t const width = planes[0].width;
        int32_t const height = planes[0].height;

        RawImage2D<xvec<uint8_t, 3, 0>> image({ width, height });

        int32_t i0 = 0, i1 = 0, i2 = 0, ii = 0;
        for(int32_t i = 0; i < height; ++i)
        {
          for(int32_t j = 0; j < width; ++j)
          {
            vec4 pixel = transform * vec4
            {
              (float)*((uint8_t*)planes[0].data + i0 + j),
              (float)*((uint8_t*)planes[1].data + i1 + j / 2),
              (float)*((uint8_t*)planes[2].data + i2 + j / 2),
              1.0f
            };
            image.pixels[ii + j] =
            {
              clamp(pixel.x()),
              clamp(pixel.y()),
              clamp(pixel.z()),
            };
          }
          i0 += planes[0].pitch;
          if(i % 2)
          {
            i1 += planes[1].pitch;
            i2 += planes[2].pitch;
          }
          ii += image.pitch.y();
        }

        return std::move(image);
      }
      break;
    default:
      throw Exception("unsupported video frame format for getting image");
    }
  }
}
