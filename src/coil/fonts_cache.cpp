#include "fonts_cache.hpp"
#include <unordered_map>

namespace Coil
{
  FontGlyphCache::FontGlyphCache(ivec2 const& offsetPrecision, ivec2 const& size)
  : _offsetPrecision(offsetPrecision), _size(size) {}

  void FontGlyphCache::ShapeText(Font const& font, std::string const& text, LanguageInfo const& languageInfo, vec2 const& textOffset, std::vector<RenderGlyph>& renderGlyphs)
  {
    font.Shape(text, languageInfo, _tempShapedGlyphs);
    for(size_t i = 0; i < _tempShapedGlyphs.size(); ++i)
    {
      auto const& shapedGlyph = _tempShapedGlyphs[i];
      vec2 position = textOffset + shapedGlyph.position;
      vec2 flooredPosition = { std::floor(position.x()), std::floor(position.y()) };
      vec2 offset = position - flooredPosition;
      uint8_t offsetX = (uint8_t)std::floor(offset.x() * (float)_offsetPrecision.x());
      uint8_t offsetY = (uint8_t)std::floor(offset.y() * (float)_offsetPrecision.y());

      GlyphWithOffset glyphWithOffset =
      {
        .index = shapedGlyph.glyphIndex,
        .offsetX = offsetX,
        .offsetY = offsetY,
      };

      renderGlyphs.push_back(
      {
        .position = (ivec2)flooredPosition,
        .glyphWithOffset = glyphWithOffset,
        .characterIndex = shapedGlyph.characterIndex,
      });

      auto [j, inserted] = _glyphsMapping.insert({ { &font, glyphWithOffset }, -1 });
      if(inserted || j->second < 0)
      {
        // no glyph in cache, mark cache dirty
        _dirty = true;
      }
    }
    _tempShapedGlyphs.clear();
  }

  bool FontGlyphCache::Update()
  {
    if(!_dirty) return false;

    std::unordered_map<Font const*, std::pair<std::vector<GlyphWithOffset>, std::vector<int32_t*>>> glyphsNeeded;
    for(auto i = _glyphsMapping.begin(); i != _glyphsMapping.end(); ++i)
    {
      i->second = (int32_t)glyphsNeeded.size();
      glyphsNeeded[i->first.first].first.push_back(i->first.second);
      glyphsNeeded[i->first.first].second.push_back(&i->second);
    }

    std::vector<Font::Glyph> glyphs;
    for(auto i = glyphsNeeded.begin(); i != glyphsNeeded.end(); ++i)
    {
      auto fontGlyphs = i->first->CreateGlyphs(i->second.first, _offsetPrecision);
      int32_t mappingIndex = (int32_t)glyphs.size();
      for(size_t j = 0; j < fontGlyphs.size(); ++j)
        *i->second.second[j] = mappingIndex++;
      for(size_t j = 0; j < fontGlyphs.size(); ++j)
        glyphs.push_back(std::move(fontGlyphs[j]));
    }

    try
    {
      auto [packing, rawImage] = Font::PackGlyphs(glyphs, _size, _offsetPrecision);

      _image = std::move(rawImage);
      _glyphsPacking = std::move(packing.glyphInfos);

    }
    catch(Exception& exception)
    {
      // couldn't pack glyphs
      // clear desired glyphs, in hope they will be repopulated on next frame
      _glyphsMapping.clear();
      _glyphsPacking.clear();
    }

    _dirty = false;

    return true;
  }

  RawImage2D<uint8_t> const& FontGlyphCache::GetImage() const
  {
    return _image;
  }

  std::optional<GlyphsPacking::GlyphInfo> FontGlyphCache::GetGlyphInfo(Font const& font, GlyphWithOffset const& glyphWithOffset) const
  {
    auto i = _glyphsMapping.find({ &font, glyphWithOffset });
    if(i == _glyphsMapping.end()) return {};
    int32_t glyphIndex = i->second;
    if(glyphIndex < 0) return {};
    return _glyphsPacking[glyphIndex];
  }
}
