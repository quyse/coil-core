#pragma once

#include "fonts.hpp"
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

    GlyphsPacking PackGlyphs(CreateGlyphsConfig const& config) override;

  private:
    static FT_Face LoadFace(Book& book, Buffer const& buffer, int32_t size);

    FT_Face _ftFace;
    hb_font_t* _hbFont;
    hb_buffer_t* _hbBuffer;
    int32_t _size;
  };
}
