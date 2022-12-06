#pragma once

#include "graphics.hpp"
#include "image.hpp"
#include <vector>

namespace Coil
{
  class Font
  {
  public:
    struct GlyphsPacking
    {
      ivec2 size;

      // glyphs scale if they are upscaled
      ivec2 scale;

      struct GlyphInfo
      {
        // glyph size
        ivec2 size;
        // left-top corner of the glyph on texture
        ivec2 leftTop;
        // offset from pen point to left-top corner on canvas
        ivec2 offset;
      };

      std::vector<GlyphInfo> glyphInfos;

      RawImage2D<uint8_t> glyphsImage;
    };

    struct CreateGlyphsConfig
    {
      ivec2 halfScale;
      std::optional<std::vector<uint32_t>> glyphsNeeded;
      ivec2 maxSize = { 4096, 4096 };
      bool enableHinting = false;
    };

    struct Metrics
    {
      float ascender = 0;
      float descender = 0;
      float height = 0;
      float capHeight = 0;
    };

    virtual GlyphsPacking PackGlyphs(CreateGlyphsConfig const& config) = 0;
  };
}
