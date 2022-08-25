#pragma once

#include "platform.hpp"
#include "graphics_format.hpp"
#include "graphics_shaders.hpp"
#include <vector>
#include <map>
#include <optional>
#include <variant>
#include <span>

namespace Coil
{
  using GraphicsSubPassId = uint32_t;

  struct GraphicsPassConfig
  {
    using AttachmentId = uint32_t;

    struct Attachment
    {
      PixelFormat format;

      // keep attachment data from before pass
      // otherwise it will be cleared
      bool keepBefore = false;
      // keep attachment data after the pass
      // otherwise it will be undefined
      bool keepAfter = false;

      friend auto operator<=>(Attachment const&, Attachment const&) = default;
    };

    class AttachmentRef
    {
    public:
      AttachmentRef(GraphicsPassConfig& config, AttachmentId id);

      AttachmentId GetId() const;

      Attachment& operator*();
      Attachment const& operator*() const;
      Attachment* operator->();
      Attachment const* operator->() const;

    private:
      GraphicsPassConfig& _config;
      AttachmentId const _id;
    };

    struct SubPass
    {
      struct ColorAttachment
      {
        uint32_t slot;
      };
      struct DepthStencilAttachment
      {
      };
      struct InputAttachment
      {
        uint32_t slot;
      };
      struct ShaderAttachment
      {
      };
      using Attachment = std::variant<ColorAttachment, DepthStencilAttachment, InputAttachment, ShaderAttachment>;

      void UseColorAttachment(AttachmentRef attachmentRef, uint32_t slot);
      void UseDepthStencilAttachment(AttachmentRef attachmentRef);
      void UseInputAttachment(AttachmentRef attachmentRef, uint32_t slot);
      void UseShaderAttachment(AttachmentRef attachmentRef);

      std::map<AttachmentId, Attachment> attachments;
    };

    class SubPassRef
    {
    public:
      SubPassRef(GraphicsPassConfig& config, GraphicsSubPassId id);

      SubPass& operator*();
      SubPass const& operator*() const;
      SubPass* operator->();
      SubPass const* operator->() const;

    private:
      GraphicsPassConfig& _config;
      GraphicsSubPassId const _id;
    };

    AttachmentRef AddAttachment(PixelFormat format);

    SubPassRef AddSubPass();

    std::vector<Attachment> attachments;
    std::vector<SubPass> subPasses;
  };

  class GraphicsDevice;
  class GraphicsContext;
  class GraphicsPass;
  class GraphicsFramebuffer;

  class GraphicsSystem
  {
  public:
    virtual GraphicsDevice& CreateDefaultDevice(Book& book) = 0;
  };

  class GraphicsFrame
  {
  public:
    virtual uint32_t GetImageIndex() const = 0;
    virtual void Pass(GraphicsPass& pass, GraphicsFramebuffer& framebuffer, std::function<void(GraphicsSubPassId, GraphicsContext&)> const& func) = 0;
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

  class GraphicsPass
  {
  public:
  };

  class GraphicsVertexBuffer
  {
  };

  class GraphicsImage
  {
  };

  class GraphicsShader
  {
  public:
  };

  struct GraphicsShaderRoots
  {
    std::shared_ptr<ShaderStatementNode> vertex;
    std::shared_ptr<ShaderStatementNode> fragment;
  };

  class GraphicsPipelineLayout
  {
  };

  struct GraphicsPipelineConfig
  {
    struct VertexSlot
    {
      uint32_t stride = 0;
      bool perInstance = false;
    };
    std::vector<VertexSlot> vertexSlots;

    struct VertexAttribute
    {
      uint32_t location;
      uint32_t slot;
      uint32_t offset;
      VertexFormat format;
    };
    std::vector<VertexAttribute> vertexAttributes;
  };

  class GraphicsPipeline
  {
  };

  class GraphicsFramebuffer
  {
  };

  // Function type for recreating resources for present pass.
  // Accepts special presenter book (which gets freed on next recreate), pixel size of final image, and number of images.
  using GraphicsRecreatePresentFunc = void(Book&, ivec2 const&, uint32_t);
  // Function type for recreating resources for present pass per image.
  // Accepts special presenter book (which gets freed on next recreate), pixel size of final image, image index, and image.
  using GraphicsRecreatePresentPerImageFunc = void(Book&, ivec2 const&, uint32_t, GraphicsImage&);

  class GraphicsDevice
  {
  public:
    virtual GraphicsPool& CreatePool(Book& book, uint64_t chunkSize) = 0;
    virtual GraphicsPresenter& CreateWindowPresenter(Book& book, Window& window, std::function<GraphicsRecreatePresentFunc>&& recreatePresent, std::function<GraphicsRecreatePresentPerImageFunc>&& recreatePresentPerImage) = 0;
    virtual GraphicsVertexBuffer& CreateVertexBuffer(GraphicsPool& pool, Buffer const& buffer) = 0;
    virtual GraphicsPass& CreatePass(Book& book, GraphicsPassConfig const& config) = 0;
    virtual GraphicsShader& CreateShader(Book& book, GraphicsShaderRoots const& exprs) = 0;
    virtual GraphicsPipelineLayout& CreatePipelineLayout(Book& book) = 0;
    virtual GraphicsPipeline& CreatePipeline(Book& book, GraphicsPipelineConfig const& config, GraphicsPipelineLayout& graphicsPipelineLayout, GraphicsPass& pass, GraphicsSubPassId subPassId, GraphicsShader& shader) = 0;
    virtual GraphicsFramebuffer& CreateFramebuffer(Book& book, GraphicsPass& pass, std::span<GraphicsImage*> const& pImages, ivec2 const& size) = 0;
  };

  class GraphicsContext
  {
  public:
    virtual void BindVertexBuffer(GraphicsVertexBuffer& vertexBuffer) = 0;
    virtual void BindPipeline(GraphicsPipeline& pipeline) = 0;
    virtual void Draw(uint32_t verticesCount) = 0;
  };
}
