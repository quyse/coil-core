#include "render_canvas.hpp"

namespace Coil
{
  Canvas::Canvas(GraphicsDevice& device)
  : _device(device) {}

  void Canvas::SetSize(ivec2 const& size)
  {
    _size = size;
  }
  ivec2 Canvas::GetSize() const
  {
    return _size;
  }

  GraphicsMesh& Canvas::GetQuadMesh() const
  {
    return *_pQuadMesh;
  }

  void Canvas::Init(Book& book, GraphicsPool& pool)
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

  GraphicsPipeline& Canvas::CreateQuadPipeline(Book& book, GraphicsPipelineLayout& pipelineLayout, GraphicsPass& pass, GraphicsSubPassId subPassId, GraphicsShader& shader)
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

  void Canvas::InitQuadSubPass(GraphicsPassConfig& config, GraphicsPassConfig::AttachmentRef inputAttachment, GraphicsPassConfig::AttachmentRef outputAttachment)
  {
    auto subPass = config.AddSubPass();
    subPass->UseInputAttachment(inputAttachment, 0);
    subPass->UseColorAttachment(outputAttachment, 0);
  }
}
