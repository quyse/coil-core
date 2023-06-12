#pragma once

#include "fonts.hpp"
#include <map>
#include <optional>
#include <vector>

namespace Coil
{
  // cache of glyphs
  // keeps track of where glyphs are mapped to a texture,
  // decides what glyphs to keep, supports multiple fonts
  class FontGlyphCache
  {
  public:
    struct RenderGlyph
    {
      // top-left position
      ivec2 position;
      GlyphWithOffset glyphWithOffset;
      // index of first character in a cluster corresponding to this glyph
      uint32_t characterIndex;
    };

    FontGlyphCache(ivec2 const& offsetPrecision = { 4, 4 }, ivec2 const& size = { 1024, 1024 });

    void ShapeText(Font const& font, std::string const& text, LanguageInfo const& languageInfo, vec2 const& textOffset, std::vector<RenderGlyph>& renderGlyphs);

    // recreate image if needed
    bool Update();

    RawImage2D<uint8_t> const& GetImage() const;
    std::optional<GlyphsPacking::GlyphInfo> GetGlyphInfo(Font const& font, GlyphWithOffset const& glyphWithOffset) const;

  private:
    ivec2 const _offsetPrecision;
    ivec2 const _size;

    RawImage2D<uint8_t> _image;

    // glyphs (mapped and not mapped to packing)
    std::map<std::pair<Font const*, GlyphWithOffset>, int32_t> _glyphsMapping;
    // current packing
    std::vector<GlyphsPacking::GlyphInfo> _glyphsPacking;
    // is there not mapped glyphs
    bool _dirty = false;
    // temporary buffer for shaped glyphs
    std::vector<Font::ShapedGlyph> _tempShapedGlyphs;
  };
}
