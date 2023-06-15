#include "fonts.hpp"
#include <bit>

namespace Coil
{
  std::tuple<GlyphsPacking, RawImage2D<uint8_t>> Font::PackGlyphs(std::vector<Glyph> const& glyphs, ivec2 const& size, ivec2 const& offsetPrecision)
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
}
