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

    hb_buffer_t* hbBuffer = hb_buffer_create();
    book.Allocate<HbBuffer>(hbBuffer);

    return book.Allocate<FtHbFont>(ftFace, hbFont, hbBuffer, size);
  }

  void FtHbFont::Shape(std::string const& text, LanguageInfo const& languageInfo, std::vector<OutGlyph>& outGlyphs) const
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

      outGlyphs.push_back(
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
      ivec2 halfScale = config.halfScale;
      auto const& glyphsNeeded = config.glyphsNeeded;

      FT_Long glyphsCount = glyphsNeeded.has_value() ? (FT_Long)glyphsNeeded->size() : _ftFace->num_glyphs;

      if(FT_Set_Pixel_Sizes(_ftFace, _size * (halfScale.x() * 2 + 1), _size * (halfScale.y() * 2 + 1)))
        throw Exception("setting pixel sizes failed");

      std::vector<GlyphsPacking::GlyphInfo> glyphInfos(glyphsCount);
      std::vector<RawImage2D<uint8_t>> glyphImages(glyphsCount);

      // reset transform if packing all glyphs
      if(!glyphsNeeded.has_value())
      {
        FT_Set_Transform(_ftFace, nullptr, nullptr);
      }

      for(FT_Long glyphIndex = 0; glyphIndex < glyphsCount; ++glyphIndex)
      {
        // set transform if packing specific glyphs
        if(glyphsNeeded.has_value())
        {
          auto const& glyph = glyphsNeeded.value()[glyphIndex];
          FT_Matrix mat =
          {
            .xx = (1 << 16),
            .xy = (1 << 14),
            .yx = 0,
            .yy = (1 << 16),
          };
          FT_Vector delta =
          {
            .x = (int32_t)glyph.offsetX * (1 << 6) / config.offsetPrecision.x(),
            .y = (int32_t)glyph.offsetY * (1 << 6) / config.offsetPrecision.y(),
          };
          FT_Set_Transform(_ftFace, nullptr, &delta);
        }

        if(FT_Load_Glyph(_ftFace, glyphsNeeded.has_value() ? glyphsNeeded.value()[glyphIndex].index : glyphIndex, config.enableHinting ? FT_LOAD_DEFAULT : (FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT)))
          throw Exception("loading glyph failed");

        if(FT_Render_Glyph(_ftFace->glyph, FT_RENDER_MODE_NORMAL))
          throw Exception("rendering glyph failed");

        const FT_Bitmap& bitmap = _ftFace->glyph->bitmap;

        int32_t pixelsWidth = (int32_t)bitmap.width + halfScale.x() * 2;
        int32_t pixelsHeight = (int32_t)bitmap.rows + halfScale.y() * 2;

        if(bitmap.width > 0 && bitmap.rows > 0)
        {
          if(bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
            throw Exception("glyph pixel mode must be gray");

          RawImage2D<uint8_t> glyphImage = RawImageSlice2D<uint8_t>
          {
            .pixels = bitmap.pitch >= 0 ? bitmap.buffer : (bitmap.buffer + (bitmap.rows - 1) * bitmap.pitch),
            .pitch = { 1, bitmap.pitch },
            .size =
            {
              (int32_t)bitmap.width,
              (int32_t)bitmap.rows,
            },
          };

          if(halfScale.x() || halfScale.y())
          {
            ivec2 scaledSize =
            {
              (int32_t)(bitmap.width + halfScale.x() * 2),
              (int32_t)(bitmap.rows + halfScale.y() * 2),
            };

            // calculate rectangular partial sums
            int32_t pp = (int32_t)bitmap.width + 1;
            std::vector<uint32_t> ps((bitmap.rows + 1) * pp);
            {
              int32_t p = 0;
              for(int32_t i = 0; i < bitmap.rows; ++i)
              {
                for(int32_t j = 0; j < bitmap.width; ++j)
                  ps[p + pp + j + 1] = glyphImage({ j, i }) + ps[p + j + 1] + ps[p + pp + j] - ps[p + j];
                p += pp;
              }
            }
            // blur image
            RawImage2D<uint8_t> scaledGlyphImage(scaledSize);
            int32_t fullScale = (halfScale.x() * 2 + 1) * (halfScale.y() * 2 + 1);
            for(int32_t i = 0; i < pixelsHeight; ++i)
            {
              int32_t mini = std::max(i - halfScale.y() * 2, 0);
              int32_t maxi = std::min(i + 1, (int32_t)bitmap.rows);
              for(int32_t j = 0; j < pixelsWidth; ++j)
              {
                int32_t minj = std::max(j - halfScale.x() * 2, 0);
                int32_t maxj = std::min(j + 1, (int32_t)bitmap.width);
                scaledGlyphImage({ j, i }) = (uint8_t)((
                  ps[maxi * pp + maxj] + ps[mini * pp + minj] - ps[maxi * pp + minj] - ps[mini * pp + maxj]
                ) / fullScale);
              }
            }

            glyphImage = std::move(scaledGlyphImage);
          }
          glyphImages[glyphIndex] = std::move(glyphImage);
        }

        GlyphsPacking::GlyphInfo& glyphInfo = glyphInfos[glyphIndex];
        glyphInfo.size = glyphImages[glyphIndex].size;
        glyphInfo.offset =
        {
          (int32_t)_ftFace->glyph->bitmap_left + halfScale.x(),
          -(int32_t)_ftFace->glyph->bitmap_top + halfScale.y(),
        };
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
          .scale =
          {
            1 + halfScale.x() * 2,
            1 + halfScale.y() * 2,
          },
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
