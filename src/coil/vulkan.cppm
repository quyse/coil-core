module;

#include <vulkan/vulkan.h>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <unordered_map>
#include <variant>
#include <vector>

export module coil.core.vulkan;

import :utils;
import coil.core.graphics.format;
import coil.core.graphics;
import coil.core.image.format;
import coil.core.math;
import coil.core.spirv;

export namespace Coil
{
  class VulkanDevice;
  class VulkanPresenter;
  class VulkanComputer;
  class VulkanFrame;
  class VulkanPipeline;
  class VulkanImage;
  class VulkanSampler;

  class VulkanPool final : public GraphicsPool
  {
  public:
    VulkanPool(VulkanDevice& device, VkDeviceSize chunkSize)
    : _device(device), _chunkSize(chunkSize) {}

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
    VulkanContext(VulkanDevice& device, VulkanPool& pool, Book& book, VkCommandBuffer commandBuffer)
    : _device(device), _pool(pool), _book(book), _commandBuffer(commandBuffer) {}

    uint32_t GetMaxBufferSize() const override
    {
      return _maxBufferSize;
    }

    void BindVertexBuffer(uint32_t slot, GraphicsVertexBuffer& graphicsVertexBuffer) override;
    void BindDynamicVertexBuffer(uint32_t slot, Buffer const& buffer) override;
    void BindIndexBuffer(GraphicsIndexBuffer* pIndexBuffer) override;
    void BindUniformBuffer(GraphicsSlotSetId slotSet, GraphicsSlotId slot, Buffer const& buffer) override;
    void BindStorageBuffer(GraphicsSlotSetId slotSet, GraphicsSlotId slot, GraphicsStorageBuffer& storageBuffer) override;
    void BindImage(GraphicsSlotSetId slotSet, GraphicsSlotId slot, GraphicsImage& image) override;
    void BindPipeline(GraphicsPipeline& pipeline) override;

    void Draw(uint32_t indicesCount, uint32_t instancesCount) override;
    void Dispatch(ivec3 size) override;

    void SetTextureData(GraphicsImage& image, ImageBuffer const& imageBuffer) override;

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

    void BeginFrame()
    {
      _pBoundPipeline = nullptr;
      _pBoundPipelineLayout = nullptr;
      _pBoundDescriptorSets.clear();
      _hasBoundIndexBuffer = false;
      for(auto& cacheIt : _descriptorSetLayoutCaches)
        cacheIt.second.nextDescriptorSet = 0;
      for(auto& cacheIt : _bufferCaches)
      {
        cacheIt.second.nextBuffer = 0;
        cacheIt.second.nextBufferOffset = 0;
      }
      Reset();
    }

    void Reset()
    {
      _pPipeline = nullptr;
      _descriptorSets.clear();
    }

    VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout);
    AllocatedBuffer AllocateBuffer(VkBufferUsageFlagBits usage, uint32_t size, uint32_t alignment = 1);

    void Prepare();

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
    struct BindingStorageBuffer
    {
      VkDescriptorBufferInfo info;
    };
    struct BindingImage
    {
      VkDescriptorImageInfo info;
    };
    using Binding = std::variant<BindingUniformBuffer, BindingStorageBuffer, BindingImage>;
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
    friend class VulkanComputer;
  };

  class VulkanFrame final : public GraphicsFrame
  {
  public:
    VulkanFrame(Book& book, VulkanDevice& device, VulkanPresenter& presenter, VulkanPool& pool, VkCommandBuffer commandBuffer);

    uint32_t GetImageIndex() const override
    {
      return _imageIndex;
    }

    VulkanContext& GetContext() override
    {
      return _context;
    }

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

    void Init();

    void Clear()
    {
      _sizeDependentBook.Free();
      _swapchain = nullptr;
      _images.clear();
    }

    void Resize(ivec2 const& size) override
    {
      _size = size;
      Clear();
      Init();
    }

    VulkanFrame& StartFrame() override;

  private:
    VulkanDevice& _device;
    ivec2 _size;
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

  class VulkanComputer final : public GraphicsComputer
  {
  public:
    VulkanComputer(Book& book, VulkanDevice& device, VulkanPool& pool, VkCommandBuffer commandBuffer, VkFence fenceComputeFinished)
    : _book(book),
      _device(device),
      _commandBuffer(commandBuffer),
      _context(_device, pool, _book, _commandBuffer),
      _fenceComputeFinished(fenceComputeFinished)
    {
    }

    void Compute(std::function<void(GraphicsContext&)> const& func) override;

  private:
    Book& _book;
    VulkanDevice& _device;
    VkCommandBuffer const _commandBuffer;
    VulkanContext _context;
    VkFence const _fenceComputeFinished;
  };

  class VulkanPass final : public GraphicsPass
  {
  public:
    VulkanPass(VkRenderPass renderPass, std::vector<VkClearValue>&& clearValues, uint32_t subPassesCount)
    : _renderPass(renderPass), _clearValues(std::move(clearValues)), _subPassesCount(subPassesCount) {}

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
    VulkanVertexBuffer(VkBuffer buffer)
    : _buffer(buffer) {}

  private:
    VkBuffer const _buffer;

    friend class VulkanContext;
  };

  class VulkanIndexBuffer final : public GraphicsIndexBuffer
  {
  public:
    VulkanIndexBuffer(VkBuffer buffer, bool is32Bit)
    : _buffer(buffer), _is32Bit(is32Bit) {}

  private:
    VkBuffer const _buffer;
    bool _is32Bit;

    friend class VulkanContext;
  };

  class VulkanStorageBuffer final : public GraphicsStorageBuffer
  {
  public:
    VulkanStorageBuffer(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size)
    : _buffer(buffer), _memory(memory), _offset(offset), _size(size) {}

  private:
    VkBuffer const _buffer;
    VkDeviceMemory _memory;
    VkDeviceSize _offset;
    VkDeviceSize _size;

    friend class VulkanDevice;
    friend class VulkanContext;
  };

  class VulkanImage final : public GraphicsImage
  {
  public:
    VulkanImage(VkImage image, VkImageView imageView, VkSampler sampler = nullptr)
    : _image(image), _imageView(imageView), _sampler(sampler) {}

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
    VulkanSampler(VkSampler sampler)
    : _sampler(sampler) {}

  private:
    VkSampler _sampler;

    friend class VulkanDevice;
  };

  class VulkanShader final : public GraphicsShader
  {
  public:
    VulkanShader(VkShaderModule shaderModule, VkShaderStageFlags stagesMask, std::vector<SpirvDescriptorSetLayout>&& descriptorSetLayouts)
    : _shaderModule(shaderModule), _stagesMask(stagesMask), _descriptorSetLayouts(std::move(descriptorSetLayouts)) {}

  private:
    VkShaderModule const _shaderModule;
    VkShaderStageFlags const _stagesMask;
    std::vector<SpirvDescriptorSetLayout> const _descriptorSetLayouts;

    friend class VulkanDevice;
  };

  class VulkanPipelineLayout final : public GraphicsPipelineLayout
  {
  public:
    VulkanPipelineLayout(VkPipelineLayout pipelineLayout, std::vector<VkDescriptorSetLayout>&& descriptorSetLayouts)
    : _pipelineLayout(pipelineLayout), _descriptorSetLayouts(std::move(descriptorSetLayouts)) {}

  private:
    VkPipelineLayout _pipelineLayout;
    std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;

    friend class VulkanDevice;
    friend class VulkanContext;
  };

  class VulkanPipeline final : public GraphicsPipeline
  {
  public:
    VulkanPipeline(VkPipeline pipeline, VulkanPipelineLayout& pipelineLayout, VkPipelineBindPoint bindPoint)
    : _pipeline(pipeline), _pipelineLayout(pipelineLayout), _bindPoint(bindPoint) {}

  private:
    VkPipeline _pipeline;
    VulkanPipelineLayout& _pipelineLayout;
    VkPipelineBindPoint _bindPoint;

    friend class VulkanDevice;
    friend class VulkanContext;
  };

  class VulkanFramebuffer final : public GraphicsFramebuffer
  {
  public:
    VulkanFramebuffer(VkFramebuffer framebuffer, ivec2 const& size)
    : _framebuffer(framebuffer), _size(size) {}

  private:
    VkFramebuffer _framebuffer;
    ivec2 _size;

    friend class VulkanFrame;
  };

  class VulkanDevice final : public GraphicsDevice
  {
  public:
    VulkanDevice(Book& book, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamilyIndex);

    Book& GetBook() override
    {
      return _book;
    }

    VulkanPool& CreatePool(Book& book, size_t chunkSize) override
    {
      return book.Allocate<VulkanPool>(*this, chunkSize);
    }

    VulkanPresenter& CreateWindowPresenter(Book& book, GraphicsPool& pool, GraphicsWindow& window, std::function<GraphicsRecreatePresentFunc>&& recreatePresent, std::function<GraphicsRecreatePresentPerImageFunc>&& recreatePresentPerImage) override;
    VulkanComputer& CreateComputer(Book& book, GraphicsPool& pool) override;
    VulkanVertexBuffer& CreateVertexBuffer(Book& book, GraphicsPool& pool, Buffer const& buffer) override;
    VulkanIndexBuffer& CreateIndexBuffer(Book& book, GraphicsPool& pool, Buffer const& buffer, bool is32Bit) override;
    VulkanStorageBuffer& CreateStorageBuffer(Book& book, GraphicsPool& pool, Buffer const& buffer) override;
    VulkanImage& CreateRenderImage(Book& book, GraphicsPool& pool, PixelFormat const& pixelFormat, ivec2 const& size, GraphicsSampler* pSampler = nullptr) override;
    VulkanImage& CreateDepthImage(Book& book, GraphicsPool& pool, ivec2 const& size) override;
    VulkanPass& CreatePass(Book& book, GraphicsPassConfig const& config) override;
    VulkanShader& CreateShader(Book& book, GraphicsShaderRoots const& roots) override;
    VulkanPipelineLayout& CreatePipelineLayout(Book& book, std::span<GraphicsShader*> const& shaders) override;
    VulkanPipeline& CreatePipeline(Book& book, GraphicsPipelineConfig const& config, GraphicsPipelineLayout& pipelineLayout, GraphicsPass& pass, GraphicsSubPassId subPassId, GraphicsShader& shader) override;
    VulkanPipeline& CreatePipeline(Book& book, GraphicsPipelineLayout& pipelineLayout, GraphicsShader& shader) override;
    VulkanFramebuffer& CreateFramebuffer(Book& book, GraphicsPass& pass, std::span<GraphicsImage*> const& pImages, ivec2 const& size) override;
    VulkanImage& CreateTexture(Book& book, GraphicsPool& pool, ImageFormat const& format, GraphicsSampler* pSampler = nullptr) override;
    VulkanSampler& CreateSampler(Book& book, GraphicsSamplerConfig const& config) override;
    void SetStorageBufferData(GraphicsStorageBuffer& storageBuffer, Buffer const& buffer) override;
    void GetStorageBufferData(GraphicsStorageBuffer& storageBuffer, Buffer const& buffer) override;

  private:
    std::pair<VkDeviceMemory, VkDeviceSize> AllocateMemory(VulkanPool& pool, VkMemoryRequirements const& memoryRequirements, VkMemoryPropertyFlags requireFlags);
    std::tuple<VkBuffer, VkDeviceMemory, VkDeviceSize, VkDeviceSize> CreateBuffer(Book& book, VulkanPool& pool, VkBufferUsageFlags usage, Buffer const& initialData);
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
    VkFormat _depthFormat = VK_FORMAT_UNDEFINED;

    friend class VulkanContext;
    friend class VulkanPresenter;
    friend class VulkanComputer;
    friend class VulkanFrame;
    friend class VulkanPool;
  };

  class VulkanSystem final : public GraphicsSystem
  {
  public:
    VulkanSystem(VkInstance instance, GraphicsCapabilities const& capabilities)
    : _instance(instance), _capabilities(capabilities) {}

    static VulkanSystem& Create(Book& book, GraphicsCapabilities const& capabilities)
    {
      return Create(book, nullptr, capabilities);
    }
    static VulkanSystem& Create(Book& book, GraphicsWindow& window, GraphicsCapabilities const& capabilities)
    {
      return Create(book, &window, capabilities);
    }

    VulkanDevice& CreateDefaultDevice(Book& book) override;

    using InstanceExtensionsHandler = std::function<void(GraphicsWindow&, std::vector<char const*>&)>;
    using DeviceSurfaceHandler = std::function<VkSurfaceKHR(VkInstance, GraphicsWindow&)>;

    static void RegisterInstanceExtensionsHandler(InstanceExtensionsHandler&& handler)
    {
      _instanceExtensionsHandlers.push_back(std::move(handler));
    }
    static void RegisterDeviceSurfaceHandler(DeviceSurfaceHandler&& handler)
    {
      _deviceSurfaceHandlers.push_back(std::move(handler));
    }

    static VkFormat GetVertexFormat(VertexFormat format);
    static VkFormat GetPixelFormat(PixelFormat format);
    static VkCompareOp GetCompareOp(GraphicsCompareOp op);
    static VkBlendFactor GetColorBlendFactor(GraphicsColorBlendFactor factor);
    static VkBlendFactor GetAlphaBlendFactor(GraphicsAlphaBlendFactor factor);
    static VkBlendOp GetBlendOp(GraphicsBlendOp op);

  private:
    static VulkanSystem& Create(Book& book, GraphicsWindow* window, GraphicsCapabilities const& capabilities);

    VkInstance _instance;

    GraphicsCapabilities const _capabilities;

    static std::vector<InstanceExtensionsHandler> _instanceExtensionsHandlers;
    static std::vector<DeviceSurfaceHandler> _deviceSurfaceHandlers;

    friend class VulkanDevice;
  };

  class VulkanDeviceIdle
  {
  public:
    VulkanDeviceIdle(VkDevice device)
    : _device(device) {}
    ~VulkanDeviceIdle()
    {
      CheckSuccess(vkDeviceWaitIdle(_device), "waiting for Vulkan device idle failed\n");
    }

  private:
    VkDevice _device;
  };

  class VulkanWaitForFence
  {
  public:
    VulkanWaitForFence(VkDevice device, VkFence fence)
    : _device(device), _fence(fence) {}
    ~VulkanWaitForFence()
    {
      CheckSuccess(vkWaitForFences(_device, 1, &_fence, VK_TRUE, std::numeric_limits<uint64_t>::max()), "waiting for Vulkan fence failed");
    }

  private:
    VkDevice _device;
    VkFence _fence;
  };

  class VulkanCommandBuffers
  {
  public:
    VulkanCommandBuffers(VkDevice device, VkCommandPool commandPool, std::vector<VkCommandBuffer>&& commandBuffers)
    : _device(device), _commandPool(commandPool), _commandBuffers(std::move(commandBuffers)) {}
    ~VulkanCommandBuffers()
    {
      vkFreeCommandBuffers(_device, _commandPool, _commandBuffers.size(), _commandBuffers.data());
    }

    static std::vector<VkCommandBuffer> Create(Book& book, VkDevice device, VkCommandPool commandPool, uint32_t count, bool primary = true)
    {
      if(!count) return {};

      VkCommandBufferAllocateInfo const info =
      {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = commandPool,
        .level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
        .commandBufferCount = count,
      };

      std::vector<VkCommandBuffer> commandBuffers(count, nullptr);

      CheckSuccess(vkAllocateCommandBuffers(device, &info, commandBuffers.data()), "allocating Vulkan command buffers failed");

      return std::move(commandBuffers);
    }

  private:
    VkDevice _device;
    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;
  };

  std::vector<VulkanSystem::InstanceExtensionsHandler> VulkanSystem::_instanceExtensionsHandlers;
  std::vector<VulkanSystem::DeviceSurfaceHandler> VulkanSystem::_deviceSurfaceHandlers;
}
