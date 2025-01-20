module;

#include <cstdint>

export module coil.core.render.canvas;

import coil.core.base;
import coil.core.graphics.shaders;
import coil.core.graphics;
import coil.core.math;

export namespace Coil
{
  // helper for 2D rendering
  class Canvas
  {
  public:
    template <template <typename> typename T>
    struct ScreenQuadVertex
    {
      T<vec2_ua> position;
    };

  private:
    static auto const& GetScreenQuadVertexLayout()
    {
      static auto const layout = GetGraphicsVertexStructLayout<ScreenQuadVertex>();
      return layout;
    }

  public:
    Canvas(GraphicsDevice& device)
    : _device(device) {}

    void Init(Book& book, GraphicsPool& pool)
    {
      {
        ShaderDataIdentityStruct<ScreenQuadVertex> const vertices[] =
        {
          { { -1, -1 } },
          { { -1,  1 } },
          { {  1,  1 } },
          { {  1, -1 } },
        };
        uint16_t const indices[] = { 0, 1, 2, 0, 2, 3, };
        _pQuadMesh = &book.Allocate<GraphicsMesh>(
          _device.CreateVertexBuffer(book, pool, vertices),
          _device.CreateIndexBuffer(book, pool, indices, false),
          6
        );
      }
    }

    void SetSize(ivec2 const& size)
    {
      _size = size;
    }

    ivec2 GetSize() const
    {
      return _size;
    }

    GraphicsMesh& GetQuadMesh() const
    {
      return *_pQuadMesh;
    }

    GraphicsPipeline& CreateQuadPipeline(Book& book, GraphicsPipelineLayout& pipelineLayout, GraphicsPass& pass, GraphicsSubPassId subPassId, GraphicsShader& shader)
    {
      GraphicsPipelineConfig const config =
      {
        .viewport = _size,
        .depthTest = false,
        .depthWrite = false,
        .vertexLayout = GetScreenQuadVertexLayout().layout,
        .attachments = { {} },
      };
      return _device.CreatePipeline(book, config, pipelineLayout, pass, subPassId, shader);
    }

    void InitQuadSubPass(GraphicsPassConfig& config, GraphicsPassConfig::AttachmentRef inputAttachment, GraphicsPassConfig::AttachmentRef outputAttachment)
    {
      auto subPass = config.AddSubPass();
      subPass->UseInputAttachment(inputAttachment, 0);
      subPass->UseColorAttachment(outputAttachment, 0);
    }

  private:
    GraphicsDevice& _device;
    GraphicsMesh* _pQuadMesh = nullptr;
    ivec2 _size;
  };
}
