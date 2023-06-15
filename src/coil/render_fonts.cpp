#include "render_fonts.hpp"
#include "graphics_shaders.hpp"

namespace Coil
{
  auto const& FontRenderer::GetGlyphVertexLayout()
  {
    static auto const layout = GetGraphicsVertexStructLayout<GlyphVertex, GlyphInstance>();
    return layout;
  }

  FontRenderer::FontRenderer(GraphicsDevice& device, FontGlyphCache&& glyphCache)
  : _device(device), _glyphCache(std::move(glyphCache)) {}

  void FontRenderer::Init(Book& book, GraphicsPool& pool)
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
      auto fColor = *iColor * fAlpha;

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

  void FontRenderer::InitPipeline(Book& book, ivec2 canvasSize, GraphicsPass& pass, GraphicsSubPassId subPassId)
  {
    _canvasSize = canvasSize;
    {
      vec2 invCanvasSize = 2.0f / vec2(_canvasSize);
      _invCanvasSize4 = vec4(invCanvasSize.x(), invCanvasSize.y(), invCanvasSize.x(), invCanvasSize.y());
    }

    GraphicsPipelineConfig config;

    config.viewport = canvasSize;
    config.depthTest = false;
    config.depthWrite = false;
    config.vertexLayout = GetGlyphVertexLayout().layout;
    config.vertexLayout.slots[1].perInstance = true;
    config.attachments.push_back(
    {
      .blending = GraphicsPipelineConfig::Blending {},
    });
    _pGlyphPipeline = &_device.CreatePipeline(book, config, *_pGlyphPipelineLayout, pass, subPassId, *_pGlyphShader);
  }

  void FontRenderer::InitFrame(GraphicsContext& context)
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

  void FontRenderer::PrepareRender(Font const& font, std::string const& text, LanguageInfo const& languageInfo, vec2 const& textOffset)
  {
    _glyphCache.ShapeText(font, text, languageInfo, textOffset, _tempRenderGlyphs);
    _tempRenderGlyphs.clear();
  }

  void FontRenderer::Render(Cache& cache, Font const& font, std::string const& text, LanguageInfo const& languageInfo, vec2 const& textOffset, vec4 const& color)
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

      cache.Render(*_pGlyphTexture, *_pGlyphPipeline, *_pGlyphMesh, ShaderDataIdentityStruct<GlyphInstance>
      {
        .position = vec4(positionLeftTop.x(), positionLeftTop.y(), positionRightBottom.x(), positionRightBottom.y()) * _invCanvasSize4 + vec4(-1.0f, -1.0f, -1.0f, -1.0f),
        .texcoord = vec4(textureLeftTop.x(), textureLeftTop.y(), textureRightBottom.x(), textureRightBottom.y()) * invTextureSize4,
        .color = color,
      });
    }
    _tempRenderGlyphs.clear();
  }
}
