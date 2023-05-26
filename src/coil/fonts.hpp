#pragma once

#include "graphics.hpp"
#include "image.hpp"
#include "localization.hpp"
#include <tuple>
#include <vector>

namespace Coil
{
  struct GlyphWithOffset
  {
    // whatever glyph index
    uint32_t index;
    // glyph offset, numbers from 0 to offset precision - 1
    uint8_t offsetX = 0;
    uint8_t offsetY = 0;

    friend auto operator<=>(GlyphWithOffset const&, GlyphWithOffset const&) = default;
  };

  // packing of glyphs into texture
  struct GlyphsPacking
  {
    ivec2 size;

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
  };

  class Font
  {
  public:
    struct CreateGlyphsConfig
    {
      std::vector<GlyphWithOffset> glyphsNeeded;
      ivec2 maxSize = { 4096, 4096 };
      ivec2 offsetPrecision = { 1, 1 };
    };

    struct Metrics
    {
      float ascender = 0;
      float descender = 0;
      float height = 0;
      float capHeight = 0;
    };

    struct ShapedGlyph
    {
      vec2 position;
      vec2 advance;
      uint32_t glyphIndex;
      // index of first character in a cluster corresponding to this glyph
      uint32_t characterIndex;
    };

    virtual void Shape(std::string const& text, LanguageInfo const& languageInfo, std::vector<ShapedGlyph>& shapedGlyphs) const = 0;

    virtual std::tuple<GlyphsPacking, RawImage2D<uint8_t>> PackGlyphs(CreateGlyphsConfig const& config) const = 0;
  };
}
