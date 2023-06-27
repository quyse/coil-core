#pragma once

#include "graphics.hpp"

namespace Coil
{
  // helper for 2D rendering
  class Canvas
  {
  public:
    Canvas(GraphicsDevice& device);

    void Init(Book& book, GraphicsPool& pool);
    void SetSize(ivec2 const& size);
    ivec2 GetSize() const;

    GraphicsMesh& GetQuadMesh() const;

    GraphicsPipeline& CreateQuadPipeline(Book& book, GraphicsPipelineLayout& pipelineLayout, GraphicsPass& pass, GraphicsSubPassId subPassId, GraphicsShader& shader);
    void InitQuadSubPass(GraphicsPassConfig& config, GraphicsPassConfig::AttachmentRef inputAttachment, GraphicsPassConfig::AttachmentRef outputAttachment);

    template <template <typename> typename T>
    struct ScreenQuadVertex
    {
      T<vec2_ua> position;
    };

    static auto const& GetScreenQuadVertexLayout()
    {
      static auto const layout = GetGraphicsVertexStructLayout<ScreenQuadVertex>();
      return layout;
    }

  private:
    GraphicsDevice& _device;
    GraphicsMesh* _pQuadMesh = nullptr;
    ivec2 _size;
  };
}
