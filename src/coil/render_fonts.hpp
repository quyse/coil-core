#pragma once

#include "fonts_cache.hpp"
#include "render.hpp"
#include <map>
#include <set>

namespace Coil
{
  class FontRenderer
  {
  public:
    FontRenderer(GraphicsDevice& device, FontGlyphCache&& glyphCache = {});

    void Init(Book& book, GraphicsPool& pool);
    void InitPipeline(Book& book, ivec2 canvasSize, GraphicsPass& pass, GraphicsSubPassId subPassId);
    void InitFrame(GraphicsContext& context);

    template <template <typename> typename T>
    struct GlyphVertex
    {
      // factors for positioning vertex (left, top, right, bottom)
      T<vec4_ua> corner;
    };

    template <template <typename> typename T>
    struct GlyphInstance
    {
      // left+top+right+bottom position of glyph, in clip coords
      T<vec4_ua> position;
      // left+top+right+bottom texcoord of glyph
      T<vec4_ua> texcoord;
      // color of glyph
      T<vec4_ua> color;
    };

    using Cache = RenderCache<
      RenderImageKnob<0, 1>,
      RenderPipelineKnob,
      RenderMeshKnob,
      RenderInstanceDataKnob<ShaderDataIdentityStruct<GlyphInstance>>
    >;

    void PrepareRender(Font const& font, std::string const& text, LanguageInfo const& languageInfo, vec2 const& textOffset);
    void Render(Cache& cache, Font const& font, std::string const& text, LanguageInfo const& languageInfo, vec2 const& textOffset, vec4 const& color);

  private:
    static auto const& GetGlyphVertexLayout();

    GraphicsDevice& _device;
    FontGlyphCache _glyphCache;

    GraphicsMesh* _pGlyphMesh = nullptr;
    GraphicsShader* _pGlyphShader = nullptr;
    GraphicsPipelineLayout* _pGlyphPipelineLayout = nullptr;
    GraphicsPipeline* _pGlyphPipeline = nullptr;
    ivec2 _canvasSize;
    vec4 _invCanvasSize4;
    GraphicsImage* _pGlyphTexture = nullptr;

    std::vector<FontGlyphCache::RenderGlyph> _tempRenderGlyphs;
  };
}
