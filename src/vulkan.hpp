#pragma once

#include "graphics.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <array>

namespace Coil
{
  class VulkanDevice;
  class VulkanPresenter;

  class VulkanPool : public GraphicsPool
  {
  public:
    VulkanPool(VulkanDevice& device, VkDeviceSize chunkSize);

    std::pair<VkDeviceMemory, VkDeviceSize> Allocate(uint32_t memoryTypeIndex, VkDeviceSize size, VkDeviceSize alignment);

  private:
    VulkanDevice& _device;
    VkDeviceSize _chunkSize;

    struct MemoryType
    {
      VkDeviceMemory memory = nullptr;
      VkDeviceSize size = 0;
      VkDeviceSize offset = 0;
    };
    MemoryType _memoryTypes[VK_MAX_MEMORY_TYPES];
  };

  class VulkanFrame : public GraphicsFrame
  {
  public:
    void EndFrame() override;

  private:
    void Init(Book& book, VkDevice device, VkSwapchainKHR swapchain, VkQueue queue, VkCommandBuffer commandBuffer);
    void Begin(std::vector<VkImage> const& images, bool& isSubOptimal);

    VkDevice _device;
    VkSwapchainKHR _swapchain;
    VkQueue _queue;
    VkCommandBuffer _commandBuffer;
    VkFence _fenceFrameFinished;
    VkSemaphore _semaphoreImageAvailable;
    VkSemaphore _semaphoreFrameFinished;

    VkImage _image = nullptr;
    uint32_t _imageIndex = -1;

    friend class VulkanPresenter;
  };

  class VulkanPresenter : public GraphicsPresenter
  {
  public:
    VulkanPresenter(
      Book& book,
      VkPhysicalDevice physicalDevice,
      VkSurfaceKHR surface,
      VkDevice device,
      VkQueue queue,
      VkCommandPool commandPool,
      std::function<GraphicsRecreatePresentPassFunc>&& recreatePresentPass
      );

    void Init();
    void Clear();

    void Resize(ivec2 const& size) override;
    VulkanFrame& StartFrame() override;

  private:
    Book& _book;
    VkPhysicalDevice _physicalDevice;
    VkSurfaceKHR _surface;
    VkDevice _device;
    VkQueue _queue;
    VkCommandPool _commandPool;
    std::function<GraphicsRecreatePresentPassFunc> _recreatePresentPass;
    VkSwapchainKHR _swapchain = nullptr;
    // swapchain images
    std::vector<VkImage> _images;

    // frames
    static size_t constexpr _framesCount = 2;
    std::array<VulkanFrame, _framesCount> _frames;
    // next frame to use
    size_t _nextFrame = 0;

    // whether to recreate swapchain on next frame
    bool _recreateNeeded = false;
  };

  class VulkanVertexBuffer : public GraphicsVertexBuffer
  {
  public:
    VulkanVertexBuffer(VkBuffer buffer);

  private:
    VkBuffer _buffer;

    friend class VulkanDevice;
  };

  class VulkanShader : public GraphicsShader
  {
  public:
    VulkanShader(VkDevice device, VkShaderModule shaderModule, uint8_t stagesMask);
    ~VulkanShader();

    VkDevice const device;
    VkShaderModule const shaderModule;
    uint8_t const stagesMask;
  };

  class VulkanPipeline : public GraphicsPipeline
  {
  public:
    VulkanPipeline(VkDevice device, VkPipeline pipeline);
    ~VulkanPipeline();

  private:
    VkDevice _device;
    VkPipeline _pipeline;
  };

  class VulkanDevice : public GraphicsDevice
  {
  public:
    VulkanDevice(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamilyIndex);
    ~VulkanDevice();

    void Init();

    VulkanPool& CreatePool(Book& book, uint64_t chunkSize) override;
    VulkanPresenter& CreateWindowPresenter(Book& book, Window& window, std::function<GraphicsRecreatePresentPassFunc>&& recreatePresentPass) override;
    VulkanVertexBuffer& CreateVertexBuffer(GraphicsPool& pool, Buffer const& buffer) override;
    VulkanShader& CreateShader(Book& book, GraphicsShaderRoots const& roots) override;

  private:
    std::pair<VkDeviceMemory, VkDeviceSize> AllocateMemory(VulkanPool& pool, VkMemoryRequirements const& memoryRequirements, VkMemoryPropertyFlags requireFlags);

    VkInstance _instance;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    uint32_t _graphicsQueueFamilyIndex;
    VkQueue _graphicsQueue;
    VkCommandPool _commandPool = nullptr;
    VkPhysicalDeviceMemoryProperties _memoryProperties;

    friend class VulkanPool;
  };

  class VulkanSystem : public GraphicsSystem
  {
  public:
    VulkanSystem(VkInstance instance);
    ~VulkanSystem();

    static VulkanSystem& Init(Book& book, Window& window, char const* appName, uint32_t appVersion);

    VulkanDevice& CreateDefaultDevice(Book& book) override;

    using InstanceExtensionsHandler = std::function<void(Window&, std::vector<char const*>&)>;
    using DeviceSurfaceHandler = std::function<VkSurfaceKHR(VkInstance, Window&)>;

    static void RegisterInstanceExtensionsHandler(InstanceExtensionsHandler&& handler);
    static void RegisterDeviceSurfaceHandler(DeviceSurfaceHandler&& handler);

    static VkFormat GetPixelFormat(PixelFormat format);

  private:
    VkInstance _instance;

    static std::vector<InstanceExtensionsHandler> _instanceExtensionsHandlers;
    static std::vector<DeviceSurfaceHandler> _deviceSurfaceHandlers;

    friend class VulkanDevice;
  };

  class VulkanSurface
  {
  public:
    VulkanSurface(VkInstance instance, VkSurfaceKHR surface);
    ~VulkanSurface();

  private:
    VkInstance _instance;
    VkSurfaceKHR _surface;
  };

  class VulkanDeviceIdle
  {
  public:
    VulkanDeviceIdle(VkDevice device);
    ~VulkanDeviceIdle();

  private:
    VkDevice _device;
  };

  class VulkanSwapchain
  {
  public:
    VulkanSwapchain(VkDevice device, VkSwapchainKHR swapchain);
    ~VulkanSwapchain();

  private:
    VkDevice _device;
    VkSwapchainKHR _swapchain;
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

  class VulkanRenderPass
  {
  public:
    VulkanRenderPass(VkDevice device, VkRenderPass renderPass);
    ~VulkanRenderPass();

  private:
    VkDevice _device;
    VkRenderPass _renderPass;
  };

  class VulkanPipelineLayout
  {
  public:
    VulkanPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout);
    ~VulkanPipelineLayout();

  private:
    VkDevice _device;
    VkPipelineLayout _pipelineLayout;
  };

  class VulkanFramebuffer
  {
  public:
    VulkanFramebuffer(VkDevice device, VkFramebuffer framebuffer);
    ~VulkanFramebuffer();

  private:
    VkDevice _device;
    VkFramebuffer _framebuffer;
  };

  class VulkanBuffer
  {
  public:
    VulkanBuffer(VkDevice device, VkBuffer buffer);
    ~VulkanBuffer();

  private:
    VkDevice _device;
    VkBuffer _buffer;
  };

  class VulkanImageView
  {
  public:
    VulkanImageView(VkDevice device, VkImageView imageView);
    ~VulkanImageView();

  private:
    VkDevice _device;
    VkImageView _imageView;
  };

  class VulkanMemory
  {
  public:
    VulkanMemory(VkDevice device, VkDeviceMemory memory);
    ~VulkanMemory();

  private:
    VkDevice _device;
    VkDeviceMemory _memory;
  };

  class VulkanFence
  {
  public:
    VulkanFence(VkDevice device, VkFence fence);
    ~VulkanFence();

    static VkFence Create(Book& book, VkDevice device, bool signaled = false);

  private:
    VkDevice _device;
    VkFence _fence;
  };

  class VulkanSemaphore
  {
  public:
    VulkanSemaphore(VkDevice device, VkSemaphore semaphore);
    ~VulkanSemaphore();

    static VkSemaphore Create(Book& book, VkDevice device);

  private:
    VkDevice _device;
    VkSemaphore _semaphore;
  };
}
