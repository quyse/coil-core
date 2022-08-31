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

  class VulkanPool : public GraphicsPool
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

  class VulkanContext : public GraphicsContext
  {
  public:
    VulkanContext(VulkanDevice& device, VulkanPool& pool, Book& book, VkCommandBuffer commandBuffer);

    void BindVertexBuffer(GraphicsVertexBuffer& vertexBuffer) override;
    void BindUniformBuffer(GraphicsSlotSetId slotSet, GraphicsSlotId slot, Buffer const& buffer) override;
    void BindPipeline(GraphicsPipeline& pipeline) override;
    void Draw(uint32_t verticesCount) override;

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
    using Binding = std::variant<BindingUniformBuffer>;
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

    friend class VulkanFrame;
  };

  class VulkanFrame : public GraphicsFrame
  {
  public:
    VulkanFrame(Book& book, VulkanDevice& device, VulkanPresenter& presenter, VulkanPool& pool, VkCommandBuffer commandBuffer);

    uint32_t GetImageIndex() const override;
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

  class VulkanPresenter : public GraphicsPresenter
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

    void Init();
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

  class VulkanPass : public GraphicsPass
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

  class VulkanVertexBuffer : public GraphicsVertexBuffer
  {
  public:
    VulkanVertexBuffer(VkBuffer buffer);

  private:
    VkBuffer _buffer;

    friend class VulkanContext;
  };

  class VulkanImage : public GraphicsImage
  {
  public:
    VulkanImage(VkImage image, VkImageView imageView);

  private:
    VkImage _image;
    VkImageView _imageView;

    friend class VulkanDevice;
    friend class VulkanFrame;
  };

  class VulkanShader : public GraphicsShader
  {
  public:
    VulkanShader(VkShaderModule shaderModule, VkShaderStageFlags stagesMask, std::vector<SpirvDescriptorSetLayout>&& descriptorSetLayouts);

  private:
    VkShaderModule const _shaderModule;
    VkShaderStageFlags const _stagesMask;
    std::vector<SpirvDescriptorSetLayout> const _descriptorSetLayouts;

    friend class VulkanDevice;
  };

  class VulkanPipelineLayout : public GraphicsPipelineLayout
  {
  public:
    VulkanPipelineLayout(VkPipelineLayout pipelineLayout, std::vector<VkDescriptorSetLayout>&& descriptorSetLayouts);

  private:
    VkPipelineLayout _pipelineLayout;
    std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;

    friend class VulkanDevice;
    friend class VulkanContext;
  };

  class VulkanPipeline : public GraphicsPipeline
  {
  public:
    VulkanPipeline(VkPipeline pipeline, VulkanPipelineLayout& pipelineLayout);

  private:
    VkPipeline _pipeline;
    VulkanPipelineLayout& _pipelineLayout;

    friend class VulkanDevice;
    friend class VulkanContext;
  };

  class VulkanFramebuffer : public GraphicsFramebuffer
  {
  public:
    VulkanFramebuffer(VkFramebuffer framebuffer, ivec2 const& size);

  private:
    VkFramebuffer _framebuffer;
    ivec2 _size;

    friend class VulkanFrame;
  };

  class VulkanDevice : public GraphicsDevice
  {
  public:
    VulkanDevice(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamilyIndex, Book& book);

    VulkanPool& CreatePool(Book& book, size_t chunkSize) override;
    VulkanPresenter& CreateWindowPresenter(Book& book, GraphicsPool& graphicsPool, Window& window, std::function<GraphicsRecreatePresentFunc>&& recreatePresent, std::function<GraphicsRecreatePresentPerImageFunc>&& recreatePresentPerImage) override;
    VulkanVertexBuffer& CreateVertexBuffer(Book& book, GraphicsPool& pool, Buffer const& buffer) override;
    VulkanImage& CreateDepthStencilImage(Book& book, GraphicsPool& pool, ivec2 const& size) override;
    VulkanPass& CreatePass(Book& book, GraphicsPassConfig const& config) override;
    VulkanShader& CreateShader(Book& book, GraphicsShaderRoots const& roots) override;
    VulkanPipelineLayout& CreatePipelineLayout(Book& book, std::span<GraphicsShader*> const& shaders) override;
    VulkanPipeline& CreatePipeline(Book& book, GraphicsPipelineConfig const& config, GraphicsPipelineLayout& graphicsPipelineLayout, GraphicsPass& pass, GraphicsSubPassId subPassId, GraphicsShader& shader) override;
    VulkanFramebuffer& CreateFramebuffer(Book& book, GraphicsPass& pass, std::span<GraphicsImage*> const& pImages, ivec2 const& size) override;

  private:
    std::pair<VkDeviceMemory, VkDeviceSize> AllocateMemory(VulkanPool& pool, VkMemoryRequirements const& memoryRequirements, VkMemoryPropertyFlags requireFlags);
    VkFence CreateFence(Book& book, bool signaled = false);
    VkSemaphore CreateSemaphore(Book& book);

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

  class VulkanSystem : public GraphicsSystem
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
