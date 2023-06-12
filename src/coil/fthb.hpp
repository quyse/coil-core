#pragma once

#include "fonts.hpp"
#include <unordered_map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

namespace Coil
{
  class FtHbFont : public Font
  {
  public:
    FtHbFont(FT_Face ftFace, hb_font_t* hbFont, hb_buffer_t* hbBuffer, int32_t size);

    static FtHbFont& Load(Book& book, Buffer const& buffer, int32_t size);

    void Shape(std::string const& text, LanguageInfo const& languageInfo, std::vector<ShapedGlyph>& shapedGlyphs) const override;
    std::vector<Glyph> CreateGlyphs(std::vector<GlyphWithOffset> const& glyphsNeeded, ivec2 const& offsetPrecision) const override;

  private:
    static FT_Face LoadFace(Book& book, Buffer const& buffer, int32_t size);

    FT_Face const _ftFace;
    hb_font_t* const _hbFont;
    hb_buffer_t* const _hbBuffer;
    int32_t const _size;

    mutable std::unordered_map<LanguageTag, hb_language_t> _languagesCache;
  };
}
