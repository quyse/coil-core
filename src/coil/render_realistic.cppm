/*
Relatively simple realistic renderer.
*/

module;

#include <functional>
#include <numbers>
#include <vector>

export module coil.core.render.realistic;

import coil.core.base;
import coil.core.graphics.shaders;
import coil.core.graphics;
import coil.core.image.format;
import coil.core.math;
import coil.core.render.math;
import coil.core.render.realistic.formats;
import coil.core.render;

export namespace Coil
{
  class RealisticRenderer
  {
  private:
    auto const& GetOpaqueVertexLayout()
    {
      static auto const layout = GetGraphicsVertexStructLayout<VertexPN, VertexRO>();
      return layout;
    }

    auto const& GetScreenVertexLayout()
    {
      static auto const layout = GetGraphicsVertexStructLayout<ScreenVertex>();
      return layout;
    }

    static constexpr PixelFormat hdrColorFormat = PixelFormats::floatRGB32;

  public:
    template <template <typename> typename T>
    struct SceneUniform
    {
      T<mat4x4> viewProj;
      T<vec3> eyePosition;
      T<vec3> lightPosition;
      T<vec3> lightColor;
      T<vec3> ambientLightColor;
    };

    template <template <typename> typename T>
    struct OpaqueMaterialUniform
    {
      T<vec3> color;
      T<float> roughness;
      T<float> metalness;
    };

    using OpaqueTransform = ShaderDataIdentityStruct<VertexRO>;
    using OpaqueMaterial = ShaderDataIdentityStruct<OpaqueMaterialUniform>;

    RealisticRenderer(GraphicsDevice& graphicsDevice)
    : graphicsDevice_(graphicsDevice) {}

    void Init(GraphicsPool& pool, GraphicsWindow& window, std::function<void()>&& onRecreatePresent = {})
    {
      Book& book = graphicsDevice_.GetBook();

      InitMeshes(book, pool);
      InitSamplers(book);
      InitShaders(book);

      onRecreatePresent_ = std::move(onRecreatePresent);

      pGraphicsPresenter_ = &graphicsDevice_.CreateWindowPresenter(book, pool, window,
        [&](GraphicsPresentConfig const& presentConfig, uint32_t imagesCount)
        {
          RecreatePresent(presentConfig, imagesCount);
        },
        [&](GraphicsPresentConfig const& presentConfig, uint32_t imageIndex, GraphicsImage& image)
        {
          RecreatePresentPerImage(presentConfig, imageIndex, image);
        });
    }

    void RecreatePresent(GraphicsPresentConfig const& presentConfig, uint32_t imagesCount)
    {
      Book& presentBook = presentConfig.book;

      // create present pool
      pPresentPool_ = &graphicsDevice_.CreatePool(presentBook, 64 * 1024 * 1024);
      // remember screen size
      screenSize_ = presentConfig.size;

      InitPasses(presentBook, presentConfig.pixelFormat);
      InitPipelines(presentBook);

      imagePipelines_.assign(imagesCount, {});

      if(onRecreatePresent_)
      {
        onRecreatePresent_();
      }
    }

    void RecreatePresentPerImage(GraphicsPresentConfig const& presentConfig, uint32_t imageIndex, GraphicsImage& image)
    {
      Book& presentBook = presentConfig.book;

      auto& imagePipeline = imagePipelines_[imageIndex];

      // create HDR color image
      imagePipeline.pHdrColorImage = &graphicsDevice_.CreateRenderImage(presentBook, *pPresentPool_, hdrColorFormat, screenSize_, pHdrColorSampler_);

      // create depth stencil image
      imagePipeline.pDepthStencilImage = &graphicsDevice_.CreateDepthImage(presentBook, *pPresentPool_, screenSize_);

      GraphicsImage* images[] =
      {
        imagePipeline.pHdrColorImage,
        imagePipeline.pDepthStencilImage,
        &image,
      };

      // create framebuffer
      imagePipeline.pFramebuffer = &graphicsDevice_.CreateFramebuffer(presentBook, *pPass_, images, screenSize_);
    }

    ivec2 GetScreenSize() const
    {
      return screenSize_;
    }

    void Frame(std::function<void()> const& func)
    {
      GraphicsFrame& frame = pGraphicsPresenter_->StartFrame();

      opaqueRenderCache_.Reset();

      {
        GraphicsContext& context = frame.GetContext();
        for(auto& task : contextTasks_)
          task(context);
        contextTasks_.clear();
      }

      func();

      auto& imagePipeline = imagePipelines_[frame.GetImageIndex()];

      frame.Pass(*pPass_, *imagePipeline.pFramebuffer, [&](GraphicsSubPassId subPassId, GraphicsContext& graphicsContext)
      {
        switch(subPassId)
        {
        case 0: // opaque subpass
          opaqueRenderCache_.Flush(graphicsContext);
          break;
        case 1: // tone mapping subpass
          toneMappingRenderContext_.Begin(graphicsContext);
          toneMappingRenderContext_.SetPipeline(*pToneMappingPipeline_);
          toneMappingRenderContext_.SetMesh(*pScreenQuadMesh_);
          toneMappingRenderContext_.SetImage(0, 0, *imagePipeline.pHdrColorImage);
          toneMappingRenderContext_.EndInstance();
          toneMappingRenderContext_.Flush();
          break;
        }
      });

      frame.EndFrame();
    }

    void SetViewProj(vec3 const& eyePosition, mat4x4 const& viewProjTransform)
    {
      sceneUniform_.eyePosition = eyePosition;
      sceneUniform_.viewProj = viewProjTransform;
    }

    void SetLight(vec3 const& lightPosition, vec3 const& lightColor)
    {
      sceneUniform_.lightPosition = lightPosition;
      sceneUniform_.lightColor = lightColor;
    }

    void SetAmbientLight(vec3 const& ambientLightColor)
    {
      sceneUniform_.ambientLightColor = ambientLightColor;
    }

    void RenderOpaque(OpaqueMaterial const& material, GraphicsMesh& mesh, ShaderDataIdentityStruct<VertexRO> const& transform)
    {
      opaqueRenderCache_.Render(sceneUniform_, material, *pOpaquePipeline_, mesh, transform);
    }

    void AddContextTask(std::function<void(GraphicsContext&)>&& task)
    {
      contextTasks_.push_back(std::move(task));
    }

  private:
    void InitSamplers(Book& book)
    {
      pHdrColorSampler_ = &graphicsDevice_.CreateSampler(book, {});
    }

    void InitMeshes(Book& book, GraphicsPool& pool)
    {
      InitScreenQuadMesh(book, pool);
    }
    void InitScreenQuadMesh(Book& book, GraphicsPool& pool);

    void InitShaders(Book& book)
    {
      InitOpaqueShaders(book);
      InitToneMappingShaders(book);
    }
    void InitOpaqueShaders(Book& book);
    void InitToneMappingShaders(Book& book);

    void InitPasses(Book& book, GraphicsOpaquePixelFormat pixelFormat)
    {
      GraphicsPassConfig passConfig;

      // HDR color attachment
      auto hdrColorRef = passConfig.AddAttachment(GraphicsPassConfig::ColorAttachmentConfig
      {
        .format = hdrColorFormat,
        .clearColor = vec4(0, 0, 0, 0),
      });

      // depth attachment
      auto depthRef = passConfig.AddAttachment(GraphicsPassConfig::DepthStencilAttachmentConfig
      {
        .clearDepth = 0.0f,
      });

      // result attachment
      auto resultRef = passConfig.AddAttachment(GraphicsPassConfig::ColorAttachmentConfig
      {
        .format = pixelFormat,
        .clearColor = vec4(0, 0, 0, 0),
      });
      resultRef->keepAfter = true;

      // opaque subpass
      {
        auto subPass = passConfig.AddSubPass();
        subPass->UseColorAttachment(hdrColorRef, 0);
        subPass->UseDepthStencilAttachment(depthRef);
      }

      // tone mapping subpass
      {
        auto subPass = passConfig.AddSubPass();
        subPass->UseShaderAttachment(hdrColorRef);
        subPass->UseColorAttachment(resultRef, 0);
      }

      pPass_ = &graphicsDevice_.CreatePass(book, passConfig);
    }

    void InitPipelines(Book& book)
    {
      // opaque pipeline
      {
        GraphicsPipelineConfig config;
        config.viewport = screenSize_;
        config.depthCompareOp = GraphicsCompareOp::Greater;
        config.vertexLayout = GetOpaqueVertexLayout().layout;
        config.vertexLayout.slots[1].perInstance = true;
        config.attachments.push_back({});

        pOpaquePipeline_ = &graphicsDevice_.CreatePipeline(book, config, *pOpaquePipelineLayout_, *pPass_, 0, *pOpaqueShader_);
      }

      // tone mapping pipeline
      {
        GraphicsPipelineConfig config;
        config.viewport = screenSize_;
        config.vertexLayout = GetScreenVertexLayout().layout;
        config.attachments.push_back({});

        pToneMappingPipeline_ = &graphicsDevice_.CreatePipeline(book, config, *pToneMappingPipelineLayout_, *pPass_, 1, *pToneMappingShader_);
      }
    }

    GraphicsDevice& graphicsDevice_;

    // presenter-independent resources
    GraphicsShader* pOpaqueShader_ = nullptr;
    GraphicsPipelineLayout* pOpaquePipelineLayout_ = nullptr;
    GraphicsPipeline* pOpaquePipeline_ = nullptr;

    GraphicsMesh* pScreenQuadMesh_ = nullptr;
    GraphicsSampler* pHdrColorSampler_ = nullptr;
    GraphicsShader* pToneMappingShader_ = nullptr;
    GraphicsPipelineLayout* pToneMappingPipelineLayout_ = nullptr;
    GraphicsPipeline* pToneMappingPipeline_ = nullptr;

    GraphicsPresenter* pGraphicsPresenter_ = nullptr;

    // presenter-dependent resources
    GraphicsPool* pPresentPool_ = nullptr;
    ivec2 screenSize_;
    GraphicsPass* pPass_ = nullptr;

    // frame-dependent resources
    struct ImagePipeline
    {
      GraphicsImage* pHdrColorImage = nullptr;
      GraphicsImage* pDepthStencilImage = nullptr;
      GraphicsFramebuffer* pFramebuffer = nullptr;
    };
    std::vector<ImagePipeline> imagePipelines_;

    // render state
    ShaderDataIdentityStruct<SceneUniform> sceneUniform_;

    RenderCache<
      RenderUniformBufferKnob<ShaderDataIdentityStruct<SceneUniform>, 0, 0>,
      RenderUniformBufferKnob<ShaderDataIdentityStruct<OpaqueMaterialUniform>, 1, 0>,
      RenderPipelineKnob,
      RenderMeshKnob,
      RenderInstanceDataKnob<ShaderDataIdentityStruct<VertexRO>>
    > opaqueRenderCache_;

    RenderContext toneMappingRenderContext_{RenderContext::RenderType::Instances};

    std::vector<std::function<void(GraphicsContext&)>> contextTasks_;

    std::function<void()> onRecreatePresent_;
  };
}
