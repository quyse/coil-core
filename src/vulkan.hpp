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
    VulkanFrame(VulkanDevice& device, VulkanPresenter& presenter);

    void EndFrame() override;

  private:
    void Init(Book& book, VkCommandBuffer commandBuffer);
    void Begin(std::vector<VkImage> const& images, bool& isSubOptimal);

    VulkanDevice& _device;
    VulkanPresenter& _presenter;
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
      VulkanDevice& device,
      Book& book,
      VkSurfaceKHR surface,
      std::function<GraphicsRecreatePresentPassFunc>&& recreatePresentPass
      );

    void Init();
    void Clear();

    void Resize(ivec2 const& size) override;
    VulkanFrame& StartFrame() override;

  private:
    VulkanDevice& _device;
    Book& _book;
    VkSurfaceKHR _surface;
    std::function<GraphicsRecreatePresentPassFunc> _recreatePresentPass;
    VkSwapchainKHR _swapchain = nullptr;
    // swapchain images
    std::vector<VkImage> _images;

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
    VulkanPass(VkRenderPass renderPass);

  private:
    VkRenderPass _renderPass;
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
    VulkanShader(VkShaderModule shaderModule, uint8_t stagesMask);

    VkShaderModule const shaderModule;
    uint8_t const stagesMask;
  };

  class VulkanDevice : public GraphicsDevice
  {
  public:
    VulkanDevice(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamilyIndex);

    void Init(Book& book);

    VulkanPool& CreatePool(Book& book, uint64_t chunkSize) override;
    VulkanPresenter& CreateWindowPresenter(Book& book, Window& window, std::function<GraphicsRecreatePresentPassFunc>&& recreatePresentPass) override;
    VulkanVertexBuffer& CreateVertexBuffer(GraphicsPool& pool, Buffer const& buffer) override;
    VulkanPass& CreatePass(Book& book, GraphicsPassConfig const& config) override;
    VulkanShader& CreateShader(Book& book, GraphicsShaderRoots const& roots) override;

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
    VkPhysicalDeviceMemoryProperties _memoryProperties;

    friend class VulkanPresenter;
    friend class VulkanFrame;
    friend class VulkanPool;
  };

  class VulkanSystem : public GraphicsSystem
  {
  public:
    VulkanSystem(VkInstance instance);

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

  class VulkanDeviceIdle
  {
  public:
    VulkanDeviceIdle(VkDevice device);
    ~VulkanDeviceIdle();

  private:
    VkDevice _device;
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
