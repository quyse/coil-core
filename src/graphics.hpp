#pragma once

#include "platform.hpp"
#include "graphics_format.hpp"
#include "graphics_shaders.hpp"

namespace Coil
{
  class GraphicsDevice;

  class GraphicsSystem
  {
  public:
    virtual GraphicsDevice& CreateDefaultDevice(Book& book) = 0;
  };

  class GraphicsFrame
  {
  public:
    virtual void EndFrame() = 0;
  };

  // Represents memory pool.
  class GraphicsPool
  {
  public:
    Book& GetBook();

  protected:
    Book _book;
  };

  class GraphicsPresenter
  {
  public:
    virtual void Resize(ivec2 const& size) = 0;
    virtual GraphicsFrame& StartFrame() = 0;
  };

  class GraphicsVertexBuffer
  {
  };

  class GraphicsShader
  {
  public:
    enum GraphicsStageFlag
    {
      VertexStageFlag = 1,
      FragmentStageFlag = 2,
    };
  };

  struct GraphicsShaderRoots
  {
    std::shared_ptr<ShaderStatementNode> vertex;
    std::shared_ptr<ShaderStatementNode> fragment;
  };

  class GraphicsPipeline
  {
  };

  // Function type for recreating resources for present pass.
  // Accepts special presenter book (which gets freed on next recreate), and pixel size of final frame.
  using GraphicsRecreatePresentPassFunc = void(Book&, ivec2 const&);

  class GraphicsDevice
  {
  public:
    virtual GraphicsPool& CreatePool(Book& book, uint64_t chunkSize) = 0;
    virtual GraphicsPresenter& CreateWindowPresenter(Book& book, Window& window, std::function<GraphicsRecreatePresentPassFunc>&& recreatePresentPass) = 0;
    virtual GraphicsVertexBuffer& CreateVertexBuffer(GraphicsPool& pool, Buffer const& buffer) = 0;
    virtual GraphicsShader& CreateShader(Book& book, GraphicsShaderRoots const& exprs) = 0;
  };
}
