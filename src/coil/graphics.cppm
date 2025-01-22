module;

#include <map>
#include <optional>
#include <span>
#include <variant>
#include <vector>

export module coil.core.graphics;

import coil.core.base;
import coil.core.graphics.shaders;
import coil.core.image.format;
import coil.core.math;
import coil.core.tasks;

export namespace Coil
{
  using GraphicsSubPassId = uint32_t;
  using GraphicsSlotSetId = uint32_t;
  using GraphicsSlotId = uint32_t;

  using GraphicsOpaquePixelFormat = uint32_t;

  struct GraphicsPassConfig
  {
    using AttachmentId = uint32_t;

    using ColorAttachmentPixelFormat = std::variant<PixelFormat, GraphicsOpaquePixelFormat>;

    struct ColorAttachmentConfig
    {
      ColorAttachmentPixelFormat format;
      vec4 clearColor;
    };
    struct DepthStencilAttachmentConfig
    {
      float clearDepth = 0;
      uint32_t clearStencil = 0;
    };
    using AttachmentConfig = std::variant<ColorAttachmentConfig, DepthStencilAttachmentConfig>;
    struct Attachment
    {
      // keep attachment data from before pass
      // otherwise it will be cleared
      bool keepBefore = false;
      // keep attachment data after the pass
      // otherwise it will be undefined
      bool keepAfter = false;
      // attachment config
      AttachmentConfig config;
    };

    class AttachmentRef
    {
    public:
      AttachmentRef(GraphicsPassConfig& config, AttachmentId id)
      : _config(config), _id(id) {}

      AttachmentId GetId() const
      {
        return _id;
      }

      Attachment& operator*()
      {
        return _config.attachments[_id];
      }
      Attachment const& operator*() const
      {
        return _config.attachments[_id];
      }
      Attachment* operator->()
      {
        return &_config.attachments[_id];
      }
      Attachment const* operator->() const
      {
        return &_config.attachments[_id];
      }

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

      void UseColorAttachment(AttachmentRef attachmentRef, uint32_t slot)
      {
        attachments[attachmentRef.GetId()] = ColorAttachment
        {
          .slot = slot,
        };
      }
      void UseDepthStencilAttachment(AttachmentRef attachmentRef)
      {
        attachments[attachmentRef.GetId()] = DepthStencilAttachment
        {
        };
      }
      void UseInputAttachment(AttachmentRef attachmentRef, uint32_t slot)
      {
        attachments[attachmentRef.GetId()] = InputAttachment
        {
          .slot = slot,
        };
      }
      void UseShaderAttachment(AttachmentRef attachmentRef)
      {
        attachments[attachmentRef.GetId()] = ShaderAttachment
        {
        };
      }

      std::map<AttachmentId, Attachment> attachments;
    };

    class SubPassRef
    {
    public:
      SubPassRef(GraphicsPassConfig& config, GraphicsSubPassId id)
      : _config(config), _id(id) {}

      SubPass& operator*()
      {
        return _config.subPasses[_id];
      }
      SubPass const& operator*() const
      {
        return _config.subPasses[_id];
      }
      SubPass* operator->()
      {
        return &_config.subPasses[_id];
      }
      SubPass const* operator->() const
      {
        return &_config.subPasses[_id];
      }

    private:
      GraphicsPassConfig& _config;
      GraphicsSubPassId const _id;
    };

    AttachmentRef AddAttachment(AttachmentConfig const& config)
    {
      AttachmentId attachmentId = (AttachmentId)attachments.size();
      attachments.push_back(
      {
        .config = config,
      });
      return { *this, attachmentId };
    }

    SubPassRef AddSubPass()
    {
      GraphicsSubPassId subPassId = (GraphicsSubPassId)subPasses.size();
      subPasses.push_back({});
      return { *this, subPassId };
    }

    std::vector<Attachment> attachments;
    std::vector<SubPass> subPasses;
  };

  struct GraphicsCapabilities
  {
    bool render = false;
    bool tessellation = false;
    bool compute = false;
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
    virtual GraphicsContext& GetContext() = 0;
    virtual void Pass(GraphicsPass& pass, GraphicsFramebuffer& framebuffer, std::function<void(GraphicsSubPassId, GraphicsContext&)> const& func) = 0;
    virtual void EndFrame() = 0;
  };

  // Represents memory pool.
  class GraphicsPool
  {
  };

  class GraphicsPresenter
  {
  public:
    virtual void Resize(ivec2 const& size) = 0;
    virtual GraphicsFrame& StartFrame() = 0;
  };

  class GraphicsComputer
  {
  public:
    virtual void Compute(std::function<void(GraphicsContext&)> const& func) = 0;
  };

  class GraphicsPass
  {
  public:
  };

  class GraphicsVertexBuffer
  {
  };

  class GraphicsIndexBuffer
  {
  };

  class GraphicsStorageBuffer
  {
  };

  class GraphicsImage
  {
  };

  struct GraphicsSamplerConfig
  {
    enum class Filter
    {
      Nearest,
      Linear,
    };
    enum class Wrap
    {
      Repeat,
      RepeatMirror,
      Clamp,
      Border,
    };

    Filter magFilter = Filter::Nearest;
    Filter minFilter = Filter::Nearest;
    Filter mipFilter = Filter::Nearest;

    Wrap wrapU = Wrap::Repeat;
    Wrap wrapV = Wrap::Repeat;
    Wrap wrapW = Wrap::Repeat;
  };

  template <> GraphicsSamplerConfig::Filter FromString(std::string_view str)
  {
    static std::unordered_map<std::string_view, GraphicsSamplerConfig::Filter> const values =
    {{
      { "Nearest", GraphicsSamplerConfig::Filter::Nearest },
      { "Linear", GraphicsSamplerConfig::Filter::Linear },
    }};
    auto i = values.find(str);
    if(i == values.end())
      throw Exception() << "invalid sampler filter: " << str;
    return i->second;
  }

  template <> GraphicsSamplerConfig::Wrap FromString(std::string_view str)
  {
    static std::unordered_map<std::string_view, GraphicsSamplerConfig::Wrap> const values =
    {{
      { "Repeat", GraphicsSamplerConfig::Wrap::Repeat },
      { "RepeatMirror", GraphicsSamplerConfig::Wrap::RepeatMirror },
      { "Clamp", GraphicsSamplerConfig::Wrap::Clamp },
      { "Border", GraphicsSamplerConfig::Wrap::Border },
    }};
    auto i = values.find(str);
    if(i == values.end())
      throw Exception() << "invalid sampler wrap: " << str;
    return i->second;
  }

  class GraphicsSampler
  {
  };

  class GraphicsShader
  {
  };

  struct GraphicsShaderRoots
  {
    std::shared_ptr<ShaderStatementNode> vertex;
    std::shared_ptr<ShaderStatementNode> tessellationControl;
    std::shared_ptr<ShaderStatementNode> tessellationEvaluation;
    uint32_t tessellationOutputVertices = 0;
    std::shared_ptr<ShaderStatementNode> fragment;
    std::shared_ptr<ShaderStatementNode> compute;
    ivec3 computeSize = { 1, 1, 1 };
  };

  class GraphicsPipelineLayout
  {
  };

  enum class GraphicsCompareOp
  {
    Never,
    Less,
    LessOrEqual,
    Equal,
    NonEqual,
    GreaterOrEqual,
    Greater,
    Always,
  };

  enum class GraphicsColorBlendFactor
  {
    Zero,
    One,
    Src,
    InvSrc,
    SrcAlpha,
    InvSrcAlpha,
    Dst,
    InvDst,
    DstAlpha,
    InvDstAlpha,
    SecondSrc,
    InvSecondSrc,
    SecondSrcAlpha,
    InvSecondSrcAlpha,
  };

  enum class GraphicsAlphaBlendFactor
  {
    Zero,
    One,
    Src,
    InvSrc,
    Dst,
    InvDst,
    SecondSrc,
    InvSecondSrc,
  };

  enum class GraphicsBlendOp
  {
    Add,
    SubtractAB,
    SubtractBA,
    Min,
    Max,
  };

  struct GraphicsPipelineConfig
  {
    ivec2 viewport;
    bool depthTest = true;
    bool depthWrite = true;
    GraphicsCompareOp depthCompareOp = GraphicsCompareOp::Less;

    GraphicsVertexLayout vertexLayout;

    struct Blending
    {
      // default config for blending with premultiplied alpha
      GraphicsColorBlendFactor srcColorBlendFactor = GraphicsColorBlendFactor::One;
      GraphicsColorBlendFactor dstColorBlendFactor = GraphicsColorBlendFactor::InvSrcAlpha;
      GraphicsBlendOp colorBlendOp = GraphicsBlendOp::Add;
      GraphicsAlphaBlendFactor srcAlphaBlendFactor = GraphicsAlphaBlendFactor::One;
      GraphicsAlphaBlendFactor dstAlphaBlendFactor = GraphicsAlphaBlendFactor::InvSrc;
      GraphicsBlendOp alphaBlendOp = GraphicsBlendOp::Add;
    };
    struct Attachment
    {
      std::optional<Blending> blending;
    };
    std::vector<Attachment> attachments;
  };

  class GraphicsPipeline
  {
  };

  class GraphicsFramebuffer
  {
  };

  struct GraphicsPresentConfig
  {
    // presenter book which gets freed on next presenter resize
    Book& book;
    ivec2 size;
    GraphicsOpaquePixelFormat pixelFormat;
  };

  class GraphicsWindow
  {
  public:
    virtual void SetPresenter(GraphicsPresenter* presenter)
    {
      _presenter = presenter;
    }

  protected:
    GraphicsPresenter* _presenter = nullptr;
  };

  // Function type for recreating resources for present pass.
  // Accepts config, and number of images.
  using GraphicsRecreatePresentFunc = void(GraphicsPresentConfig const&, uint32_t);
  // Function type for recreating resources for present pass per image.
  // Accepts config, image index, and image.
  using GraphicsRecreatePresentPerImageFunc = void(GraphicsPresentConfig const&, uint32_t, GraphicsImage&);

  class GraphicsDevice
  {
  public:
    virtual Book& GetBook() = 0;
    virtual GraphicsPool& CreatePool(Book& book, size_t chunkSize) = 0;
    virtual GraphicsPresenter& CreateWindowPresenter(Book& book, GraphicsPool& pool, GraphicsWindow& window, std::function<GraphicsRecreatePresentFunc>&& recreatePresent, std::function<GraphicsRecreatePresentPerImageFunc>&& recreatePresentPerImage) = 0;
    virtual GraphicsComputer& CreateComputer(Book& book, GraphicsPool& pool) = 0;
    virtual GraphicsVertexBuffer& CreateVertexBuffer(Book& book, GraphicsPool& pool, Buffer const& buffer) = 0;
    virtual GraphicsIndexBuffer& CreateIndexBuffer(Book& book, GraphicsPool& pool, Buffer const& buffer, bool is32Bit) = 0;
    virtual GraphicsStorageBuffer& CreateStorageBuffer(Book& book, GraphicsPool& pool, Buffer const& data) = 0;
    virtual GraphicsImage& CreateRenderImage(Book& book, GraphicsPool& pool, PixelFormat const& pixelFormat, ivec2 const& size, GraphicsSampler* pSampler = nullptr) = 0;
    virtual GraphicsImage& CreateDepthImage(Book& book, GraphicsPool& pool, ivec2 const& size) = 0;
    virtual GraphicsPass& CreatePass(Book& book, GraphicsPassConfig const& config) = 0;
    virtual GraphicsShader& CreateShader(Book& book, GraphicsShaderRoots const& exprs) = 0;
    virtual GraphicsPipelineLayout& CreatePipelineLayout(Book& book, std::span<GraphicsShader*> const& shaders) = 0;
    virtual GraphicsPipeline& CreatePipeline(Book& book, GraphicsPipelineConfig const& config, GraphicsPipelineLayout& graphicsPipelineLayout, GraphicsPass& pass, GraphicsSubPassId subPassId, GraphicsShader& shader) = 0;
    virtual GraphicsPipeline& CreatePipeline(Book& book, GraphicsPipelineLayout& graphicsPipelineLayout, GraphicsShader& shader) = 0;
    virtual GraphicsFramebuffer& CreateFramebuffer(Book& book, GraphicsPass& pass, std::span<GraphicsImage*> const& pImages, ivec2 const& size) = 0;
    virtual GraphicsImage& CreateTexture(Book& book, GraphicsPool& pool, ImageFormat const& format, GraphicsSampler* pSampler = nullptr) = 0;
    virtual GraphicsSampler& CreateSampler(Book& book, GraphicsSamplerConfig const& config) = 0;
    virtual void SetStorageBufferData(GraphicsStorageBuffer& storageBuffer, Buffer const& buffer) = 0;
    virtual void GetStorageBufferData(GraphicsStorageBuffer& storageBuffer, Buffer const& buffer) = 0;
  };

  class GraphicsMesh
  {
  public:
    GraphicsMesh(GraphicsVertexBuffer& vertexBuffer, uint32_t verticesCount)
    : _vertexBuffer(vertexBuffer), _count(verticesCount) {}
    GraphicsMesh(GraphicsVertexBuffer& vertexBuffer, GraphicsIndexBuffer& indexBuffer, uint32_t indicesCount)
    : _vertexBuffer(vertexBuffer), _pIndexBuffer(&indexBuffer), _count(indicesCount) {}

    GraphicsVertexBuffer& GetVertexBuffer() const
    {
      return _vertexBuffer;
    }
    GraphicsIndexBuffer* GetIndexBuffer() const
    {
      return _pIndexBuffer;
    }
    uint32_t GetCount() const
    {
      return _count;
    }

  private:
    GraphicsVertexBuffer& _vertexBuffer;
    GraphicsIndexBuffer* _pIndexBuffer = nullptr;
    uint32_t _count;

    friend class GraphicsContext;
  };

  class GraphicsContext
  {
  public:
    virtual uint32_t GetMaxBufferSize() const = 0;
    virtual void BindVertexBuffer(uint32_t slot, GraphicsVertexBuffer& vertexBuffer) = 0;
    virtual void BindDynamicVertexBuffer(uint32_t slot, Buffer const& buffer) = 0;
    virtual void BindIndexBuffer(GraphicsIndexBuffer* pIndexBuffer) = 0;
    virtual void BindUniformBuffer(GraphicsSlotSetId slotSet, GraphicsSlotId slot, Buffer const& buffer) = 0;
    virtual void BindStorageBuffer(GraphicsSlotSetId slotSet, GraphicsSlotId slot, GraphicsStorageBuffer& storageBuffer) = 0;
    virtual void BindImage(GraphicsSlotSetId slotSet, GraphicsSlotId slot, GraphicsImage& image) = 0;
    virtual void BindPipeline(GraphicsPipeline& pipeline) = 0;
    virtual void Draw(uint32_t indicesCount, uint32_t instancesCount = 1) = 0;
    virtual void Dispatch(ivec3 size) = 0;
    virtual void SetTextureData(GraphicsImage& image, ImageBuffer const& imageBuffer) = 0;

    void BindMesh(GraphicsMesh const& mesh)
    {
      BindVertexBuffer(0, mesh._vertexBuffer);
      BindIndexBuffer(mesh._pIndexBuffer);
    }
  };

  template <>
  struct AssetTraits<GraphicsImage*>
  {
    static constexpr std::string_view assetTypeName = "graphics_image";
  };
  static_assert(IsAsset<GraphicsImage*>);

  template <>
  struct AssetTraits<GraphicsSampler*>
  {
    static constexpr std::string_view assetTypeName = "graphics_sampler";
  };
  static_assert(IsAsset<GraphicsSampler*>);

  // graphics asset manager
  class GraphicsAssetManager
  {
  public:
    GraphicsAssetManager(GraphicsDevice& device, GraphicsPool& pool)
    : _device(device), _pool(pool) {}

    GraphicsDevice& GetDevice() const
    {
      return _device;
    }
    GraphicsPool& GetPool() const
    {
      return _pool;
    }

    void AddContextTask(std::function<void(GraphicsContext&)>&& contextTask)
    {
      _contextTasks.push_back(std::move(contextTask));
    }
    void RunContextTasks(GraphicsContext& context)
    {
      for(size_t i = 0; i < _contextTasks.size(); ++i)
        _contextTasks[i](context);
      _contextTasks.clear();
    }

  private:
    GraphicsDevice& _device;
    GraphicsPool& _pool;
    std::vector<std::function<void(GraphicsContext&)>> _contextTasks;
  };

  class TextureAssetLoader
  {
  public:
    TextureAssetLoader(GraphicsAssetManager& manager)
    : _manager(manager) {}

    template <std::same_as<GraphicsImage*> Asset, typename AssetContext>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      auto image = co_await assetContext.template LoadAssetParam<ImageBuffer>(book, "image");
      auto* pSampler = co_await assetContext.template LoadAssetParam<GraphicsSampler*>(book, "sampler");
      auto* pTexture = &_manager.GetDevice().CreateTexture(book, _manager.GetPool(), image.format, pSampler);
      _manager.AddContextTask([pTexture, image](GraphicsContext& context)
      {
        context.SetTextureData(*pTexture, image);
      });
      co_return pTexture;
    }

    static constexpr std::string_view assetLoaderName = "texture";

  private:
    GraphicsAssetManager& _manager;
  };
  static_assert(IsAssetLoader<TextureAssetLoader>);

  class SamplerAssetLoader
  {
  public:
    SamplerAssetLoader(GraphicsAssetManager& manager)
    : _manager(manager) {}

    template <std::same_as<GraphicsSampler*> Asset, typename AssetContext>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      auto allFilter = assetContext.template GetFromStringParam<GraphicsSamplerConfig::Filter>("filter", GraphicsSamplerConfig::Filter::Nearest);
      auto allWrap = assetContext.template GetFromStringParam<GraphicsSamplerConfig::Wrap>("wrap", GraphicsSamplerConfig::Wrap::Repeat);
      co_return &_manager.GetDevice().CreateSampler(book,
      {
        .magFilter = assetContext.template GetFromStringParam<GraphicsSamplerConfig::Filter>("magFilter", allFilter),
        .minFilter = assetContext.template GetFromStringParam<GraphicsSamplerConfig::Filter>("minFilter", allFilter),
        .mipFilter = assetContext.template GetFromStringParam<GraphicsSamplerConfig::Filter>("mipFilter", allFilter),
        .wrapU = assetContext.template GetFromStringParam<GraphicsSamplerConfig::Wrap>("wrapU", allWrap),
        .wrapV = assetContext.template GetFromStringParam<GraphicsSamplerConfig::Wrap>("wrapV", allWrap),
        .wrapW = assetContext.template GetFromStringParam<GraphicsSamplerConfig::Wrap>("wrapW", allWrap),
      });
    }

    static constexpr std::string_view assetLoaderName = "sampler";

  private:
    GraphicsAssetManager& _manager;
  };
  static_assert(IsAssetLoader<SamplerAssetLoader>);
}
