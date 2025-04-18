module;

#include <optional>
#include <string_view>
#include <vector>

export module coil.core.fonts;

import coil.core.base;
import coil.core.image;
import coil.core.localization;
import coil.core.math;

export namespace Coil
{
  // identifier for both font glyph and subpixel offset
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

  // parameters of variable font
  struct FontVariableStyle
  {
    // DPI scale, 1 means 96 DPI
    // used to calculate font's optical size
    float dpiScale = 1.0f;
    // 'wght' axis
    int16_t weight = 400; // regular by default
    // 'opsz' axis, in pixels
    std::optional<int16_t> opticalSize; // default uses font's pixel size
    // 'wdth' axis
    int16_t width = 100; // 100% by default
    // 'ital' axis
    bool italic = false; // no italics by default
    // 'slnt' axis
    int16_t slant = 0; // no slant by default
    // 'GRAD' axis
    int16_t grade = 0; // no change by default
  };

  class Font
  {
  public:
    struct Glyph
    {
      Glyph() = default;
      Glyph(Glyph&&) = default;

      RawImage2D<uint8_t> image;
      // offset from pen point to left-top corner on canvas
      ivec2 offset;
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

    virtual void Shape(std::string_view text, LanguageInfo const& languageInfo, std::vector<ShapedGlyph>& shapedGlyphs) const = 0;
    virtual std::vector<Glyph> CreateGlyphs(std::vector<GlyphWithOffset> const& glyphsNeeded, ivec2 const& offsetPrecision = { 1, 1 }) const = 0;

    static std::tuple<GlyphsPacking, RawImage2D<uint8_t>> PackGlyphs(std::vector<Glyph> const& glyphs, ivec2 const& size = { 4096, 4096 }, ivec2 const& offsetPrecision = { 1, 1 })
    {
      size_t glyphsCount = glyphs.size();

      std::vector<GlyphsPacking::GlyphInfo> glyphInfos(glyphsCount);

      // calculate placement of glyph images
      std::vector<ivec2> glyphSizes(glyphsCount);
      for(size_t i = 0; i < glyphsCount; ++i)
        glyphSizes[i] = glyphs[i].image.size;

      auto [glyphPositions, resultSize] = Image2DShelfUnion(glyphSizes, size.x(), 1);

      if(resultSize.x() > size.x() || resultSize.y() > size.y())
        throw Exception("result image is too big");

      // blit images into single image
      RawImage2D<uint8_t> glyphsImage(size);
      for(size_t i = 0; i < glyphsCount; ++i)
        glyphsImage.Blit(glyphs[i].image, glyphPositions[i], {}, glyphs[i].image.size);

      // set glyph positions
      for(size_t i = 0; i < glyphsCount; ++i)
      {
        auto& glyphInfo = glyphInfos[i];
        glyphInfo.size = glyphs[i].image.size;
        glyphInfo.leftTop = glyphPositions[i];
        glyphInfo.offset = glyphs[i].offset;
      }

      return
      {
        GlyphsPacking{
          .size = size,
          .glyphInfos = std::move(glyphInfos),
        },
        std::move(glyphsImage),
      };
    }
  };

  class FontSource
  {
  public:
    virtual Font& CreateFont(Book& book, int32_t size, FontVariableStyle const& style = {}) = 0;
  };

  template <>
  struct AssetTraits<FontSource*>
  {
    static constexpr std::string_view assetTypeName = "font";
  };
  static_assert(IsAsset<FontSource*>);
}
