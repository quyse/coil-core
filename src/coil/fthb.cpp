#include "fthb.hpp"
#include <bit>

namespace
{
  using namespace Coil;

  class FtEngine
  {
  public:
    FtEngine()
    {
      if(FT_Init_FreeType(&_library))
        throw Exception("initializing FreeType failed");
    }

    ~FtEngine()
    {
      FT_Done_FreeType(_library);
    }

    static FT_Library GetLibrary()
    {
      static FtEngine engine;
      return engine._library;
    }

  private:
    FT_Library _library;
  };

  class FtFontFace
  {
  public:
    FtFontFace(FT_Face ftFace)
    : _ftFace(ftFace) {}

    ~FtFontFace()
    {
      FT_Done_Face(_ftFace);
    }

  private:
    FT_Face _ftFace;
  };

  class HbFont
  {
  public:
    HbFont(hb_font_t* hbFont)
    : _hbFont(hbFont) {}

    ~HbFont()
    {
      hb_font_destroy(_hbFont);
    }

  private:
    hb_font_t* _hbFont;
  };

  class HbBuffer
  {
  public:
    HbBuffer(hb_buffer_t* hbBuffer)
    : _hbBuffer(hbBuffer) {}

    ~HbBuffer()
    {
      hb_buffer_destroy(_hbBuffer);
    }

  private:
    hb_buffer_t* _hbBuffer;
  };
}

namespace Coil
{
  FtHbFont::FtHbFont(FT_Face ftFace, hb_font_t* hbFont, hb_buffer_t* hbBuffer, int32_t size)
  : _ftFace(ftFace), _hbFont(hbFont), _hbBuffer(hbBuffer), _size(size) {}

  FtHbFont& FtHbFont::Load(Book& book, Buffer const& buffer, int32_t size)
  {
    FT_Face ftFace = LoadFace(book, buffer, size);
    FT_Face ftImmutableFace = LoadFace(book, buffer, size);

    hb_font_t* hbFont = hb_ft_font_create(ftImmutableFace, nullptr);
    book.Allocate<HbFont>(hbFont);

    hb_ft_font_set_funcs(hbFont);
    hb_ft_font_set_load_flags(hbFont, FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT);

    hb_buffer_t* hbBuffer = hb_buffer_create();
    book.Allocate<HbBuffer>(hbBuffer);

    return book.Allocate<FtHbFont>(ftFace, hbFont, hbBuffer, size);
  }

  void FtHbFont::Shape(std::string const& text, LanguageInfo const& languageInfo, std::vector<ShapedGlyph>& shapedGlyphs) const
  {
    // get language
    hb_language_t language;
    {
      auto i = _languagesCache.find(languageInfo.tag);
      if(i != _languagesCache.end()) [[likely]]
      {
        language = i->second;
      }
      else
      {
        char s[sizeof(languageInfo.tag)];
        size_t j = 0;
        for(size_t i = 0; i < sizeof(languageInfo.tag); ++i)
        {
          char c = (char)(((uint16_t)languageInfo.tag >> ((sizeof(languageInfo.tag) - i - 1) * 8)) & 0x7F);
          if(c)
            s[j++] = c;
        }
        language = hb_language_from_string(s, (int)j);
        _languagesCache.insert({ languageInfo.tag, language });
      }
    }
    hb_buffer_set_language(_hbBuffer, language);
    hb_buffer_set_script(_hbBuffer, (hb_script_t)languageInfo.script);
    switch(languageInfo.direction)
    {
    case LanguageDirection::LeftToRight:
      hb_buffer_set_direction(_hbBuffer, HB_DIRECTION_LTR);
      break;
    case LanguageDirection::RightToLeft:
      hb_buffer_set_direction(_hbBuffer, HB_DIRECTION_RTL);
      break;
    }
    hb_buffer_add_utf8(_hbBuffer, text.c_str(), (int)text.length(), 0, (int)text.length());
    hb_shape(_hbFont, _hbBuffer, nullptr, 0);
    hb_buffer_normalize_glyphs(_hbBuffer);

    unsigned int glyphCount;
    hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos(_hbBuffer, &glyphCount);
    hb_glyph_position_t* glyphPositions = hb_buffer_get_glyph_positions(_hbBuffer, &glyphCount);

    vec2 position;
    for(uint32_t i = 0; i < glyphCount; ++i)
    {
      vec2 advance(float(glyphPositions[i].x_advance) / 64, float(glyphPositions[i].y_advance) / 64);

      shapedGlyphs.push_back(
      {
        .position = position + vec2(float(glyphPositions[i].x_offset) / 64, float(glyphPositions[i].y_offset) / 64),
        .advance = advance,
        .glyphIndex = glyphInfos[i].codepoint,
        .characterIndex = glyphInfos[i].cluster,
      });

      position = position + advance;
    }

    hb_buffer_reset(_hbBuffer);
  }

  std::tuple<GlyphsPacking, RawImage2D<uint8_t>> FtHbFont::PackGlyphs(CreateGlyphsConfig const& config) const
  {
    try
    {
      auto const& glyphsNeeded = config.glyphsNeeded;
      auto const& offsetPrecision = config.offsetPrecision;

      size_t const glyphsCount = glyphsNeeded.size();

      if(FT_Set_Pixel_Sizes(_ftFace, _size * offsetPrecision.x(), _size * offsetPrecision.y()))
        throw Exception("setting pixel sizes failed");

      std::vector<GlyphsPacking::GlyphInfo> glyphInfos(glyphsCount);
      std::vector<RawImage2D<uint8_t>> glyphImages(glyphsCount);

      for(size_t glyphIndex = 0; glyphIndex < glyphsCount; ++glyphIndex)
      {
        auto const& glyph = glyphsNeeded[glyphIndex];
        ivec2 const glyphPrecisionOffset = { glyph.offsetX, glyph.offsetY };
        // set transform for glyph
        FT_Vector delta =
        {
          .x = (int32_t)(glyph.offsetX << 6),
          .y = (int32_t)(glyph.offsetY << 6),
        };
        FT_Set_Transform(_ftFace, nullptr, &delta);

        if(FT_Load_Glyph(_ftFace, glyphsNeeded[glyphIndex].index, FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT))
          throw Exception("loading glyph failed");

        if(FT_Render_Glyph(_ftFace->glyph, FT_RENDER_MODE_NORMAL))
          throw Exception("rendering glyph failed");

        GlyphsPacking::GlyphInfo& glyphInfo = glyphInfos[glyphIndex];

        const FT_Bitmap& bitmap = _ftFace->glyph->bitmap;
        ivec2 bitmapSize((int32_t)bitmap.width, (int32_t)bitmap.rows);

        if(bitmapSize.x() > 0 && bitmapSize.y() > 0)
        {
          if(bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
            throw Exception("glyph pixel mode must be gray");

          RawImageSlice2D<uint8_t> bitmapImage
          {
            .pixels = bitmap.pitch >= 0 ? bitmap.buffer : (bitmap.buffer + (bitmap.rows - 1) * bitmap.pitch),
            .pitch = { 1, bitmap.pitch },
            .size = bitmapSize,
          };

          ivec2 glyphOffset
          {
            (int32_t)_ftFace->glyph->bitmap_left,
            -(int32_t)_ftFace->glyph->bitmap_top,
          };
          ivec2 glyphLeftTop = glyphOffset;
          ivec2 glyphRightBottom = glyphLeftTop + bitmapSize;
          ivec2 boxLeftTop
          {
            divFloor(glyphLeftTop.x(), offsetPrecision.x()),
            divFloor(glyphLeftTop.y(), offsetPrecision.y()),
          };
          ivec2 boxRightBottom
          {
            divCeil(glyphRightBottom.x(), offsetPrecision.x()),
            divCeil(glyphRightBottom.y(), offsetPrecision.y()),
          };

          RawImage2D<uint8_t> glyphImage((boxRightBottom - boxLeftTop) * offsetPrecision);
          glyphImage.Blit(bitmapImage, glyphLeftTop - boxLeftTop * offsetPrecision, {}, bitmapSize);

          if(offsetPrecision.x() > 1 || offsetPrecision.y() > 1)
          {
            glyphImage = glyphImage.DownSample<uint32_t>(offsetPrecision);
          }

          glyphImages[glyphIndex] = std::move(glyphImage);
          glyphInfo.size = glyphImages[glyphIndex].size;
          glyphInfo.offset = boxLeftTop;
        }
      }

      // calculate placement of glyph images
      std::vector<ivec2> glyphSizes(glyphsCount);
      for(size_t i = 0; i < glyphsCount; ++i)
        glyphSizes[i] = glyphImages[i].size;

      auto [glyphPositions, resultSize] = Image2DShelfUnion(glyphSizes, config.maxSize.x(), 1);

      // make size power of two
      resultSize =
      {
        (int32_t)std::bit_ceil<uint32_t>(resultSize.x()),
        (int32_t)std::bit_ceil<uint32_t>(resultSize.y()),
      };

      if(resultSize.y() > config.maxSize.y())
        throw Exception("result image is too big");

      // blit images into single image
      RawImage2D<uint8_t> glyphsImage(resultSize);
      for(size_t i = 0; i < glyphsCount; ++i)
        glyphsImage.Blit(glyphImages[i], glyphPositions[i], {}, glyphImages[i].size);

      // set glyph positions
      for(FT_Long i = 0; i < glyphsCount; ++i)
        glyphInfos[i].leftTop = glyphPositions[i];

      return
      {
        GlyphsPacking{
          .size = resultSize,
          .glyphInfos = std::move(glyphInfos),
        },
        std::move(glyphsImage),
      };
    }
    catch(Exception const& exception)
    {
      throw Exception("packing FreeType/Harfbuzz glyphs failed") << exception;
    }
  }

  FT_Face FtHbFont::LoadFace(Book& book, Buffer const& buffer, int32_t size)
  {
    FT_Face ftFace;
    if(FT_New_Memory_Face(FtEngine::GetLibrary(), (const FT_Byte*)buffer.data, (FT_Long)buffer.size, 0, &ftFace))
      throw Exception("creating FreeType font face failed");

    book.Allocate<FtFontFace>(ftFace);

    if(FT_Set_Pixel_Sizes(ftFace, size, size))
      throw Exception("setting FreeType font size failed");

    return ftFace;
  }
}
