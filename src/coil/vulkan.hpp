#pragma once

#include "graphics.hpp"
#include "spirv.hpp"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>

namespace Coil
{
  class VulkanDevice;
  class VulkanPresenter;
  class VulkanFrame;
  class VulkanPipeline;
  class VulkanImage;
  class VulkanSampler;

  class VulkanPool final : public GraphicsPool
  {
  public:
    VulkanPool(VulkanDevice& device, VkDeviceSize chunkSize);

  private:
    std::pair<VkDeviceMemory, VkDeviceSize> AllocateMemory(uint32_t memoryTypeIndex, VkDeviceSize size, VkDeviceSize alignment);
    VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout);

    VulkanDevice& _device;
    VkDeviceSize _chunkSize;
    Book _book;

    struct MemoryType
    {
      VkDeviceMemory memory = nullptr;
      VkDeviceSize size = 0;
      VkDeviceSize offset = 0;
    };
    MemoryType _memoryTypes[VK_MAX_MEMORY_TYPES];

    VkDescriptorPool _descriptorPool = nullptr;

    friend class VulkanDevice;
    friend class VulkanContext;
  };

  class VulkanContext final : public GraphicsContext
  {
  public:
    VulkanContext(VulkanDevice& device, VulkanPool& pool, Book& book, VkCommandBuffer commandBuffer);

    uint32_t GetMaxBufferSize() const override;
    void BindVertexBuffer(uint32_t slot, GraphicsVertexBuffer& vertexBuffer) override;
    void BindDynamicVertexBuffer(uint32_t slot, Buffer const& buffer) override;
    void BindIndexBuffer(GraphicsIndexBuffer* pIndexBuffer) override;
    void BindUniformBuffer(GraphicsSlotSetId slotSet, GraphicsSlotId slot, Buffer const& buffer) override;
    void BindImage(GraphicsSlotSetId slotSet, GraphicsSlotId slot, GraphicsImage& image) override;
    void BindPipeline(GraphicsPipeline& pipeline) override;
    void Draw(uint32_t indicesCount, uint32_t instancesCount) override;
    void SetTextureData(GraphicsImage& image, GraphicsRawImage const& rawImage) override;

  private:
    struct CachedBuffer
    {
      VkBuffer buffer;
      VkDeviceMemory memory;
      VkDeviceSize memoryOffset;
    };
    struct AllocatedBuffer
    {
      CachedBuffer buffer;
      VkDeviceSize bufferOffset;
    };

    void BeginFrame();
    void Reset();
    VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout);
    AllocatedBuffer AllocateBuffer(VkBufferUsageFlagBits usage, uint32_t size);
    void PrepareDraw();

    VulkanDevice& _device;
    VulkanPool& _pool;
    Book& _book;
    VkCommandBuffer _commandBuffer;

    // resources to bind
    VulkanPipeline* _pPipeline = nullptr;
    struct BindingUniformBuffer
    {
      VkDescriptorBufferInfo info;
    };
    struct BindingImage
    {
      VkDescriptorImageInfo info;
    };
    using Binding = std::variant<BindingUniformBuffer, BindingImage>;
    struct DescriptorSet
    {
      std::map<GraphicsSlotId, Binding> bindings;
      VkDescriptorSet descriptorSet = nullptr;
    };
    std::vector<DescriptorSet> _descriptorSets;

    // actually bound resources
    VkPipeline _pBoundPipeline = nullptr;
    VkPipelineLayout _pBoundPipelineLayout = nullptr;
    std::vector<VkDescriptorSet> _pBoundDescriptorSets;
    bool _hasBoundIndexBuffer = false;

    // temporary buffers
    std::vector<VkDescriptorSet> _bufDescriptorSets;
    std::vector<VkWriteDescriptorSet> _bufWriteDescriptorSets;

    struct DescriptorSetLayoutCache
    {
      std::vector<VkDescriptorSet> descriptorSets;
      size_t nextDescriptorSet = 0;
    };
    std::unordered_map<VkDescriptorSetLayout, DescriptorSetLayoutCache> _descriptorSetLayoutCaches;

    struct BufferCache
    {
      std::vector<CachedBuffer> buffers;
      size_t nextBuffer = 0;
      VkDeviceSize nextBufferOffset = 0;
    };
    std::unordered_map<VkBufferUsageFlagBits, BufferCache> _bufferCaches;

    // max buffer size is fixed for now
    static constexpr uint32_t _maxBufferSize = 0x100000;

    friend class VulkanFrame;
  };

  class VulkanFrame final : public GraphicsFrame
  {
  public:
    VulkanFrame(Book& book, VulkanDevice& device, VulkanPresenter& presenter, VulkanPool& pool, VkCommandBuffer commandBuffer);

    uint32_t GetImageIndex() const override;
    VulkanContext& GetContext() override;
    void Pass(GraphicsPass& pass, GraphicsFramebuffer& framebuffer, std::function<void(GraphicsSubPassId, GraphicsContext&)> const& func) override;
    void EndFrame() override;

  private:
    void Begin(std::vector<VulkanImage*> const& pImages, bool& isSubOptimal);

    Book& _book;
    VulkanDevice& _device;
    VulkanPresenter& _presenter;
    VkCommandBuffer const _commandBuffer;
    VkFence const _fenceFrameFinished;
    VkSemaphore const _semaphoreImageAvailable;
    VkSemaphore const _semaphoreFrameFinished;

    VulkanContext _context;

    VulkanImage* _pImage = nullptr;
    uint32_t _imageIndex = -1;

    friend class VulkanPresenter;
  };

  class VulkanPresenter final : public GraphicsPresenter
  {
  public:
    VulkanPresenter(
      VulkanDevice& device,
      Book& book,
      VkSurfaceKHR surface,
      VulkanPool& persistentPool,
      std::function<GraphicsRecreatePresentFunc>&& recreatePresent,
      std::function<GraphicsRecreatePresentPerImageFunc>&& recreatePresentPerImage
      );

    void Init(std::optional<ivec2> const& size);
    void Clear();

    void Resize(ivec2 const& size) override;
    VulkanFrame& StartFrame() override;

  private:
    VulkanDevice& _device;
    // Book for resources recreated on resize.
    Book _sizeDependentBook;
    VkSurfaceKHR _surface;
    std::function<GraphicsRecreatePresentFunc> _recreatePresent;
    std::function<GraphicsRecreatePresentPerImageFunc> _recreatePresentPerImage;
    VkSwapchainKHR _swapchain = nullptr;
    VkFormat _surfaceFormat = VK_FORMAT_UNDEFINED;
    // swapchain images
    std::vector<VulkanImage*> _images;

    // frames
    static size_t constexpr _framesCount = 2;
    std::vector<VulkanFrame> _frames;
    // next frame to use
    size_t _nextFrame = 0;

    // whether to recreate swapchain on next frame
    bool _recreateNeeded = false;

    friend class VulkanFrame;
  };

  class VulkanPass final : public GraphicsPass
  {
  public:
    VulkanPass(VkRenderPass renderPass, std::vector<VkClearValue>&& clearValues, uint32_t subPassesCount);

  private:
    VkRenderPass const _renderPass;
    std::vector<VkClearValue> const _clearValues;
    uint32_t const _subPassesCount;

    friend class VulkanDevice;
    friend class VulkanContext;
    friend class VulkanFrame;
  };

  class VulkanVertexBuffer final : public GraphicsVertexBuffer
  {
  public:
    VulkanVertexBuffer(VkBuffer buffer);

  private:
    VkBuffer _buffer;

    friend class VulkanContext;
  };

  class VulkanIndexBuffer final : public GraphicsIndexBuffer
  {
  public:
    VulkanIndexBuffer(VkBuffer buffer, bool is32Bit);

  private:
    VkBuffer _buffer;
    bool _is32Bit;

    friend class VulkanContext;
  };

  class VulkanImage final : public GraphicsImage
  {
  public:
    VulkanImage(VkImage image, VkImageView imageView, VkSampler _sampler = nullptr);

  private:
    VkImage _image;
    VkImageView _imageView;
    VkSampler _sampler;

    friend class VulkanDevice;
    friend class VulkanContext;
    friend class VulkanFrame;
  };

  class VulkanSampler final : public GraphicsSampler
  {
  public:
    VulkanSampler(VkSampler sampler);

  private:
    VkSampler _sampler;

    friend class VulkanDevice;
  };

  class VulkanShader final : public GraphicsShader
  {
  public:
    VulkanShader(VkShaderModule shaderModule, VkShaderStageFlags stagesMask, std::vector<SpirvDescriptorSetLayout>&& descriptorSetLayouts);

  private:
    VkShaderModule const _shaderModule;
    VkShaderStageFlags const _stagesMask;
    std::vector<SpirvDescriptorSetLayout> const _descriptorSetLayouts;

    friend class VulkanDevice;
  };

  class VulkanPipelineLayout final : public GraphicsPipelineLayout
  {
  public:
    VulkanPipelineLayout(VkPipelineLayout pipelineLayout, std::vector<VkDescriptorSetLayout>&& descriptorSetLayouts);

  private:
    VkPipelineLayout _pipelineLayout;
    std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;

    friend class VulkanDevice;
    friend class VulkanContext;
  };

  class VulkanPipeline final : public GraphicsPipeline
  {
  public:
    VulkanPipeline(VkPipeline pipeline, VulkanPipelineLayout& pipelineLayout);

  private:
    VkPipeline _pipeline;
    VulkanPipelineLayout& _pipelineLayout;

    friend class VulkanDevice;
    friend class VulkanContext;
  };

  class VulkanFramebuffer final : public GraphicsFramebuffer
  {
  public:
    VulkanFramebuffer(VkFramebuffer framebuffer, ivec2 const& size);

  private:
    VkFramebuffer _framebuffer;
    ivec2 _size;

    friend class VulkanFrame;
  };

  class VulkanDevice final : public GraphicsDevice
  {
  public:
    VulkanDevice(Book& book, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamilyIndex);

    Book& GetBook() override;
    VulkanPool& CreatePool(Book& book, size_t chunkSize) override;
    VulkanPresenter& CreateWindowPresenter(Book& book, GraphicsPool& pool, Window& window, std::function<GraphicsRecreatePresentFunc>&& recreatePresent, std::function<GraphicsRecreatePresentPerImageFunc>&& recreatePresentPerImage) override;
    VulkanVertexBuffer& CreateVertexBuffer(Book& book, GraphicsPool& pool, Buffer const& buffer) override;
    VulkanIndexBuffer& CreateIndexBuffer(Book& book, GraphicsPool& pool, Buffer const& buffer, bool is32Bit) override;
    VulkanImage& CreateDepthStencilImage(Book& book, GraphicsPool& pool, ivec2 const& size) override;
    VulkanPass& CreatePass(Book& book, GraphicsPassConfig const& config) override;
    VulkanShader& CreateShader(Book& book, GraphicsShaderRoots const& roots) override;
    VulkanPipelineLayout& CreatePipelineLayout(Book& book, std::span<GraphicsShader*> const& shaders) override;
    VulkanPipeline& CreatePipeline(Book& book, GraphicsPipelineConfig const& config, GraphicsPipelineLayout& pipelineLayout, GraphicsPass& pass, GraphicsSubPassId subPassId, GraphicsShader& shader) override;
    VulkanFramebuffer& CreateFramebuffer(Book& book, GraphicsPass& pass, std::span<GraphicsImage*> const& pImages, ivec2 const& size) override;
    VulkanImage& CreateTexture(Book& book, GraphicsPool& pool, GraphicsImageFormat const& format, GraphicsSampler* pSampler = nullptr) override;
    VulkanSampler& CreateSampler(Book& book, GraphicsSamplerConfig const& config) override;

  private:
    std::pair<VkDeviceMemory, VkDeviceSize> AllocateMemory(VulkanPool& pool, VkMemoryRequirements const& memoryRequirements, VkMemoryPropertyFlags requireFlags);
    VkFence CreateFence(Book& book, bool signaled = false);
    VkSemaphore CreateSemaphore(Book& book);

    Book& _book;
    VkInstance _instance;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    uint32_t _graphicsQueueFamilyIndex;
    VkQueue _graphicsQueue;
    VkCommandPool _commandPool = nullptr;
    VkPhysicalDeviceProperties _properties;
    VkPhysicalDeviceMemoryProperties _memoryProperties;

    friend class VulkanContext;
    friend class VulkanPresenter;
    friend class VulkanFrame;
    friend class VulkanPool;
  };

  class VulkanSystem final : public GraphicsSystem
  {
  public:
    VulkanSystem(VkInstance instance);

    static VulkanSystem& Create(Book& book, Window& window, char const* appName, uint32_t appVersion);

    VulkanDevice& CreateDefaultDevice(Book& book) override;

    using InstanceExtensionsHandler = std::function<void(Window&, std::vector<char const*>&)>;
    using DeviceSurfaceHandler = std::function<VkSurfaceKHR(VkInstance, Window&)>;

    static void RegisterInstanceExtensionsHandler(InstanceExtensionsHandler&& handler);
    static void RegisterDeviceSurfaceHandler(DeviceSurfaceHandler&& handler);

    static VkFormat GetVertexFormat(VertexFormat format);
    static VkFormat GetPixelFormat(PixelFormat format);
    static VkCompareOp GetCompareOp(GraphicsCompareOp op);
    static VkBlendFactor GetColorBlendFactor(GraphicsColorBlendFactor factor);
    static VkBlendFactor GetAlphaBlendFactor(GraphicsAlphaBlendFactor factor);
    static VkBlendOp GetBlendOp(GraphicsBlendOp op);

  private:
    VkInstance _instance;

    static std::vector<InstanceExtensionsHandler> _instanceExtensionsHandlers;
    static std::vector<DeviceSurfaceHandler> _deviceSurfaceHandlers;

    friend class VulkanDevice;
  };

  class VulkanDeviceIdle
  {
  public:
    VulkanDeviceIdle(VkDevice device);
    ~VulkanDeviceIdle();

  private:
    VkDevice _device;
  };

  class VulkanWaitForFence
  {
  public:
    VulkanWaitForFence(VkDevice device, VkFence fence);
    ~VulkanWaitForFence();

  private:
    VkDevice _device;
    VkFence _fence;
  };

  class VulkanCommandBuffers
  {
  public:
    VulkanCommandBuffers(VkDevice device, VkCommandPool commandPool, std::vector<VkCommandBuffer>&& commandBuffers);
    ~VulkanCommandBuffers();

    static std::vector<VkCommandBuffer> Create(Book& book, VkDevice device, VkCommandPool commandPool, uint32_t count, bool primary = true);

  private:
    VkDevice _device;
    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;
  };
}
