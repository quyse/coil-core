#include "vulkan.hpp"
#include "spirv.hpp"
#include <vector>
#include <limits>
#include <cstring>

namespace
{
  template <VkResult... additionalSuccessResults>
  VkResult CheckSuccess(VkResult result, char const* message)
  {
    if(((result != VK_SUCCESS) && ... && (result != additionalSuccessResults)))
    {
      char const* str;
      switch(result)
      {
#define R(r) case r: str = #r; break
      R(VK_SUCCESS);
      R(VK_NOT_READY);
      R(VK_TIMEOUT);
      R(VK_EVENT_SET);
      R(VK_EVENT_RESET);
      R(VK_INCOMPLETE);
      R(VK_ERROR_OUT_OF_HOST_MEMORY);
      R(VK_ERROR_OUT_OF_DEVICE_MEMORY);
      R(VK_ERROR_INITIALIZATION_FAILED);
      R(VK_ERROR_DEVICE_LOST);
      R(VK_ERROR_MEMORY_MAP_FAILED);
      R(VK_ERROR_LAYER_NOT_PRESENT);
      R(VK_ERROR_EXTENSION_NOT_PRESENT);
      R(VK_ERROR_FEATURE_NOT_PRESENT);
      R(VK_ERROR_INCOMPATIBLE_DRIVER);
      R(VK_ERROR_TOO_MANY_OBJECTS);
      R(VK_ERROR_FORMAT_NOT_SUPPORTED);
      R(VK_ERROR_FRAGMENTED_POOL);
      R(VK_ERROR_UNKNOWN);
      R(VK_ERROR_OUT_OF_POOL_MEMORY);
      R(VK_ERROR_INVALID_EXTERNAL_HANDLE);
      R(VK_ERROR_FRAGMENTATION);
      R(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
      R(VK_PIPELINE_COMPILE_REQUIRED);
      R(VK_ERROR_SURFACE_LOST_KHR);
      R(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
      R(VK_SUBOPTIMAL_KHR);
      R(VK_ERROR_OUT_OF_DATE_KHR);
      R(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
      R(VK_ERROR_VALIDATION_FAILED_EXT);
      R(VK_ERROR_INVALID_SHADER_NV);
      R(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
      R(VK_ERROR_NOT_PERMITTED_KHR);
      R(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
      R(VK_THREAD_IDLE_KHR);
      R(VK_THREAD_DONE_KHR);
      R(VK_OPERATION_DEFERRED_KHR);
      R(VK_OPERATION_NOT_DEFERRED_KHR);
#undef R
      default:
        str = "unknown";
        break;
      }
      throw Coil::Exception(message);
    }

    return result;
  }

}

namespace Coil
{
  VulkanSystem::VulkanSystem(VkInstance instance)
  : _instance(instance) {}

  VulkanSystem::~VulkanSystem()
  {
    vkDestroyInstance(_instance, nullptr);
  }

  VulkanSystem& VulkanSystem::Init(Book& book, Window& window, char const* appName, uint32_t appVersion)
  {
    // get instance version
    {
      uint32_t vulkanVersion;
      CheckSuccess(vkEnumerateInstanceVersion(&vulkanVersion), "getting Vulkan instance version failed");

      // not checking version, because for now we stick to Vulkan 1.0 spec + a few core extensions
    }

    // create instance
    VkInstance instance;
    {
      // get instance extensions
      std::vector<char const*> extensions;
      for(auto const& handler : _instanceExtensionsHandlers)
        handler(window, extensions);

      VkApplicationInfo appInfo =
      {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = appName,
        .applicationVersion = appVersion,
        .pEngineName = "Coil Core",
        .engineVersion = 0,
        .apiVersion = VK_HEADER_VERSION_COMPLETE,
      };

      VkInstanceCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = (uint32_t)extensions.size(),
        .ppEnabledExtensionNames = extensions.data(),
      };

      CheckSuccess(vkCreateInstance(&info, nullptr, &instance), "creating Vulkan instance failed");
    }

    return book.Allocate<VulkanSystem>(instance);
  }

  VulkanDevice& VulkanSystem::CreateDefaultDevice(Book& book)
  {
    // get physical devices
    uint32_t physicalDevicesCount = 0;
    CheckSuccess(vkEnumeratePhysicalDevices(_instance, &physicalDevicesCount, nullptr), "enumerating Vulkan physical devices failed");
    if(!physicalDevicesCount)
      throw Exception("no Vulkan physical devices");
    std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
    CheckSuccess(vkEnumeratePhysicalDevices(_instance, &physicalDevicesCount, physicalDevices.data()), "enumerating Vulkan physical devices failed");
    // note: VkPhysicalDevice handles need not to be destroyed

    // TODO: filter devices by supported features, device API version, etc

    // for default device we just use first device
    VkPhysicalDevice physicalDevice = physicalDevices[0];

    // get extension properties
    bool enablePortabilitySubsetExtension = false;
    {
      uint32_t propertiesCount = 0;
      CheckSuccess(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertiesCount, nullptr), "enumerating Vulkan device extension properties failed");
      std::vector<VkExtensionProperties> properties(propertiesCount);
      CheckSuccess(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertiesCount, properties.data()), "enumerating Vulkan device extension properties failed");

      // check extensions
      for(size_t i = 0; i < properties.size(); ++i)
      {
        if(strcmp(properties[i].extensionName, "VK_KHR_portability_subset") == 0)
          enablePortabilitySubsetExtension = true;
      }
    }

    // get device queue family properties
    uint32_t queueFamiliesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamiliesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, queueFamiliesProperties.data());

    // detect queue capabilities
    uint32_t graphicsQueueFamilyIndex = queueFamiliesCount;
    for(uint32_t i = 0; i < queueFamiliesCount; ++i)
    {
      if((queueFamiliesProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && graphicsQueueFamilyIndex >= queueFamiliesCount)
        graphicsQueueFamilyIndex = i;
    }

    if(graphicsQueueFamilyIndex >= queueFamiliesCount)
      throw Exception("no Vulkan device queue family supports graphics");

    // create device
    VkDevice device;
    {
      // currently just create single queue
      float const queuePriority = 1;
      VkDeviceQueueCreateInfo queueInfo =
      {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
      };

      std::vector<char const*> enabledExtensionNames =
      {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      };
      if(enablePortabilitySubsetExtension)
        enabledExtensionNames.push_back("VK_KHR_portability_subset");
      VkDeviceCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = (uint32_t)enabledExtensionNames.size(),
        .ppEnabledExtensionNames = enabledExtensionNames.data(),
        .pEnabledFeatures = nullptr,
      };

      CheckSuccess(vkCreateDevice(physicalDevice, &info, nullptr, &device), "creating Vulkan device failed");
    }

    VulkanDevice& deviceObj = book.Allocate<VulkanDevice>(_instance, physicalDevice, device, graphicsQueueFamilyIndex);
    deviceObj.Init();
    return deviceObj;
  }

  void VulkanSystem::RegisterInstanceExtensionsHandler(InstanceExtensionsHandler&& handler)
  {
    _instanceExtensionsHandlers.push_back(std::move(handler));
  }

  void VulkanSystem::RegisterDeviceSurfaceHandler(DeviceSurfaceHandler&& handler)
  {
    _deviceSurfaceHandlers.push_back(std::move(handler));
  }

  VkFormat VulkanSystem::GetPixelFormat(PixelFormat format)
  {
#define T(t) PixelFormat::Type::t
#define C(c) PixelFormat::Components::c
#define F(f) PixelFormat::Format::f
#define S(s) PixelFormat::Size::_##s
#define X(x) PixelFormat::Compression::x
#define R(r) VK_FORMAT_##r
    switch(format.type)
    {
    case T(Unknown):
      return R(UNDEFINED);
    case T(Uncompressed):
      switch(format.components)
      {
      case C(R):
        switch(format.format)
        {
        case F(Untyped):
          break;
        case F(Uint):
          switch(format.size)
          {
          case S(8bit): return R(R8_UNORM);
          case S(16bit): return R(R16_UNORM);
          default: break;
          }
          break;
        case F(Float):
          switch(format.size)
          {
          case S(16bit): return R(R16_SFLOAT);
          case S(32bit): return R(R32_SFLOAT);
          default: break;
          }
          break;
        }
        break;
      case C(RG):
        switch(format.format)
        {
        case F(Untyped):
          break;
        case F(Uint):
          switch(format.size)
          {
          case S(16bit): return R(R8G8_UNORM);
          case S(32bit): return R(R16G16_UNORM);
          default: break;
          }
          break;
        case F(Float):
          switch(format.size)
          {
          case S(32bit): return R(R16G16_SFLOAT);
          case S(64bit): return R(R32G32_SFLOAT);
          default: break;
          }
          break;
        }
        break;
      case C(RGB):
        switch(format.format)
        {
        case F(Untyped):
          break;
        case F(Uint):
          break;
        case F(Float):
          switch(format.size)
          {
          case S(32bit): return R(B10G11R11_UFLOAT_PACK32);
          case S(96bit): return R(R32G32B32_SFLOAT);
          default: break;
          }
          break;
        }
        break;
      case C(RGBA):
        switch(format.format)
        {
        case F(Untyped):
          break;
        case F(Uint):
          switch(format.size)
          {
          case S(32bit): return format.srgb ? R(B8G8R8A8_SRGB) : R(B8G8R8A8_UNORM);
          case S(64bit): return R(R16G16B16A16_UNORM);
          default: break;
          }
          break;
        case F(Float):
          switch(format.size)
          {
          case S(64bit): return R(R16G16B16A16_SFLOAT);
          case S(128bit): return R(R32G32B32A32_SFLOAT);
          default: break;
          }
          break;
        }
        break;
      }
      break;
    case T(Compressed):
      switch(format.compression)
      {
      case X(Bc1): return format.srgb ? R(BC1_RGB_SRGB_BLOCK) : R(BC1_RGB_UNORM_BLOCK);
      case X(Bc1Alpha): return format.srgb ? R(BC1_RGBA_SRGB_BLOCK) : R(BC1_RGBA_UNORM_BLOCK);
      case X(Bc2): return format.srgb ? R(BC2_SRGB_BLOCK) : R(BC2_UNORM_BLOCK);
      case X(Bc3): return format.srgb ? R(BC3_SRGB_BLOCK) : R(BC3_UNORM_BLOCK);
      case X(Bc4): return R(BC4_UNORM_BLOCK);
      case X(Bc4Signed): return R(BC4_SNORM_BLOCK);
      case X(Bc5): return R(BC5_UNORM_BLOCK);
      case X(Bc5Signed): return R(BC5_SNORM_BLOCK);
      default: break;
      }
      break;
    }
    throw Exception("unsupported Vulkan pixel format");
  }

  std::vector<VulkanSystem::InstanceExtensionsHandler> VulkanSystem::_instanceExtensionsHandlers;
  std::vector<VulkanSystem::DeviceSurfaceHandler> VulkanSystem::_deviceSurfaceHandlers;

  VulkanDevice::VulkanDevice(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamilyIndex)
  : _instance(instance), _physicalDevice(physicalDevice), _device(device), _graphicsQueueFamilyIndex(graphicsQueueFamilyIndex)
  {
    vkGetDeviceQueue(_device, _graphicsQueueFamilyIndex, 0, &_graphicsQueue);
  }

  VulkanDevice::~VulkanDevice()
  {
    if(_commandPool)
      vkDestroyCommandPool(_device, _commandPool, nullptr);
    vkDestroyDevice(_device, nullptr);
  }

  void VulkanDevice::Init()
  {
    // create command pool
    {
      VkCommandPoolCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = _graphicsQueueFamilyIndex,
      };
      CheckSuccess(vkCreateCommandPool(_device, &info, nullptr, &_commandPool), "creating Vulkan command pool failed");
    }

    // get memory properties
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &_memoryProperties);
  }

  VulkanPool& VulkanDevice::CreatePool(Book& book, uint64_t chunkSize)
  {
    return book.Allocate<VulkanPool>(*this, chunkSize);
  }

  VulkanPresenter& VulkanDevice::CreateWindowPresenter(Book& book, Window& window, std::function<GraphicsRecreatePresentPassFunc>&& recreatePresentPass)
  {
    // create surface for window
    VkSurfaceKHR surface = nullptr;
    for(auto const& handler : VulkanSystem::_deviceSurfaceHandlers)
    {
      surface = handler(_instance, window);
    }
    if(!surface)
      throw Exception("creating Vulkan surface failed");
    book.Allocate<VulkanSurface>(_instance, surface);

    // create presenter
    VulkanPresenter& presenter = book.Allocate<VulkanPresenter>(book.Allocate<Book>(), _physicalDevice, surface, _device, _graphicsQueue, _commandPool, std::move(recreatePresentPass));

    presenter.Init();

    window.SetPresenter(&presenter);

    return presenter;
  }

  VulkanVertexBuffer& VulkanDevice::CreateVertexBuffer(GraphicsPool& pool, Buffer const& buffer)
  {
    // create buffer
    VkBuffer vertexBuffer;
    {
      VkBufferCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = buffer.size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
      };
      CheckSuccess(vkCreateBuffer(_device, &info, nullptr, &vertexBuffer), "creating Vulkan vertex buffer failed");
      pool.GetBook().Allocate<VulkanBuffer>(_device, vertexBuffer);
    }

    // get memory requirements
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(_device, vertexBuffer, &memoryRequirements);

    // allocate memory
    // temporary, for simplicity we use host coherent memory
    // Vulkan spec guarantees that buffer's memory requirements allow such memory, and it exists on device
    std::pair<VkDeviceMemory, VkDeviceSize> memory = AllocateMemory(static_cast<VulkanPool&>(pool), memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // copy data
    {
      // map memory
      void* data;
      CheckSuccess(vkMapMemory(_device, memory.first, memory.second, memoryRequirements.size, 0, &data), "mapping Vulkan memory failed");

      // copy data
      memcpy(data, buffer.data, buffer.size);

      // unmap memory
      vkUnmapMemory(_device, memory.first);
    }

    // bind it to buffer
    CheckSuccess(vkBindBufferMemory(_device, vertexBuffer, memory.first, memory.second), "binding Vulkan memory to vertex buffer failed");

    return pool.GetBook().Allocate<VulkanVertexBuffer>(vertexBuffer);
  }

  VulkanShader& VulkanDevice::CreateShader(Book& book, GraphicsShaderRoots const& roots)
  {
    SpirvCode code = SpirvCompile(roots);

    {
      VkShaderModuleCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code.size() * sizeof(code[0]),
        .pCode = code.data(),
      };

      VkShaderModule shaderModule;
      CheckSuccess(vkCreateShaderModule(_device, &info, nullptr, &shaderModule), "creating Vulkan shader module failed");

      uint8_t stagesMask = 0;
      if(roots.vertex) stagesMask |= GraphicsShader::VertexStageFlag;
      if(roots.fragment) stagesMask |= GraphicsShader::FragmentStageFlag;

      return book.Allocate<VulkanShader>(_device, shaderModule, stagesMask);
    }
  }

  std::pair<VkDeviceMemory, VkDeviceSize> VulkanDevice::AllocateMemory(VulkanPool& pool, VkMemoryRequirements const& memoryRequirements, VkMemoryPropertyFlags requireFlags)
  {
    // find suitable memory type
    for(uint32_t memoryType = 0; memoryType < _memoryProperties.memoryTypeCount; ++memoryType)
    {
      if((memoryRequirements.memoryTypeBits & (1 << memoryType)) && (_memoryProperties.memoryTypes[memoryType].propertyFlags & requireFlags) == requireFlags)
      {
        return pool.Allocate(memoryType, memoryRequirements.size, memoryRequirements.alignment);
      }
    }

    throw Exception("no suitable Vulkan memory found");
  }

  VulkanPresenter::VulkanPresenter(
    Book& book,
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    VkDevice device,
    VkQueue queue,
    VkCommandPool commandPool,
    std::function<GraphicsRecreatePresentPassFunc>&& recreatePresentPass
    ) :
  _book(book),
  _physicalDevice(physicalDevice),
  _surface(surface),
  _device(device),
  _queue(queue),
  _commandPool(commandPool),
  _recreatePresentPass(std::move(recreatePresentPass))
  {}

  void VulkanPresenter::Init()
  {
    // get surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    CheckSuccess(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &surfaceCapabilities), "getting Vulkan physical device surface capabilities failed");

    VkExtent2D extent = surfaceCapabilities.currentExtent;

    // create swapchain
    {
      VkSwapchainCreateInfoKHR info =
      {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = _surface,
        .minImageCount = std::min(
          surfaceCapabilities.minImageCount + 1,
          surfaceCapabilities.maxImageCount > 0 ? surfaceCapabilities.maxImageCount : std::numeric_limits<uint32_t>::max()
          ),
        .imageFormat = VK_FORMAT_R8G8B8A8_SRGB,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = surfaceCapabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = nullptr,
      };

      CheckSuccess(vkCreateSwapchainKHR(_device, &info, nullptr, &_swapchain), "creating Vulkan swapchain failed");
      _book.Allocate<VulkanSwapchain>(_device, _swapchain);
    }

    // get swapchain images
    {
      uint32_t imagesCount;
      CheckSuccess(vkGetSwapchainImagesKHR(_device, _swapchain, &imagesCount, nullptr), "getting Vulkan swapchain images failed");

      _images.assign(imagesCount, nullptr);
      CheckSuccess(vkGetSwapchainImagesKHR(_device, _swapchain, &imagesCount, _images.data()), "getting Vulkan swapchain images failed");
    }

    // create a few frames
    std::vector<VkCommandBuffer> commandBuffers = VulkanCommandBuffers::Create(_book, _device, _commandPool, _framesCount, true);
    for(size_t i = 0; i < _framesCount; ++i)
      _frames[i].Init(_book, _device, _swapchain, _queue, commandBuffers[i]);

    _book.Allocate<VulkanDeviceIdle>(_device);
  }

  void VulkanPresenter::Clear()
  {
    _book.Free();
    _swapchain = nullptr;
    _images.clear();
    _frames = {};
  }

  void VulkanPresenter::Resize(ivec2 const& size)
  {
    Clear();
    Init();
  }

  VulkanPool::VulkanPool(VulkanDevice& device, VkDeviceSize chunkSize)
  : _device(device), _chunkSize(chunkSize) {}

  std::pair<VkDeviceMemory, VkDeviceSize> VulkanPool::Allocate(uint32_t memoryTypeIndex, VkDeviceSize size, VkDeviceSize alignment)
  {
    MemoryType& memoryType = _memoryTypes[memoryTypeIndex];

    // align up
    VkDeviceSize offset = (memoryType.offset + alignment - 1) & ~(alignment - 1);

    if(offset + size > memoryType.size)
    {
      if(size > _chunkSize) throw Exception("too big Vulkan memory allocation");

      memoryType = {};
      offset = 0;

      VkMemoryAllocateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = _chunkSize,
        .memoryTypeIndex = memoryTypeIndex,
      };
      CheckSuccess(vkAllocateMemory(_device._device, &info, nullptr, &memoryType.memory), "allocating Vulkan memory failed");
      _book.Allocate<VulkanMemory>(_device._device, memoryType.memory);

      memoryType.size = _chunkSize;
    }

    memoryType.offset = offset + size;

    return { memoryType.memory, offset };
  }

  VulkanFrame& VulkanPresenter::StartFrame()
  {
    // recreate swapchain if needed
    if(_recreateNeeded)
    {
      _recreateNeeded = false;
      Clear();
      Init();
    }

    // choose frame
    VulkanFrame& frame = _frames[_nextFrame++];
    _nextFrame %= _framesCount;

    // begin frame
    frame.Begin(_images, _recreateNeeded);

    return frame;
  }

  void VulkanFrame::Init(Book& book, VkDevice device, VkSwapchainKHR swapchain, VkQueue queue, VkCommandBuffer commandBuffer)
  {
    _device = device;
    _swapchain = swapchain;
    _queue = queue;
    _commandBuffer = commandBuffer;
    _fenceFrameFinished = VulkanFence::Create(book, device, true);
    _semaphoreImageAvailable = VulkanSemaphore::Create(book, device);
    _semaphoreFrameFinished = VulkanSemaphore::Create(book, device);
  }

  void VulkanFrame::Begin(std::vector<VkImage> const& images, bool& isSubOptimal)
  {
    // wait until previous use of the frame is done
    CheckSuccess(vkWaitForFences(_device, 1, &_fenceFrameFinished, VK_TRUE, std::numeric_limits<uint64_t>::max()), "waiting for Vulkan frame finished failed");
    // reset fence
    CheckSuccess(vkResetFences(_device, 1, &_fenceFrameFinished), "resetting Vulkan fence failed");

    // acquire image
    if(CheckSuccess<VK_SUBOPTIMAL_KHR>(vkAcquireNextImageKHR(_device, _swapchain, std::numeric_limits<uint64_t>::max(), _semaphoreImageAvailable, nullptr, &_imageIndex), "acquiring Vulkan next image failed") == VK_SUBOPTIMAL_KHR)
      isSubOptimal = true;
    _image = images[_imageIndex];

    // start command buffer
    {
      VkCommandBufferBeginInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
      };
      CheckSuccess(vkBeginCommandBuffer(_commandBuffer, &info), "beginning Vulkan command buffer failed");
    }
  }

  void VulkanFrame::EndFrame()
  {
    // end command buffer
    CheckSuccess(vkEndCommandBuffer(_commandBuffer), "ending Vulkan command buffer failed");

    // queue command buffer
    {
      VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      VkSubmitInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0, //1,
        .pWaitSemaphores = nullptr, //&_semaphoreImageAvailable,
        .pWaitDstStageMask = &waitDstStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &_commandBuffer,
        .signalSemaphoreCount = 0, // 1,
        .pSignalSemaphores = nullptr, //&_semaphoreFrameFinished,
      };
      CheckSuccess(vkQueueSubmit(_queue, 1, &info, _fenceFrameFinished), "queueing Vulkan command buffer failed");
    }

    // present
    {
      VkPresentInfoKHR info =
      {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 0, //1,
        .pWaitSemaphores = nullptr, //&_semaphoreFrameFinished,
        .swapchainCount = 1,
        .pSwapchains = &_swapchain,
        .pImageIndices = &_imageIndex,
        .pResults = nullptr,
      };
      CheckSuccess<VK_SUBOPTIMAL_KHR>(vkQueuePresentKHR(_queue, &info), "queueing Vulkan present failed");
    }
  }

  VulkanSurface::VulkanSurface(VkInstance instance, VkSurfaceKHR surface)
  : _instance(instance), _surface(surface) {}

  VulkanSurface::~VulkanSurface()
  {
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
  }

  VulkanDeviceIdle::VulkanDeviceIdle(VkDevice device)
  : _device(device) {}

  VulkanDeviceIdle::~VulkanDeviceIdle()
  {
    CheckSuccess(vkDeviceWaitIdle(_device), "waiting for Vulkan device idle failed\n");
  }

  VulkanSwapchain::VulkanSwapchain(VkDevice device, VkSwapchainKHR swapchain)
  : _device(device), _swapchain(swapchain) {}

  VulkanSwapchain::~VulkanSwapchain()
  {
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
  }

  VulkanCommandBuffers::VulkanCommandBuffers(VkDevice device, VkCommandPool commandPool, std::vector<VkCommandBuffer>&& commandBuffers)
  : _device(device), _commandPool(commandPool), _commandBuffers(std::move(commandBuffers)) {}

  VulkanCommandBuffers::~VulkanCommandBuffers()
  {
    vkFreeCommandBuffers(_device, _commandPool, _commandBuffers.size(), _commandBuffers.data());
  }

  std::vector<VkCommandBuffer> VulkanCommandBuffers::Create(Book& book, VkDevice device, VkCommandPool commandPool, uint32_t count, bool primary)
  {
    if(!count) return {};

    VkCommandBufferAllocateInfo info =
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

  VulkanRenderPass::VulkanRenderPass(VkDevice device, VkRenderPass renderPass)
  : _device(device), _renderPass(renderPass) {}

  VulkanRenderPass::~VulkanRenderPass()
  {
    vkDestroyRenderPass(_device, _renderPass, nullptr);
  }

  VulkanPipeline::VulkanPipeline(VkDevice device, VkPipeline pipeline)
  : _device(device), _pipeline(pipeline) {}

  VulkanPipeline::~VulkanPipeline()
  {
    vkDestroyPipeline(_device, _pipeline, nullptr);
  }

  VulkanPipelineLayout::VulkanPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout)
  : _device(device), _pipelineLayout(pipelineLayout) {}

  VulkanPipelineLayout::~VulkanPipelineLayout()
  {
    vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
  }

  VulkanFramebuffer::VulkanFramebuffer(VkDevice device, VkFramebuffer framebuffer)
  : _device(device), _framebuffer(framebuffer) {}

  VulkanFramebuffer::~VulkanFramebuffer()
  {
    vkDestroyFramebuffer(_device, _framebuffer, nullptr);
  }

  VulkanBuffer::VulkanBuffer(VkDevice device, VkBuffer buffer)
  : _device(device), _buffer(buffer) {}

  VulkanBuffer::~VulkanBuffer()
  {
    vkDestroyBuffer(_device, _buffer, nullptr);
  }

  VulkanImageView::VulkanImageView(VkDevice device, VkImageView imageView)
  : _device(device), _imageView(imageView) {}

  VulkanImageView::~VulkanImageView()
  {
    vkDestroyImageView(_device, _imageView, nullptr);
  }

  VulkanVertexBuffer::VulkanVertexBuffer(VkBuffer buffer)
  : _buffer(buffer) {}

  VulkanShader::VulkanShader(VkDevice device, VkShaderModule shaderModule, uint8_t stagesMask)
  : device(device), shaderModule(shaderModule), stagesMask(stagesMask) {}

  VulkanShader::~VulkanShader()
  {
    vkDestroyShaderModule(device, shaderModule, nullptr);
  }

  VulkanMemory::VulkanMemory(VkDevice device, VkDeviceMemory memory)
  : _device(device), _memory(memory) {}

  VulkanMemory::~VulkanMemory()
  {
    vkFreeMemory(_device, _memory, nullptr);
  }

  VulkanFence::VulkanFence(VkDevice device, VkFence fence)
  : _device(device), _fence(fence) {}

  VulkanFence::~VulkanFence()
  {
    vkDestroyFence(_device, _fence, nullptr);
  }

  VkFence VulkanFence::Create(Book& book, VkDevice device, bool signaled)
  {
    VkFenceCreateInfo info =
    {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VkFlags(signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0),
    };
    VkFence fence;
    CheckSuccess(vkCreateFence(device, &info, nullptr, &fence), "creating Vulkan fence failed");
    book.Allocate<VulkanFence>(device, fence);
    return fence;
  }

  VulkanSemaphore::VulkanSemaphore(VkDevice device, VkSemaphore semaphore)
  : _device(device), _semaphore(semaphore) {}

  VulkanSemaphore::~VulkanSemaphore()
  {
    vkDestroySemaphore(_device, _semaphore, nullptr);
  }

  VkSemaphore VulkanSemaphore::Create(Book& book, VkDevice device)
  {
    VkSemaphoreCreateInfo info =
    {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
    };
    VkSemaphore semaphore;
    CheckSuccess(vkCreateSemaphore(device, &info, nullptr, &semaphore), "creating Vulkan semaphore failed");
    book.Allocate<VulkanSemaphore>(device, semaphore);
    return semaphore;
  }
}
