module;

#include <tuple>

module coil.core.render.realistic;

namespace Coil
{
  void RealisticRenderer::InitScreenQuadMesh(Book& book, GraphicsPool& pool)
  {
    ShaderDataIdentityStruct<ScreenVertex> const vertices[] =
    {
      { { -1, -1 } },
      { { -1,  1 } },
      { {  1,  1 } },
      { {  1, -1 } },
    };
    uint16_t const indices[] =
    {
      0, 1, 2,
      0, 2, 3,
    };
    pScreenQuadMesh_ = &book.Allocate<GraphicsMesh>(
      graphicsDevice_.CreateVertexBuffer(book, pool, vertices),
      graphicsDevice_.CreateIndexBuffer(book, pool, indices, false),
      6
    );
  }

  void RealisticRenderer::InitToneMappingShaders(Book& book)
  {
    auto const& uniformScene = GetShaderDataStructBuffer<SceneUniform, 0, 0, ShaderBufferType::Uniform>();

    pToneMappingShader_ = &graphicsDevice_.CreateShader(book, [&]() -> GraphicsShaderRoots
    {
      auto const& [vertex] = GetScreenVertexLayout().slots;

      ShaderSampledImage<vec3, 2> hdrColorImage(0, 0);

      using namespace Shaders;

      // vertex inputs
      auto vInPosition = *vertex.position;

      // interpolants
      auto iOutPosition = ShaderInterpolantBuiltinPosition();
      auto iScreenCoord = ShaderInterpolant<vec2>(0);

      auto vertexProgram =
      (
        iOutPosition.Write(cvec(vInPosition, float_(0), float_(1))),
        iScreenCoord.Write(vInPosition * vec2_(0.5f, 0.5f) + vec2_(0.5f, 0.5f))
      );

      // fragment inputs
      auto fScreenCoord = *iScreenCoord;

      // sample HDR image
      auto fHdrColor = hdrColorImage.Sample(fScreenCoord);

      // tone map
      auto fLdrColor = fHdrColor / (fHdrColor + vec3_(1, 1, 1));

      // fragment outputs
      auto fOutColor = ShaderFragment<vec4>(0);

      auto fragmentProgram =
      (
        fOutColor.Write(cvec(fLdrColor, float_(1)))
      );

      return
      {
        .vertex = vertexProgram,
        .fragment = fragmentProgram,
      };
    }());

    {
      GraphicsShader* shaders[] = { pToneMappingShader_ };
      pToneMappingPipelineLayout_ = &graphicsDevice_.CreatePipelineLayout(book, shaders);
    }
  }
}
