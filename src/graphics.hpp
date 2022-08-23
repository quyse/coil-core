#pragma once

#include "platform.hpp"
#include "graphics_format.hpp"
#include "graphics_shaders.hpp"
#include <vector>
#include <map>
#include <optional>
#include <variant>

namespace Coil
{
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

    using SubPassId = uint32_t;

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
      SubPassRef(GraphicsPassConfig& config, SubPassId id);

      SubPass& operator*();
      SubPass const& operator*() const;
      SubPass* operator->();
      SubPass const* operator->() const;

    private:
      GraphicsPassConfig& _config;
      SubPassId const _id;
    };

    AttachmentRef AddAttachment(PixelFormat format);

    SubPassRef AddSubPass();

    std::vector<Attachment> attachments;
    std::vector<SubPass> subPasses;
  };

  class GraphicsPass
  {
  public:
  };

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
    virtual GraphicsPass& CreatePass(Book& book, GraphicsPassConfig const& config) = 0;
    virtual GraphicsShader& CreateShader(Book& book, GraphicsShaderRoots const& exprs) = 0;
  };
}
