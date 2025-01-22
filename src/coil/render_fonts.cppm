module;

#include <string_view>

export module coil.core.render.fonts;

import coil.core.base;
import coil.core.fonts.cache;
import coil.core.fonts;
import coil.core.graphics.shaders;
import coil.core.graphics;
import coil.core.image.format;
import coil.core.localization;
import coil.core.math;
import coil.core.render;

export namespace Coil
{
  class FontRenderer
  {
  public:
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

  private:
    static auto const& GetGlyphVertexLayout()
    {
      static auto const layout = GetGraphicsVertexStructLayout<GlyphVertex, GlyphInstance>();
      return layout;
    }

  public:
    FontRenderer(GraphicsDevice& device, FontGlyphCache&& glyphCache = {})
    : _device(device), _glyphCache(std::move(glyphCache)) {}

    void Init(Book& book, GraphicsPool& pool)
    {
      {
        ShaderDataIdentityStruct<GlyphVertex> const vertices[] =
        {
          { { 1, 1, 0, 0 } },
          { { 1, 0, 0, 1 } },
          { { 0, 0, 1, 1 } },
          { { 0, 1, 1, 0 } },
        };
        uint16_t const indices[] = { 0, 1, 2, 0, 2, 3, };
        _pGlyphMesh = &book.Allocate<GraphicsMesh>(
          _device.CreateVertexBuffer(book, pool, vertices),
          _device.CreateIndexBuffer(book, pool, indices, false),
          6
        );
      }

      _pGlyphShader = &_device.CreateShader(book, [&]() -> GraphicsShaderRoots
      {
        using namespace Shaders;

        auto const& [vertex, instance] = GetGlyphVertexLayout().slots;

        ShaderSampledImage<float, 2> image(0, 1);

        auto vInCorner = *vertex.corner;
        auto vInPosition = *instance.position;
        auto vInTexcoord = *instance.texcoord;
        auto vInColor = *instance.color;

        auto iOutPosition = ShaderInterpolantBuiltinPosition();
        auto iTexcoord = ShaderInterpolant<vec2>(0);
        auto iColor = ShaderInterpolant<vec4>(1);

        auto vertexProgram = (
          iOutPosition.Write(cvec(
            dot(swizzle(vInCorner, "xz"), swizzle(vInPosition, "xz")),
            dot(swizzle(vInCorner, "yw"), swizzle(vInPosition, "yw")),
            float_(0),
            float_(1))
          ),
          iTexcoord.Write(cvec(
            dot(swizzle(vInCorner, "xz"), swizzle(vInTexcoord, "xz")),
            dot(swizzle(vInCorner, "yw"), swizzle(vInTexcoord, "yw"))
          )),
          iColor.Write(vInColor)
        );

        auto fOutColor = ShaderFragment<vec4>(0);

        auto fAlpha = image.Sample(*iTexcoord);
        auto fColor = *iColor;
        fColor = cvec(swizzle(fColor, "xyz"), float_(1)) * (swizzle(fColor, "w") * fAlpha);

        auto fragmentProgram = (
          fOutColor.Write(fColor)
        );

        return
        {
          .vertex = vertexProgram,
          .fragment = fragmentProgram,
        };
      }());

      {
        GraphicsShader* shaders[] = { _pGlyphShader };
        _pGlyphPipelineLayout = &_device.CreatePipelineLayout(book, shaders);
      }

      GraphicsSampler& glyphSampler = _device.CreateSampler(book,
      {
        .wrapU = GraphicsSamplerConfig::Wrap::Border,
        .wrapV = GraphicsSamplerConfig::Wrap::Border,
        .wrapW = GraphicsSamplerConfig::Wrap::Border,
      });
      auto size = _glyphCache.GetSize();
      _pGlyphTexture = &_device.CreateTexture(book, pool,
      {
        .format = PixelFormats::uintR8,
        .width = size.x(),
        .height = size.y(),
        .depth = 0,
        .mips = 1,
        .count = 0,
      }, &glyphSampler);
    }

    void InitPipeline(Book& book, ivec2 canvasSize, GraphicsPass& pass, GraphicsSubPassId subPassId)
    {
      _canvasSize = canvasSize;
      {
        vec2 invCanvasSize = 2.0f / vec2(_canvasSize);
        _invCanvasSize4 = vec4(invCanvasSize.x(), invCanvasSize.y(), invCanvasSize.x(), invCanvasSize.y());
      }

      GraphicsPipelineConfig config =
      {
        .viewport = canvasSize,
        .depthTest = false,
        .depthWrite = false,
        .vertexLayout = GetGlyphVertexLayout().layout,
        .attachments =
        {
          {
            .blending = GraphicsPipelineConfig::Blending {},
          },
        },
      };
      config.vertexLayout.slots[1].perInstance = true;
      _pGlyphPipeline = &_device.CreatePipeline(book, config, *_pGlyphPipelineLayout, pass, subPassId, *_pGlyphShader);
    }

    void InitFrame(GraphicsContext& context)
    {
      if(_glyphCache.Update())
      {
        auto size = _glyphCache.GetSize();
        context.SetTextureData(*_pGlyphTexture,
        {
          .format =
          {
            .format = PixelFormats::uintR8,
            .width = size.x(),
            .height = size.y(),
            .depth = 0,
            .mips = 1,
            .count = 0,
          },
          .buffer = _glyphCache.GetImage()
        });
      }
    }

    using Cache = RenderCache<
      RenderPipelineKnob,
      RenderMeshKnob,
      RenderImageKnob<0, 1>,
      RenderInstanceDataKnob<ShaderDataIdentityStruct<GlyphInstance>>
    >;

    void PrepareRender(Font const& font, std::string_view text, LanguageInfo const& languageInfo, vec2 const& textOffset)
    {
      _glyphCache.ShapeText(font, text, languageInfo, textOffset, _tempRenderGlyphs);
      _tempRenderGlyphs.clear();
    }

    void Render(Cache& cache, Font const& font, std::string_view text, LanguageInfo const& languageInfo, vec2 const& textOffset, vec4 const& color)
    {
      _glyphCache.ShapeText(font, text, languageInfo, textOffset, _tempRenderGlyphs);

      ivec2 textureSize = _glyphCache.GetSize();
      vec2 invTextureSize = 1.0f / vec2(textureSize);
      vec4 invTextureSize4 = { invTextureSize.x(), invTextureSize.y(), invTextureSize.x(), invTextureSize.y() };

      for(size_t i = 0; i < _tempRenderGlyphs.size(); ++i)
      {
        auto const& renderGlyph = _tempRenderGlyphs[i];

        auto optGlyphInfo = _glyphCache.GetGlyphInfo(font, renderGlyph.glyphWithOffset);
        if(!optGlyphInfo) continue;
        auto const& glyphInfo = optGlyphInfo.value();

        ivec2 positionLeftTop = renderGlyph.position + glyphInfo.offset;
        ivec2 positionRightBottom = positionLeftTop + glyphInfo.size;
        ivec2 textureLeftTop = glyphInfo.leftTop;
        ivec2 textureRightBottom = textureLeftTop + glyphInfo.size;

        cache.Render(*_pGlyphPipeline, *_pGlyphMesh, *_pGlyphTexture, ShaderDataIdentityStruct<GlyphInstance>
        {
          .position = vec4(positionLeftTop.x(), positionLeftTop.y(), positionRightBottom.x(), positionRightBottom.y()) * _invCanvasSize4 + vec4(-1.0f, -1.0f, -1.0f, -1.0f),
          .texcoord = vec4(textureLeftTop.x(), textureLeftTop.y(), textureRightBottom.x(), textureRightBottom.y()) * invTextureSize4,
          .color = color,
        });
      }

      _tempRenderGlyphs.clear();
    }

  private:
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
