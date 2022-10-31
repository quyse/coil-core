#include "vulkan.hpp"
#include "vulkan_objects.hpp"
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

  VulkanSystem& VulkanSystem::Create(Book& book, Window& window, char const* appName, uint32_t appVersion)
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
        .apiVersion = VK_API_VERSION_1_0,
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
      AllocateVulkanObject(book, instance);
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
      AllocateVulkanObject(book, device);
    }

    VulkanDevice& vulkanDevice = book.Allocate<VulkanDevice>(book.Allocate<Book>(), _instance, physicalDevice, device, graphicsQueueFamilyIndex);
    book.Allocate<VulkanDeviceIdle>(device);
    return vulkanDevice;
  }

  void VulkanSystem::RegisterInstanceExtensionsHandler(InstanceExtensionsHandler&& handler)
  {
    _instanceExtensionsHandlers.push_back(std::move(handler));
  }

  void VulkanSystem::RegisterDeviceSurfaceHandler(DeviceSurfaceHandler&& handler)
  {
    _deviceSurfaceHandlers.push_back(std::move(handler));
  }

  std::vector<VulkanSystem::InstanceExtensionsHandler> VulkanSystem::_instanceExtensionsHandlers;
  std::vector<VulkanSystem::DeviceSurfaceHandler> VulkanSystem::_deviceSurfaceHandlers;

  VulkanDevice::VulkanDevice(Book& book, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamilyIndex)
  : _book(book), _instance(instance), _physicalDevice(physicalDevice), _device(device), _graphicsQueueFamilyIndex(graphicsQueueFamilyIndex)
  {
    vkGetDeviceQueue(_device, _graphicsQueueFamilyIndex, 0, &_graphicsQueue);

    // query physical device properties
    vkGetPhysicalDeviceProperties(_physicalDevice, &_properties);

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
      AllocateVulkanObject(book, _device, _commandPool);
    }

    // get memory properties
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &_memoryProperties);
  }

  Book& VulkanDevice::GetBook()
  {
    return _book;
  }

  VulkanPool& VulkanDevice::CreatePool(Book& book, size_t chunkSize)
  {
    return book.Allocate<VulkanPool>(*this, chunkSize);
  }

  VulkanPresenter& VulkanDevice::CreateWindowPresenter(Book& book, GraphicsPool& graphicsPool, Window& window, std::function<GraphicsRecreatePresentFunc>&& recreatePresent, std::function<GraphicsRecreatePresentPerImageFunc>&& recreatePresentPerImage)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);

    // create surface for window
    VkSurfaceKHR surface = nullptr;
    for(auto const& handler : VulkanSystem::_deviceSurfaceHandlers)
    {
      surface = handler(_instance, window);
    }
    if(!surface)
      throw Exception("creating Vulkan surface failed");
    AllocateVulkanObject(book, _instance, surface);

    // create presenter
    VulkanPresenter& presenter = book.Allocate<VulkanPresenter>(*this, book, surface, pool, std::move(recreatePresent), std::move(recreatePresentPerImage));

    presenter.Init({});

    window.SetPresenter(&presenter);

    return presenter;
  }

  VulkanVertexBuffer& VulkanDevice::CreateVertexBuffer(Book& book, GraphicsPool& graphicsPool, Buffer const& buffer)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);

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
      AllocateVulkanObject(book, _device, vertexBuffer);
    }

    // get memory requirements
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(_device, vertexBuffer, &memoryRequirements);

    // allocate memory
    // temporary, for simplicity we use host coherent memory
    // Vulkan spec guarantees that buffer's memory requirements allow such memory, and it exists on device
    std::pair<VkDeviceMemory, VkDeviceSize> memory = AllocateMemory(pool, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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

    return book.Allocate<VulkanVertexBuffer>(vertexBuffer);
  }

  VulkanIndexBuffer& VulkanDevice::CreateIndexBuffer(Book& book, GraphicsPool& graphicsPool, Buffer const& buffer, bool is32Bit)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);

    // create buffer
    VkBuffer indexBuffer;
    {
      VkBufferCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = buffer.size,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
      };
      CheckSuccess(vkCreateBuffer(_device, &info, nullptr, &indexBuffer), "creating Vulkan index buffer failed");
      AllocateVulkanObject(book, _device, indexBuffer);
    }

    // get memory requirements
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(_device, indexBuffer, &memoryRequirements);

    // allocate memory
    // temporary, for simplicity we use host coherent memory
    // Vulkan spec guarantees that buffer's memory requirements allow such memory, and it exists on device
    std::pair<VkDeviceMemory, VkDeviceSize> memory = AllocateMemory(pool, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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
    CheckSuccess(vkBindBufferMemory(_device, indexBuffer, memory.first, memory.second), "binding Vulkan memory to index buffer failed");

    return book.Allocate<VulkanIndexBuffer>(indexBuffer, is32Bit);
  }

  VulkanImage& VulkanDevice::CreateDepthStencilImage(Book& book, GraphicsPool& graphicsPool, ivec2 const& size)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);

    VkFormat const format = VK_FORMAT_D24_UNORM_S8_UINT;

    // create image
    VkImage image;
    {
      VkImageCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent =
        {
          .width = (uint32_t)size.x(),
          .height = (uint32_t)size.y(),
          .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      };
      CheckSuccess(vkCreateImage(_device, &info, nullptr, &image), "creating Vulkan depth stencil image failed");
      AllocateVulkanObject(book, _device, image);
    }

    // get memory requirements
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(_device, image, &memoryRequirements);
    // allocate memory
    std::pair<VkDeviceMemory, VkDeviceSize> memory = AllocateMemory(pool, memoryRequirements, 0);
    // bind memory
    CheckSuccess(vkBindImageMemory(_device, image, memory.first, memory.second), "binding Vulkan memory to depth stencil image failed");

    // create image view
    VkImageView imageView;
    {
      VkImageViewCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components = {},
        .subresourceRange =
        {
          .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      };
      CheckSuccess(vkCreateImageView(_device, &info, nullptr, &imageView), "creating Vulkan depth stencil image view failed");
      AllocateVulkanObject(book, _device, imageView);
    }

    return book.Allocate<VulkanImage>(image, imageView);
  }

  VulkanPass& VulkanDevice::CreatePass(Book& book, GraphicsPassConfig const& config)
  {
    using AttachmentId = GraphicsPassConfig::AttachmentId;
    using SubPassId = GraphicsSubPassId;

    size_t attachmentsCount = config.attachments.size();
    size_t subPassesCount = config.subPasses.size();

    // process passes
    std::vector<VkAttachmentDescription> attachmentsDescriptions(attachmentsCount);
    std::vector<VkSubpassDescription> subPassesDescriptions(subPassesCount);
    std::vector<VkAttachmentReference> colorAttachmentsReferences;
    std::vector<VkAttachmentReference> depthStencilAttachmentsReferences(attachmentsCount);
    std::vector<VkAttachmentReference> inputAttachmentsReferences;
    std::vector<uint32_t> preserveAttachmentsReferences;
    uint32_t subPassesColorAttachmentsCount = 0;
    uint32_t subPassesInputAttachmentsCount = 0;
    struct AttachmentInfo
    {
      VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };
    std::vector<AttachmentInfo> attachmentsInfos(attachmentsCount);
    struct Dependency
    {
      VkPipelineStageFlags srcStageMask = 0;
      VkAccessFlags srcAccessMask = 0;
      VkPipelineStageFlags dstStageMask = 0;
      VkAccessFlags dstAccessMask = 0;

      void operator|=(Dependency const& dependency)
      {
        srcStageMask |= dependency.srcStageMask;
        srcAccessMask |= dependency.srcAccessMask;
        dstStageMask |= dependency.dstStageMask;
        dstAccessMask |= dependency.dstAccessMask;
      }
    };
    struct SubPassInfo
    {
      SubPassInfo(size_t subPassesCount)
      : dependencies(subPassesCount) {}

      std::vector<Dependency> dependencies;
      std::vector<AttachmentId> preserveAttachments;
    };
    std::vector<SubPassInfo> subPassesInfos(subPassesCount, subPassesCount);
    std::vector<VkSubpassDependency> subPassesDependencies;
    std::vector<std::optional<std::tuple<SubPassId, VkPipelineStageFlags, VkAccessFlags>>> attachmentsLastWriter(attachmentsCount);
    std::vector<std::vector<std::tuple<SubPassId, VkPipelineStageFlags>>> attachmentsLastReaders(attachmentsCount);
    std::vector<Dependency> currentSubPassDependencies;
    std::vector<AttachmentId> currentPassReadAttachments;
    for(SubPassId subPassId = 0; subPassId < subPassesCount; ++subPassId)
    {
      GraphicsPassConfig::SubPass const& subPass = config.subPasses[subPassId];
      SubPassInfo& subPassInfo = subPassesInfos[subPassId];

      uint32_t subPassColorAttachmentsCount = 0;
      uint32_t subPassInputAttachmentsCount = 0;

      std::optional<AttachmentId> depthStencilAttachmentId;

      // process attachments
      for(auto const& it : subPass.attachments)
      {
        AttachmentId attachmentId = it.first;
        AttachmentInfo& attachmentInfo = attachmentsInfos[attachmentId];

        // note usage of attachment
        VkPipelineStageFlags readStageMask = 0, writeStageMask = 0;
        VkAccessFlags readAccessMask = 0, writeAccessMask = 0;
        std::visit([&](auto const& attachment)
        {
          using T = std::decay_t<decltype(attachment)>;
          // color attachment
          if constexpr(std::is_same_v<T, GraphicsPassConfig::SubPass::ColorAttachment>)
          {
            colorAttachmentsReferences.push_back({
              .attachment = attachmentId,
              .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            });
            subPassColorAttachmentsCount = std::max(subPassColorAttachmentsCount, attachmentId + 1);

            if(attachmentInfo.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
              attachmentInfo.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            readStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            readAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            writeStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            writeAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
          }
          // depth stencil attachment
          if constexpr(std::is_same_v<T, GraphicsPassConfig::SubPass::DepthStencilAttachment>)
          {
            if(depthStencilAttachmentId.has_value()) throw Exception("more than one Vulkan depth-stencil attachments in pass");
            depthStencilAttachmentId = attachmentId;
            depthStencilAttachmentsReferences[attachmentId] =
            {
              .attachment = attachmentId,
              .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            };

            if(attachmentInfo.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
              attachmentInfo.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            readStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            readAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            writeStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            writeAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
          }
          // input attachment
          if constexpr(std::is_same_v<T, GraphicsPassConfig::SubPass::InputAttachment>)
          {
            inputAttachmentsReferences.push_back({
              .attachment = attachmentId,
              .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            });
            subPassInputAttachmentsCount = std::max(subPassInputAttachmentsCount, attachmentId + 1);

            if(attachmentInfo.initialLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
              attachmentInfo.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            readStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            readAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
          }
          // shader attachment
          if constexpr(std::is_same_v<T, GraphicsPassConfig::SubPass::ShaderAttachment>)
          {
            if(attachmentInfo.initialLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
              attachmentInfo.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            readStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            readAccessMask = VK_ACCESS_SHADER_READ_BIT;
          }
        }, it.second);

        currentPassReadAttachments.push_back(attachmentId);

        // add dependency on writer subpass of this attachment
        if(attachmentsLastWriter[attachmentId].has_value())
        {
          auto const& [lastWriterSubPassId, lastWriterStageMask, lastWriterAccessMask] = attachmentsLastWriter[attachmentId].value();
          Dependency& dependency = subPassInfo.dependencies[lastWriterSubPassId];
          dependency.srcStageMask |= lastWriterStageMask;
          dependency.srcAccessMask |= lastWriterAccessMask;
          dependency.dstStageMask |= readStageMask;
          dependency.dstAccessMask |= readAccessMask;
        }

        // if this subpass writes to the attachment
        if(writeAccessMask)
        {
          // add dependencies on reader subpasses
          for(auto const& [lastReaderSubPassId, lastReaderStageMask] : attachmentsLastReaders[attachmentId])
          {
            Dependency& dependency = subPassInfo.dependencies[lastReaderSubPassId];
            // write-after-read does not need access scope
            dependency.srcStageMask |= lastReaderStageMask;
            dependency.dstStageMask |= writeStageMask;
          }

          // record it as last writer
          attachmentsLastWriter[attachmentId] = { subPassId, writeStageMask, writeAccessMask };
          // clear reader subpasses
          attachmentsLastReaders[attachmentId].clear();
        }
        // else if this subpass reads the attachment
        else if(readAccessMask)
        {
          // add it as a reader
          attachmentsLastReaders[attachmentId].push_back({ subPassId, readStageMask });
        }
      }

      // process dependencies in reverse order, so we can filter out dependencies
      // which are already transitively accounted for
      currentSubPassDependencies.assign(subPassesCount, {});
      for(int32_t dependencySubPassId = subPassId - 1; dependencySubPassId >= 0; --dependencySubPassId)
      {
        auto const& dependency = subPassInfo.dependencies[dependencySubPassId];
        auto& currentDependency = currentSubPassDependencies[dependencySubPassId];

        bool const isDirectDependency = dependency.srcStageMask || dependency.dstStageMask;
        bool const isTransitiveDependency = currentDependency.srcStageMask || currentDependency.dstStageMask;
        // if we do not depend on this subpass (directly or transitively), skip
        if(!isDirectDependency && !isTransitiveDependency) continue;

        // if this is direct dependency not yet transitively accounted for
        if(isDirectDependency && (
          (dependency.srcStageMask & ~currentDependency.srcStageMask) ||
          (dependency.srcAccessMask & ~currentDependency.srcAccessMask) ||
          (dependency.dstStageMask & ~currentDependency.dstStageMask) ||
          (dependency.dstAccessMask & ~currentDependency.dstAccessMask)
        ))
        {
          // create dependency
          subPassesDependencies.push_back({
            .srcSubpass = (uint32_t)dependencySubPassId,
            .dstSubpass = subPassId,
            .srcStageMask = dependency.srcStageMask,
            .dstStageMask = dependency.dstStageMask,
            .srcAccessMask = dependency.srcAccessMask,
            .dstAccessMask = dependency.dstAccessMask,
          });
          // account for it
          currentDependency |= dependency;
        }

        // account for sub-dependencies
        SubPassInfo& dependencySubPassInfo = subPassesInfos[dependencySubPassId];
        for(SubPassId subDependencySubPassId = 0; subDependencySubPassId < dependencySubPassId; ++subDependencySubPassId)
        {
          currentSubPassDependencies[subDependencySubPassId] |= dependencySubPassInfo.dependencies[subDependencySubPassId];
        }

        // preserve attachments the subpass depends on
        dependencySubPassInfo.preserveAttachments.insert(dependencySubPassInfo.preserveAttachments.end(), currentPassReadAttachments.begin(), currentPassReadAttachments.end());
      }

      subPassesDescriptions[subPassId] =
      {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = subPassInputAttachmentsCount,
        .pInputAttachments = (VkAttachmentReference const*)(uintptr_t)subPassesInputAttachmentsCount, // to be fixed up later
        .colorAttachmentCount = subPassColorAttachmentsCount,
        .pColorAttachments = (VkAttachmentReference const*)(uintptr_t)subPassesColorAttachmentsCount, // to be fixed up later
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = depthStencilAttachmentId.has_value() ? &depthStencilAttachmentsReferences[depthStencilAttachmentId.value()] : nullptr,
        .preserveAttachmentCount = 0, // later
        .pPreserveAttachments = nullptr, // later
      };

      // array counts
      subPassesColorAttachmentsCount += subPassColorAttachmentsCount;
      subPassesInputAttachmentsCount += subPassInputAttachmentsCount;

      currentPassReadAttachments.clear();
    }

    // calculate preserve attachments
    for(SubPassId subPassId = 0; subPassId < subPassesCount; ++subPassId)
    {
      GraphicsPassConfig::SubPass const& subPass = config.subPasses[subPassId];
      SubPassInfo& subPassInfo = subPassesInfos[subPassId];

      std::sort(subPassInfo.preserveAttachments.begin(), subPassInfo.preserveAttachments.end());
      subPassInfo.preserveAttachments.resize(std::unique(subPassInfo.preserveAttachments.begin(), subPassInfo.preserveAttachments.end()) - subPassInfo.preserveAttachments.begin());

      subPassesDescriptions[subPassId].pPreserveAttachments = (uint32_t const*)(uintptr_t)preserveAttachmentsReferences.size(); // to be fixed up later

      for(size_t i = 0; i < subPassInfo.preserveAttachments.size(); ++i)
      {
        AttachmentId attachmentId = subPassInfo.preserveAttachments[i];

        // filter out used attachments
        if(subPass.attachments.find(attachmentId) != subPass.attachments.end()) continue;

        preserveAttachmentsReferences.push_back(attachmentId);
        ++subPassesDescriptions[subPassId].preserveAttachmentCount;
      }
    }

    // fix up attachment references
    for(SubPassId subPassId = 0; subPassId < subPassesCount; ++subPassId)
    {
      VkSubpassDescription& desc = subPassesDescriptions[subPassId];
      desc.pInputAttachments = inputAttachmentsReferences.data() + (uintptr_t)desc.pInputAttachments;
      desc.pColorAttachments = colorAttachmentsReferences.data() + (uintptr_t)desc.pColorAttachments;
      desc.pPreserveAttachments = preserveAttachmentsReferences.data() + (uintptr_t)desc.pPreserveAttachments;
    }

    // attachments
    std::vector<VkClearValue> clearValues(attachmentsCount);
    for(AttachmentId attachmentId = 0; attachmentId < attachmentsCount; ++attachmentId)
    {
      GraphicsPassConfig::Attachment const& attachment = config.attachments[attachmentId];
      VkFormat format = VK_FORMAT_UNDEFINED;
      VkClearValue& clearValue = clearValues[attachmentId];
      std::visit([&](auto const& config)
      {
        using T = std::decay_t<decltype(config)>;
        if constexpr(std::is_same_v<T, GraphicsPassConfig::ColorAttachmentConfig>)
        {
          format = std::visit([&](auto const& attachmentPixelFormat) -> VkFormat
          {
            using T = std::decay_t<decltype(attachmentPixelFormat)>;
            if constexpr(std::is_same_v<T, PixelFormat>)
              return VulkanSystem::GetPixelFormat(attachmentPixelFormat);
            if constexpr(std::is_same_v<T, GraphicsOpaquePixelFormat>)
              return (VkFormat)attachmentPixelFormat;
          }, config.format);
          clearValue.color =
          {
            .float32 =
            {
              config.clearColor.x(),
              config.clearColor.y(),
              config.clearColor.z(),
              config.clearColor.w(),
            },
          };
        }
        if constexpr(std::is_same_v<T, GraphicsPassConfig::DepthStencilAttachmentConfig>)
        {
          format = VK_FORMAT_D24_UNORM_S8_UINT;
          clearValue.depthStencil =
          {
            .depth = config.clearDepth,
            .stencil = config.clearStencil,
          };
        }
      }, attachment.config);

      attachmentsDescriptions[attachmentId] =
      {
        .flags = 0,
        .format = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = attachment.keepBefore ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = attachment.keepAfter ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = attachment.keepBefore ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR,
        .stencilStoreOp = attachment.keepAfter ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = attachment.keepBefore ? attachmentsInfos[attachmentId].initialLayout : VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = attachmentsInfos[attachmentId].finalLayout,
      };
    }

    VkRenderPassCreateInfo info =
    {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .attachmentCount = (uint32_t)attachmentsCount,
      .pAttachments = attachmentsDescriptions.data(),
      .subpassCount = (uint32_t)subPassesCount,
      .pSubpasses = subPassesDescriptions.data(),
      .dependencyCount = (uint32_t)subPassesDependencies.size(),
      .pDependencies = subPassesDependencies.data(),
    };
    VkRenderPass renderPass;
    CheckSuccess(vkCreateRenderPass(_device, &info, nullptr, &renderPass), "creating Vulkan render pass failed");
    AllocateVulkanObject(book, _device, renderPass);

    return book.Allocate<VulkanPass>(renderPass, std::move(clearValues), subPassesCount);
  }

  VulkanShader& VulkanDevice::CreateShader(Book& book, GraphicsShaderRoots const& roots)
  {
    SpirvModule module = SpirvCompile(roots);

    {
      VkShaderModuleCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = module.code.size() * sizeof(module.code[0]),
        .pCode = module.code.data(),
      };

      VkShaderModule shaderModule;
      CheckSuccess(vkCreateShaderModule(_device, &info, nullptr, &shaderModule), "creating Vulkan shader module failed");

      VkShaderStageFlags stagesMask = 0;
      if(roots.vertex) stagesMask |= VK_SHADER_STAGE_VERTEX_BIT;
      if(roots.fragment) stagesMask |= VK_SHADER_STAGE_FRAGMENT_BIT;

      AllocateVulkanObject(book, _device, shaderModule);
      return book.Allocate<VulkanShader>(shaderModule, stagesMask, std::move(module.descriptorSetLayouts));
    }
  }

  VulkanPipelineLayout& VulkanDevice::CreatePipelineLayout(Book& book, std::span<GraphicsShader*> const& graphicsShaders)
  {
    // create descriptor set layouts
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    {
      // merge descriptor set layouts from all shaders
      std::vector<SpirvDescriptorSetLayout> mergedSets;
      for(GraphicsShader* graphicsShader : graphicsShaders)
      {
        VulkanShader& shader = static_cast<VulkanShader&>(*graphicsShader);
        auto const& sets = shader._descriptorSetLayouts;

        if(mergedSets.size() < sets.size())
          mergedSets.resize(sets.size());

        for(size_t i = 0; i < sets.size(); ++i)
        {
          auto const& set = sets[i];
          auto& mergedSet = mergedSets[i];

          if(mergedSet.bindings.size() < set.bindings.size())
            mergedSet.bindings.resize(set.bindings.size());

          for(size_t j = 0; j < set.bindings.size(); ++j)
          {
            auto const& binding = set.bindings[j];
            auto& mergedBinding = mergedSet.bindings[j];

            if(mergedBinding.descriptorType != binding.descriptorType)
            {
              if(mergedBinding.descriptorType != SpirvDescriptorType::Unused)
                throw Exception("conflicting Vulkan descriptor type when creating pipeline layout");
              mergedBinding.descriptorType = binding.descriptorType;
            }
            mergedBinding.descriptorCount = std::max(mergedBinding.descriptorCount, binding.descriptorCount);
            mergedBinding.stageFlags |= binding.stageFlags;
          }
        }
      }

      // create descriptor set layouts
      descriptorSetLayouts.resize(mergedSets.size());
      for(size_t i = 0; i < mergedSets.size(); ++i)
      {
        auto const& mergedSet = mergedSets[i];

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(mergedSets[i].bindings.size());

        for(size_t j = 0; j < mergedSet.bindings.size(); ++j)
        {
          auto const& mergedBinding = mergedSet.bindings[j];

          if(mergedBinding.descriptorType == SpirvDescriptorType::Unused)
            continue;

          VkDescriptorType descriptorType;
          switch(mergedBinding.descriptorType)
          {
          case SpirvDescriptorType::UniformBuffer:
            descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
          case SpirvDescriptorType::SampledImage:
            descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            break;
          default:
            throw Exception("unsupported Vulkan descriptor type from SPIR-V module");
          }

          VkShaderStageFlags stageFlags = 0;
          if(mergedBinding.stageFlags & (uint32_t)SpirvStageFlag::Vertex) stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
          if(mergedBinding.stageFlags & (uint32_t)SpirvStageFlag::Fragment) stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;

          bindings.push_back(
          {
            .binding = (uint32_t)j,
            .descriptorType = descriptorType,
            .descriptorCount = mergedBinding.descriptorCount,
            .stageFlags = stageFlags,
            .pImmutableSamplers = nullptr,
          });
        }

        VkDescriptorSetLayoutCreateInfo info =
        {
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .bindingCount = (uint32_t)bindings.size(),
          .pBindings = bindings.data(),
        };
        CheckSuccess(vkCreateDescriptorSetLayout(_device, &info, nullptr, &descriptorSetLayouts[i]), "creating Vulkan descriptor set layout failed");
        AllocateVulkanObject(book, _device, descriptorSetLayouts[i]);
      }
    }

    VkPipelineLayoutCreateInfo info =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = (uint32_t)descriptorSetLayouts.size(),
      .pSetLayouts = descriptorSetLayouts.data(),
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
    };
    VkPipelineLayout pipelineLayout;
    CheckSuccess(vkCreatePipelineLayout(_device, &info, nullptr, &pipelineLayout), "creating Vulkan pipeline layout failed");
    AllocateVulkanObject(book, _device, pipelineLayout);
    return book.Allocate<VulkanPipelineLayout>(pipelineLayout, std::move(descriptorSetLayouts));
  }

  VulkanPipeline& VulkanDevice::CreatePipeline(Book& book, GraphicsPipelineConfig const& config, GraphicsPipelineLayout& graphicsPipelineLayout, GraphicsPass& graphicsPass, GraphicsSubPassId subPassId, GraphicsShader& graphicsShader)
  {
    VulkanPipelineLayout& pipelineLayout = static_cast<VulkanPipelineLayout&>(graphicsPipelineLayout);
    VulkanPass& pass = static_cast<VulkanPass&>(graphicsPass);
    VulkanShader& shader = static_cast<VulkanShader&>(graphicsShader);

    std::vector<VkPipelineShaderStageCreateInfo> stagesInfos;
    auto addShaderStage = [&](VkShaderStageFlagBits flag, char const* entryPoint)
    {
      if(shader._stagesMask & flag)
      {
        stagesInfos.push_back(
        {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = flag,
          .module = shader._shaderModule,
          .pName = entryPoint,
          .pSpecializationInfo = nullptr,
        });
      }
    };
    addShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "mainVertex");
    addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "mainFragment");

    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
    for(size_t i = 0; i < config.vertexLayout.slots.size(); ++i)
    {
      auto const& slot = config.vertexLayout.slots[i];
      vertexBindingDescriptions.push_back(
      {
        .binding = (uint32_t)i,
        .stride = slot.stride,
        .inputRate = slot.perInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX,
      });
    }

    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
    for(size_t i = 0; i < config.vertexLayout.attributes.size(); ++i)
    {
      auto const& attribute = config.vertexLayout.attributes[i];
      vertexAttributeDescriptions.push_back(
      {
        .location = (uint32_t)i,
        .binding = attribute.slot,
        .format = VulkanSystem::GetVertexFormat(attribute.format),
        .offset = attribute.offset,
      });
    }

    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount = (uint32_t)vertexBindingDescriptions.size(),
      .pVertexBindingDescriptions = vertexBindingDescriptions.data(),
      .vertexAttributeDescriptionCount = (uint32_t)vertexAttributeDescriptions.size(),
      .pVertexAttributeDescriptions = vertexAttributeDescriptions.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport =
    {
      .x = 0,
      .y = 0,
      .width = (float)config.viewport.x(),
      .height = (float)config.viewport.y(),
      .minDepth = 0,
      .maxDepth = 1,
    };
    VkRect2D scissor =
    {
      .offset = { 0, 0 },
      .extent = { (uint32_t)config.viewport.x(), (uint32_t)config.viewport.y() },
    };
    VkPipelineViewportStateCreateInfo viewportStateInfo =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizationState =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .depthBiasConstantFactor = 0,
      .depthBiasClamp = 0,
      .depthBiasSlopeFactor = 0,
      .lineWidth = 1,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilState =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .depthTestEnable = config.depthTest ? VK_TRUE : VK_FALSE,
      .depthWriteEnable = config.depthWrite ? VK_TRUE : VK_FALSE,
      .depthCompareOp = VulkanSystem::GetCompareOp(config.depthCompareOp),
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
      .front = {},
      .back = {},
      .minDepthBounds = 0,
      .maxDepthBounds = 0,
    };

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates(config.attachments.size());
    for(size_t i = 0; i < config.attachments.size(); ++i)
    {
      auto const& optionalBlending = config.attachments[i].blending;
      if(optionalBlending.has_value())
      {
        auto const& blending = optionalBlending.value();
        colorBlendAttachmentStates[i] =
        {
          .blendEnable = VK_TRUE,
          .srcColorBlendFactor = VulkanSystem::GetColorBlendFactor(blending.srcColorBlendFactor),
          .dstColorBlendFactor = VulkanSystem::GetColorBlendFactor(blending.dstColorBlendFactor),
          .colorBlendOp = VulkanSystem::GetBlendOp(blending.colorBlendOp),
          .srcAlphaBlendFactor = VulkanSystem::GetAlphaBlendFactor(blending.srcAlphaBlendFactor),
          .dstAlphaBlendFactor = VulkanSystem::GetAlphaBlendFactor(blending.dstAlphaBlendFactor),
          .alphaBlendOp = VulkanSystem::GetBlendOp(blending.alphaBlendOp),
          .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
      }
      else
      {
        colorBlendAttachmentStates[i] =
        {
          .blendEnable = VK_FALSE,
          .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
      }
    }

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_CLEAR,
      .attachmentCount = (uint32_t)colorBlendAttachmentStates.size(),
      .pAttachments = colorBlendAttachmentStates.data(),
      .blendConstants = { 0 },
    };

    VkGraphicsPipelineCreateInfo info =
    {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stageCount = (uint32_t)stagesInfos.size(),
      .pStages = stagesInfos.data(),
      .pVertexInputState = &vertexInputStateInfo,
      .pInputAssemblyState = &inputAssemblyState,
      .pTessellationState = nullptr,
      .pViewportState = &viewportStateInfo,
      .pRasterizationState = &rasterizationState,
      .pMultisampleState = nullptr,
      .pDepthStencilState = &depthStencilState,
      .pColorBlendState = &colorBlendStateInfo,
      .pDynamicState = nullptr,
      .layout = pipelineLayout._pipelineLayout,
      .renderPass = pass._renderPass,
      .subpass = subPassId,
      .basePipelineHandle = nullptr,
      .basePipelineIndex = -1,
    };

    VkPipeline pipeline;
    CheckSuccess(vkCreateGraphicsPipelines(_device, nullptr, 1, &info, nullptr, &pipeline), "creating Vulkan graphics pipeline failed");
    AllocateVulkanObject(book, _device, pipeline);
    return book.Allocate<VulkanPipeline>(pipeline, pipelineLayout);
  }

  VulkanFramebuffer& VulkanDevice::CreateFramebuffer(Book& book, GraphicsPass& graphicsPass, std::span<GraphicsImage*> const& pImages, ivec2 const& size)
  {
    VulkanPass& pass = static_cast<VulkanPass&>(graphicsPass);

    std::vector<VkImageView> attachments;
    attachments.reserve(pImages.size());
    for(auto const& pImage : pImages)
      attachments.push_back(static_cast<VulkanImage&>(*pImage)._imageView);

    VkFramebufferCreateInfo info =
    {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderPass = pass._renderPass,
      .attachmentCount = (uint32_t)attachments.size(),
      .pAttachments = attachments.data(),
      .width = (uint32_t)size.x(),
      .height = (uint32_t)size.y(),
      .layers = 1,
    };
    VkFramebuffer framebuffer;
    CheckSuccess(vkCreateFramebuffer(_device,  &info, nullptr, &framebuffer), "creating Vulkan framebuffer failed");
    AllocateVulkanObject(book, _device, framebuffer);
    return book.Allocate<VulkanFramebuffer>(framebuffer, size);
  }

  VulkanImage& VulkanDevice::CreateTexture(Book& book, GraphicsPool& graphicsPool, GraphicsImageFormat const& format, GraphicsSampler* pGraphicsSampler)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);
    VulkanSampler* pSampler = static_cast<VulkanSampler*>(pGraphicsSampler);

    VkImage image;
    {
      VkImageCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = format.depth ? VK_IMAGE_TYPE_3D : format.height ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D,
        .format = VulkanSystem::GetPixelFormat(format.format),
        .extent =
        {
          .width = (uint32_t)format.width,
          .height = (uint32_t)(format.height > 0 ? format.height : 1),
          .depth = (uint32_t)(format.depth > 0 ? format.depth : 1),
        },
        .mipLevels = (uint32_t)format.mips,
        .arrayLayers = (uint32_t)(format.count ? format.count : 1),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      };
      CheckSuccess(vkCreateImage(_device, &info, nullptr, &image), "creating Vulkan texture image failed");
      AllocateVulkanObject(book, _device, image);
    }

    // get memory requirements
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(_device, image, &memoryRequirements);
    // allocate memory
    std::pair<VkDeviceMemory, VkDeviceSize> memory = AllocateMemory(pool, memoryRequirements, 0);
    // bind memory
    CheckSuccess(vkBindImageMemory(_device, image, memory.first, memory.second), "binding Vulkan memory to texture image failed");

    // create image view
    VkImageView imageView;
    {
      VkImageViewType imageViewType;
      if(format.count)
      {
        if(format.depth) throw Exception("3D texture array is not supported");
        imageViewType = format.height ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_1D_ARRAY;
      }
      else
      {
        imageViewType = format.depth ? VK_IMAGE_VIEW_TYPE_3D : format.height ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;
      }
      VkImageViewCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image,
        .viewType = imageViewType,
        .format = VulkanSystem::GetPixelFormat(format.format),
        .components = {},
        .subresourceRange =
        {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = (uint32_t)format.mips,
          .baseArrayLayer = 0,
          .layerCount = (uint32_t)(format.count ? format.count : 1),
        },
      };
      CheckSuccess(vkCreateImageView(_device, &info, nullptr, &imageView), "creating Vulkan texture image view failed");
      AllocateVulkanObject(book, _device, imageView);
    }

    return book.Allocate<VulkanImage>(image, imageView, pSampler ? pSampler->_sampler : nullptr);
  }

  VulkanSampler& VulkanDevice::CreateSampler(Book& book, GraphicsSamplerConfig const& config)
  {
    struct Helper
    {
      static VkFilter GetFilter(GraphicsSamplerConfig::Filter filter)
      {
        switch(filter)
        {
        case GraphicsSamplerConfig::Filter::Nearest:
          return VK_FILTER_NEAREST;
        case GraphicsSamplerConfig::Filter::Linear:
          return VK_FILTER_LINEAR;
        default:
          throw Exception("unknown Vulkan sampler filter");
        }
      }

      static VkSamplerMipmapMode GetMipmapMode(GraphicsSamplerConfig::Filter filter)
      {
        switch(filter)
        {
        case GraphicsSamplerConfig::Filter::Nearest:
          return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case GraphicsSamplerConfig::Filter::Linear:
          return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        default:
          throw Exception("unknown Vulkan sampler mipmap mode");
        }
      }

      static VkSamplerAddressMode GetAddressMode(GraphicsSamplerConfig::Wrap wrap)
      {
        switch(wrap)
        {
        case GraphicsSamplerConfig::Wrap::Repeat:
          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case GraphicsSamplerConfig::Wrap::RepeatMirror:
          return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case GraphicsSamplerConfig::Wrap::Clamp:
          return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case GraphicsSamplerConfig::Wrap::Border:
          return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default:
          throw Exception("unknown Vulkan sampler address mode");
        }
      }
    };

    VkSamplerCreateInfo info =
    {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .magFilter = Helper::GetFilter(config.magFilter),
      .minFilter = Helper::GetFilter(config.minFilter),
      .mipmapMode = Helper::GetMipmapMode(config.mipFilter),
      .addressModeU = Helper::GetAddressMode(config.wrapU),
      .addressModeV = Helper::GetAddressMode(config.wrapV),
      .addressModeW = Helper::GetAddressMode(config.wrapW),
      .mipLodBias = 0,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 0,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_NEVER,
      .minLod = 0,
      .maxLod = VK_LOD_CLAMP_NONE,
      .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
    };

    VkSampler sampler;
    CheckSuccess(vkCreateSampler(_device, &info, nullptr, &sampler), "creating Vulkan sampler failed");
    AllocateVulkanObject(book, _device, sampler);

    return book.Allocate<VulkanSampler>(sampler);
  }

  std::pair<VkDeviceMemory, VkDeviceSize> VulkanDevice::AllocateMemory(VulkanPool& pool, VkMemoryRequirements const& memoryRequirements, VkMemoryPropertyFlags requireFlags)
  {
    // find suitable memory type
    for(uint32_t memoryType = 0; memoryType < _memoryProperties.memoryTypeCount; ++memoryType)
    {
      if((memoryRequirements.memoryTypeBits & (1 << memoryType)) && (_memoryProperties.memoryTypes[memoryType].propertyFlags & requireFlags) == requireFlags)
      {
        return pool.AllocateMemory(memoryType, memoryRequirements.size, memoryRequirements.alignment);
      }
    }

    throw Exception("no suitable Vulkan memory found");
  }

  VkFence VulkanDevice::CreateFence(Book& book, bool signaled)
  {
    VkFenceCreateInfo info =
    {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VkFlags(signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0),
    };
    VkFence fence;
    CheckSuccess(vkCreateFence(_device, &info, nullptr, &fence), "creating Vulkan fence failed");
    AllocateVulkanObject(book, _device, fence);
    return fence;
  }

  VkSemaphore VulkanDevice::CreateSemaphore(Book& book)
  {
    VkSemaphoreCreateInfo info =
    {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
    };
    VkSemaphore semaphore;
    CheckSuccess(vkCreateSemaphore(_device, &info, nullptr, &semaphore), "creating Vulkan semaphore failed");
    AllocateVulkanObject(book, _device, semaphore);
    return semaphore;
  }

  VulkanContext::VulkanContext(VulkanDevice& device, VulkanPool& pool, Book& book, VkCommandBuffer commandBuffer)
  : _device(device), _pool(pool), _book(book), _commandBuffer(commandBuffer) {}

  uint32_t VulkanContext::GetMaxBufferSize() const
  {
    return _maxBufferSize;
  }

  void VulkanContext::BindVertexBuffer(uint32_t slot, GraphicsVertexBuffer& graphicsVertexBuffer)
  {
    VulkanVertexBuffer& vertexBuffer = static_cast<VulkanVertexBuffer&>(graphicsVertexBuffer);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(_commandBuffer, slot, 1, &vertexBuffer._buffer, &offset);
  }

  void VulkanContext::BindDynamicVertexBuffer(uint32_t slot, Buffer const& buffer)
  {
    // allocate buffer
    auto buf = AllocateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, buffer.size);
    // upload data
    void* data;
    CheckSuccess(vkMapMemory(_device._device, buf.buffer.memory, buf.buffer.memoryOffset + buf.bufferOffset, buffer.size, 0, &data), "mapping Vulkan dynamic vertex buffer memory failed");
    memcpy(data, buffer.data, buffer.size);
    vkUnmapMemory(_device._device, buf.buffer.memory);

    // bind buffer
    vkCmdBindVertexBuffers(_commandBuffer, slot, 1, &buf.buffer.buffer, &buf.bufferOffset);
  }

  void VulkanContext::BindIndexBuffer(GraphicsIndexBuffer* pGraphicsIndexBuffer)
  {
    VulkanIndexBuffer* pIndexBuffer = static_cast<VulkanIndexBuffer*>(pGraphicsIndexBuffer);

    _hasBoundIndexBuffer = !!pIndexBuffer;
    if(_hasBoundIndexBuffer)
    {
      vkCmdBindIndexBuffer(_commandBuffer, pIndexBuffer->_buffer, 0, pIndexBuffer->_is32Bit ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
    }
  }

  void VulkanContext::BindUniformBuffer(GraphicsSlotSetId slotSet, GraphicsSlotId slot, Buffer const& buffer)
  {
    // allocate buffer
    auto buf = AllocateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, buffer.size);
    // upload data
    void* data;
    CheckSuccess(vkMapMemory(_device._device, buf.buffer.memory, buf.buffer.memoryOffset + buf.bufferOffset, buffer.size, 0, &data), "mapping Vulkan uniform buffer memory failed");
    memcpy(data, buffer.data, buffer.size);
    vkUnmapMemory(_device._device, buf.buffer.memory);

    // set buffer to descriptor set
    if(_descriptorSets.size() <= slotSet)
      _descriptorSets.resize(slotSet + 1);
    auto& descriptorSet = _descriptorSets[slotSet];
    descriptorSet.bindings[slot] = BindingUniformBuffer
    {
      .info =
      {
        .buffer = buf.buffer.buffer,
        .offset = buf.bufferOffset,
        .range = buffer.size,
      },
    };
    descriptorSet.descriptorSet = nullptr;
  }

  void VulkanContext::BindImage(GraphicsSlotSetId slotSet, GraphicsSlotId slot, GraphicsImage& graphicsImage)
  {
    VulkanImage& image = static_cast<VulkanImage&>(graphicsImage);

    if(_descriptorSets.size() <= slotSet)
      _descriptorSets.resize(slotSet + 1);
    auto& descriptorSet = _descriptorSets[slotSet];
    descriptorSet.bindings[slot] = BindingImage
    {
      .info =
      {
        .sampler = image._sampler,
        .imageView = image._imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
    };
    descriptorSet.descriptorSet = nullptr;
  }

  void VulkanContext::BindPipeline(GraphicsPipeline& graphicsPipeline)
  {
    VulkanPipeline& pipeline = static_cast<VulkanPipeline&>(graphicsPipeline);

    _pPipeline = &pipeline;
  }

  void VulkanContext::Draw(uint32_t indicesCount, uint32_t instancesCount)
  {
    PrepareDraw();
    if(_hasBoundIndexBuffer)
    {
      vkCmdDrawIndexed(_commandBuffer, indicesCount, instancesCount, 0, 0, 0);
    }
    else
    {
      vkCmdDraw(_commandBuffer, indicesCount, instancesCount, 0, 0);
    }
  }

  void VulkanContext::SetTextureData(GraphicsImage& graphicsImage, GraphicsRawImage const& rawImage)
  {
    VulkanImage& image = static_cast<VulkanImage&>(graphicsImage);

    auto metrics = rawImage.format.GetMetrics();

    std::vector<VkBufferImageCopy> regions;
    int32_t count = rawImage.format.count ? rawImage.format.count : 1;

    uint32_t textureSize = metrics.imageSize * count;
    if(textureSize > rawImage.buffer.size)
      throw Exception("texture data buffer is too small");

    for(int32_t i = 0; i < count; ++i)
    {
      for(size_t j = 0; j < metrics.mips.size(); ++j)
      {
        auto const& mip = metrics.mips[j];
        regions.push_back(
        {
          .bufferOffset = i * metrics.imageSize + mip.offset,
          .bufferRowLength = (uint32_t)mip.width,
          .bufferImageHeight = (uint32_t)mip.height,
          .imageSubresource =
          {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = (uint32_t)j,
            .baseArrayLayer = (uint32_t)i,
            .layerCount = 1,
          },
          .imageOffset =
          {
            .x = 0,
            .y = 0,
            .z = 0,
          },
          .imageExtent =
          {
            .width = (uint32_t)mip.width,
            .height = (uint32_t)mip.height,
            .depth = (uint32_t)mip.depth,
          },
        });
      }
    }

    auto buf = AllocateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, textureSize);
    // upload data to buffer
    {
      void* data;
      CheckSuccess(vkMapMemory(_device._device, buf.buffer.memory, buf.buffer.memoryOffset + buf.bufferOffset, rawImage.buffer.size, 0, &data), "mapping Vulkan texture staging buffer memory failed");
      memcpy(data, rawImage.buffer.data, textureSize);
      vkUnmapMemory(_device._device, buf.buffer.memory);
    }

    // transition image to transfer dst layout
    {
      VkImageMemoryBarrier imageMemoryBarrier =
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image._image,
        .subresourceRange =
        {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = (uint32_t)metrics.mips.size(),
          .baseArrayLayer = 0,
          .layerCount = (uint32_t)count,
        },
      };
      vkCmdPipelineBarrier(_commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // srcStageMask
        VK_PIPELINE_STAGE_TRANSFER_BIT, // dstStageMask
        0, // flags
        0, nullptr, // memory barriers
        0, nullptr, // buffer memory barriers
        1, &imageMemoryBarrier // image memory barriers
      );
    }

    // copy to image
    vkCmdCopyBufferToImage(_commandBuffer, buf.buffer.buffer, image._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());

    // transition to shader read only layout
    {
      VkImageMemoryBarrier imageMemoryBarrier =
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image._image,
        .subresourceRange =
        {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = (uint32_t)metrics.mips.size(),
          .baseArrayLayer = 0,
          .layerCount = (uint32_t)count,
        },
      };
      vkCmdPipelineBarrier(_commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, // srcStageMask
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, // dstStageMask
        0, // flags
        0, nullptr, // memory barriers
        0, nullptr, // buffer memory barriers
        1, &imageMemoryBarrier // image memory barriers
      );
    }
  }

  void VulkanContext::BeginFrame()
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

  void VulkanContext::Reset()
  {
    _pPipeline = nullptr;
    _descriptorSets.clear();
  }

  VkDescriptorSet VulkanContext::AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
  {
    DescriptorSetLayoutCache& cache = _descriptorSetLayoutCaches[descriptorSetLayout];

    // if there's existing descriptor set in cache, use it
    if(cache.nextDescriptorSet < cache.descriptorSets.size())
      return cache.descriptorSets[cache.nextDescriptorSet++];

    // otherwise there's no free descriptor sets, allocate one
    VkDescriptorSet descriptorSet = _pool.AllocateDescriptorSet(descriptorSetLayout);
    cache.descriptorSets.push_back(descriptorSet);
    cache.nextDescriptorSet = cache.descriptorSets.size();
    return descriptorSet;
  }

  VulkanContext::AllocatedBuffer VulkanContext::AllocateBuffer(VkBufferUsageFlagBits usage, uint32_t size)
  {
    BufferCache& cache = _bufferCaches[usage];

    // if size is too big
    if(size > _maxBufferSize)
      throw Exception("too big Vulkan buffer to allocate");

    // if there's existing buffer, but not enough space
    if(cache.nextBuffer < cache.buffers.size() && cache.nextBufferOffset + size > _maxBufferSize)
    {
      // use next buffer
      ++cache.nextBuffer;
      cache.nextBufferOffset = 0;
    }

    // if there's no existing buffer
    if(cache.nextBuffer >= cache.buffers.size())
    {
      // create buffer
      VkBuffer buffer;
      {
        VkBufferCreateInfo info =
        {
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .size = _maxBufferSize,
          .usage = usage,
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          .queueFamilyIndexCount = 0,
          .pQueueFamilyIndices = nullptr,
        };
        CheckSuccess(vkCreateBuffer(_device._device, &info, nullptr, &buffer), "creating Vulkan buffer failed");
        AllocateVulkanObject(_book, _device._device, buffer);
      }

      // get memory requirements
      VkMemoryRequirements memoryRequirements;
      vkGetBufferMemoryRequirements(_device._device, buffer, &memoryRequirements);

      // allocate memory
      std::pair<VkDeviceMemory, VkDeviceSize> memory = _device.AllocateMemory(_pool, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

      // bind memory to buffer
      CheckSuccess(vkBindBufferMemory(_device._device, buffer, memory.first, memory.second), "binding Vulkan memory to buffer failed");

      // put in cache
      cache.buffers.push_back({ buffer, memory.first, memory.second });
      cache.nextBuffer = cache.buffers.size() - 1;
      cache.nextBufferOffset = 0;
    }

    // there's now existing buffer and enough space
    VkDeviceSize offset = cache.nextBufferOffset;
    cache.nextBufferOffset += size;
    return { cache.buffers[cache.nextBuffer], offset };
  }

  void VulkanContext::PrepareDraw()
  {
    if(!_pPipeline) throw Exception("no Vulkan pipeline bound to context");

    bool pipelineLayoutChanged = _pPipeline->_pipelineLayout._pipelineLayout != _pBoundPipelineLayout;

    if(_descriptorSets.size() < _pPipeline->_pipelineLayout._descriptorSetLayouts.size())
      throw Exception("not all Vulkan descriptor sets are bound to context");

    // update descriptor sets if needed
    for(size_t i = 0; i < _pPipeline->_pipelineLayout._descriptorSetLayouts.size(); ++i)
    {
      auto& descriptorSet = _descriptorSets[i];

      // skip if descriptor set is up-to-date and pipeline layout has not changed
      if(descriptorSet.descriptorSet && !pipelineLayoutChanged) continue;

      // allocate new descriptor set
      descriptorSet.descriptorSet = AllocateDescriptorSet(_pPipeline->_pipelineLayout._descriptorSetLayouts[i]);

      // collect info
      for(auto const& bindingIt : descriptorSet.bindings)
      {
        std::visit([&](auto const& binding)
        {
          using T = std::decay_t<decltype(binding)>;
          if constexpr(std::is_same_v<T, BindingUniformBuffer>)
          {
            _bufWriteDescriptorSets.push_back(
            {
              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .pNext = nullptr,
              .dstSet = descriptorSet.descriptorSet,
              .dstBinding = bindingIt.first,
              .dstArrayElement = 0,
              .descriptorCount = 1,
              .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
              .pImageInfo = nullptr,
              .pBufferInfo = &binding.info,
              .pTexelBufferView = nullptr,
            });
          }
          if constexpr(std::is_same_v<T, BindingImage>)
          {
            _bufWriteDescriptorSets.push_back(
            {
              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .pNext = nullptr,
              .dstSet = descriptorSet.descriptorSet,
              .dstBinding = bindingIt.first,
              .dstArrayElement = 0,
              .descriptorCount = 1,
              .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              .pImageInfo = &binding.info,
              .pBufferInfo = nullptr,
              .pTexelBufferView = nullptr,
            });
          }
        }, bindingIt.second);
      }
    }
    // update descriptor sets
    if(!_bufWriteDescriptorSets.empty())
    {
      vkUpdateDescriptorSets(_device._device,
        _bufWriteDescriptorSets.size(),
        _bufWriteDescriptorSets.data(),
        0, nullptr // descriptor copies
      );
      _bufWriteDescriptorSets.clear();
    }

    // bind descriptor sets if needed
    bool needBindDescriptorSets = pipelineLayoutChanged;
    if(!needBindDescriptorSets)
    {
      for(size_t i = 0; i < _descriptorSets.size(); ++i)
      {
        if(!_descriptorSets[i].descriptorSet || _pBoundDescriptorSets.size() <= i || _descriptorSets[i].descriptorSet != _pBoundDescriptorSets[i])
        {
          needBindDescriptorSets = true;
          break;
        }
      }
    }
    if(needBindDescriptorSets)
    {
      _bufDescriptorSets.resize(_descriptorSets.size());
      for(size_t i = 0; i < _descriptorSets.size(); ++i)
        _bufDescriptorSets[i] = _descriptorSets[i].descriptorSet;

      vkCmdBindDescriptorSets(_commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pPipeline->_pipelineLayout._pipelineLayout,
        0, // first set
        _bufDescriptorSets.size(),
        _bufDescriptorSets.data(),
        0, nullptr // dynamic offsets
      );

      _bufDescriptorSets.clear();
      _pBoundPipelineLayout = _pPipeline->_pipelineLayout._pipelineLayout;
    }

    // bind pipeline if needed
    if(_pPipeline->_pipeline != _pBoundPipeline)
    {
      vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pPipeline->_pipeline);
      _pBoundPipeline = _pPipeline->_pipeline;
    }
  }

  VulkanPresenter::VulkanPresenter(
    VulkanDevice& device,
    Book& book,
    VkSurfaceKHR surface,
    VulkanPool& persistentPool,
    std::function<GraphicsRecreatePresentFunc>&& recreatePresent,
    std::function<GraphicsRecreatePresentPerImageFunc>&& recreatePresentPerImage
    ) :
  _device(device),
  _surface(surface),
  _recreatePresent(std::move(recreatePresent)),
  _recreatePresentPerImage(std::move(recreatePresentPerImage))
  {
    // create a few frames
    std::vector<VkCommandBuffer> commandBuffers = VulkanCommandBuffers::Create(book, _device._device, _device._commandPool, _framesCount, true);
    _frames.reserve(_framesCount);
    for(size_t i = 0; i < _framesCount; ++i)
    {
      _frames.emplace_back(book, _device, *this, persistentPool, commandBuffers[i]);
    }
  }

  void VulkanPresenter::Init(std::optional<ivec2> const& size)
  {
    // get surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    CheckSuccess(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device._physicalDevice, _surface, &surfaceCapabilities), "getting Vulkan physical device surface capabilities failed");

    // get surface formats
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    {
      uint32_t surfaceFormatsCount = 0;
      CheckSuccess(vkGetPhysicalDeviceSurfaceFormatsKHR(_device._physicalDevice, _surface, &surfaceFormatsCount, nullptr), "getting Vulkan physical device surface formats failed");
      surfaceFormats.resize(surfaceFormatsCount);
      CheckSuccess(vkGetPhysicalDeviceSurfaceFormatsKHR(_device._physicalDevice, _surface, &surfaceFormatsCount, surfaceFormats.data()), "getting Vulkan physical device surface formats failed");
    }

    // choose surface format
    VkSurfaceFormatKHR const* pSurfaceFormat = nullptr;
    for(size_t i = 0; i < surfaceFormats.size(); ++i)
    {
      auto const& surfaceFormat = surfaceFormats[i];
      if((surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB || surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB) &&
        surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        pSurfaceFormat = &surfaceFormat;
        break;
      }
    }
    if(!pSurfaceFormat) throw Exception("no suitable Vulkan surface format supported");

    _surfaceFormat = pSurfaceFormat->format;

    VkExtent2D extent =
      // use size passed by window if it's available
      size.has_value() ? VkExtent2D
      {
        .width = (uint32_t)size.value().x(),
        .height = (uint32_t)size.value().y(),
      } :
      // on Wayland, initially special values are passed as extent in capabilities
      // replace them with some default size, they will be overriden with next resize call
      surfaceCapabilities.currentExtent.width == 0xFFFFFFFF || surfaceCapabilities.currentExtent.height == 0xFFFFFFFF ? VkExtent2D
      {
        .width = 1024,
        .height = 768,
      } :
      // otherwise, use extent from capabilities
      surfaceCapabilities.currentExtent;

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
        .imageFormat = pSurfaceFormat->format,
        .imageColorSpace = pSurfaceFormat->colorSpace,
        .imageExtent = extent,
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

      CheckSuccess(vkCreateSwapchainKHR(_device._device, &info, nullptr, &_swapchain), "creating Vulkan swapchain failed");
      AllocateVulkanObject(_sizeDependentBook, _device._device, _swapchain);
    }

    // get swapchain images
    {
      uint32_t imagesCount;
      CheckSuccess(vkGetSwapchainImagesKHR(_device._device, _swapchain, &imagesCount, nullptr), "getting Vulkan swapchain images failed");

      std::vector<VkImage> images(imagesCount, nullptr);
      CheckSuccess(vkGetSwapchainImagesKHR(_device._device, _swapchain, &imagesCount, images.data()), "getting Vulkan swapchain images failed");

      for(uint32_t i = 0; i < imagesCount; ++i)
      {
        VkImageViewCreateInfo info =
        {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .image = images[i],
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = _surfaceFormat,
          .components = {},
          .subresourceRange =
          {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
          },
        };
        VkImageView imageView;
        CheckSuccess(vkCreateImageView(_device._device, &info, nullptr, &imageView), "creating Vulkan swapchain image view failed");
        AllocateVulkanObject(_sizeDependentBook, _device._device, imageView);
        _images.push_back(&_sizeDependentBook.Allocate<VulkanImage>(images[i], imageView));
      }
    }


    // recreate resources linked to images
    GraphicsPresentConfig const presentConfig =
    {
      .book = _sizeDependentBook,
      .size = { (int32_t)extent.width, (int32_t)extent.height },
      .pixelFormat = _surfaceFormat,
    };
    _recreatePresent(presentConfig, _images.size());
    for(size_t i = 0; i < _images.size(); ++i)
      _recreatePresentPerImage(presentConfig, i, *_images[i]);

    _sizeDependentBook.Allocate<VulkanDeviceIdle>(_device._device);
  }

  void VulkanPresenter::Clear()
  {
    _sizeDependentBook.Free();
    _swapchain = nullptr;
    _images.clear();
  }

  void VulkanPresenter::Resize(ivec2 const& size)
  {
    Clear();
    Init(size);
  }

  VulkanPool::VulkanPool(VulkanDevice& device, VkDeviceSize chunkSize)
  : _device(device), _chunkSize(chunkSize) {}

  std::pair<VkDeviceMemory, VkDeviceSize> VulkanPool::AllocateMemory(uint32_t memoryTypeIndex, VkDeviceSize size, VkDeviceSize alignment)
  {
    MemoryType& memoryType = _memoryTypes[memoryTypeIndex];

    // if requested size is bigger than chunk, perform dedicated allocation
    if(size > _chunkSize)
    {
      VkMemoryAllocateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = size,
        .memoryTypeIndex = memoryTypeIndex,
      };
      VkDeviceMemory memory;
      CheckSuccess(vkAllocateMemory(_device._device, &info, nullptr, &memory), "allocating Vulkan memory failed");
      AllocateVulkanObject(_book, _device._device, memory);
      return { memory, 0 };
    }

    // align up
    VkDeviceSize offset = (memoryType.offset + alignment - 1) & ~(alignment - 1);

    if(offset + size > memoryType.size)
    {
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
      AllocateVulkanObject(_book, _device._device, memoryType.memory);

      memoryType.size = _chunkSize;
    }

    memoryType.offset = offset + size;
    // round up offset if memory is not host coherent
    if(!(_device._memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
      memoryType.offset = (memoryType.offset + _device._properties.limits.nonCoherentAtomSize - 1) & ~(_device._properties.limits.nonCoherentAtomSize - 1);
    }

    return { memoryType.memory, offset };
  }

  VkDescriptorSet VulkanPool::AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
  {
    auto allocateDescriptorSet = [&](bool requireSuccess) -> VkDescriptorSet
    {
      VkDescriptorSetAllocateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = _descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout,
      };
      VkDescriptorSet descriptorSet;
      VkResult result = vkAllocateDescriptorSets(_device._device, &info, &descriptorSet);
      if((requireSuccess ? CheckSuccess<> : CheckSuccess<VK_ERROR_OUT_OF_POOL_MEMORY>)(result, "allocating Vulkan descriptor sets failed") == VK_SUCCESS)
        return descriptorSet;
      return nullptr;
    };

    // if there's descriptor pool
    if(_descriptorPool)
    {
      // try to allocate set from it
      VkDescriptorSet descriptorSet = allocateDescriptorSet(false);
      if(descriptorSet) return descriptorSet;
    }

    // otherwise there's no descriptor pool, or the allocation failed due to no space in pool
    // create new descriptor pool
    {
      VkDescriptorPoolSize poolSizes[] =
      {
        {
          .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = 4096,
        },
        {
          .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 4096,
        },
      };
      VkDescriptorPoolCreateInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 4096,
        .poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]),
        .pPoolSizes = poolSizes,
      };
      CheckSuccess(vkCreateDescriptorPool(_device._device, &info, nullptr, &_descriptorPool), "creating Vulkan descriptor pool failed");
      AllocateVulkanObject(_book, _device._device, _descriptorPool);
    }

    // allocate
    return allocateDescriptorSet(true);
  }

  VulkanFrame& VulkanPresenter::StartFrame()
  {
    // recreate swapchain if needed
    if(_recreateNeeded)
    {
      _recreateNeeded = false;
      Clear();
      Init({});
    }

    // choose frame
    VulkanFrame& frame = _frames[_nextFrame++];
    _nextFrame %= _framesCount;

    // begin frame
    frame.Begin(_images, _recreateNeeded);

    return frame;
  }

  VulkanFrame::VulkanFrame(Book& book, VulkanDevice& device, VulkanPresenter& presenter, VulkanPool& pool, VkCommandBuffer commandBuffer)
  : _book(book.Allocate<Book>()), // group allocations
    _device(device), _presenter(presenter), _commandBuffer(commandBuffer),
    _fenceFrameFinished(_device.CreateFence(book, true)),
    _semaphoreImageAvailable(_device.CreateSemaphore(book)),
    _semaphoreFrameFinished(_device.CreateSemaphore(book)),
    _context(device, pool, _book, commandBuffer)
  {
    // before freeing anything, wait for last frame to finish
    book.Allocate<VulkanWaitForFence>(_device._device, _fenceFrameFinished);
  }

  void VulkanFrame::Begin(std::vector<VulkanImage*> const& pImages, bool& isSubOptimal)
  {
    // wait until previous use of the frame is done
    CheckSuccess(vkWaitForFences(_device._device, 1, &_fenceFrameFinished, VK_TRUE, std::numeric_limits<uint64_t>::max()), "waiting for Vulkan frame finished failed");

    // reset context caches
    _context.BeginFrame();

    // acquire image
    if(CheckSuccess<VK_SUBOPTIMAL_KHR>(vkAcquireNextImageKHR(_device._device, _presenter._swapchain, std::numeric_limits<uint64_t>::max(), _semaphoreImageAvailable, nullptr, &_imageIndex), "acquiring Vulkan next image failed") == VK_SUBOPTIMAL_KHR)
      isSubOptimal = true;
    _pImage = pImages[_imageIndex];

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

  uint32_t VulkanFrame::GetImageIndex() const
  {
    return _imageIndex;
  }

  VulkanContext& VulkanFrame::GetContext()
  {
    return _context;
  }

  void VulkanFrame::Pass(GraphicsPass& graphicsPass, GraphicsFramebuffer& graphicsFramebuffer, std::function<void(GraphicsSubPassId, GraphicsContext&)> const& func)
  {
    VulkanPass& pass = static_cast<VulkanPass&>(graphicsPass);
    VulkanFramebuffer& framebuffer = static_cast<VulkanFramebuffer&>(graphicsFramebuffer);

    // start render pass
    {
      VkRenderPassBeginInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = pass._renderPass,
        .framebuffer = framebuffer._framebuffer,
        .renderArea =
        {
          .offset = { 0, 0 },
          .extent = { (uint32_t)framebuffer._size.x(), (uint32_t)framebuffer._size.y() },
        },
        .clearValueCount = (uint32_t)pass._clearValues.size(),
        .pClearValues = pass._clearValues.data(),
      };
      vkCmdBeginRenderPass(_commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // perform subpasses
    for(uint32_t i = 0; i < pass._subPassesCount; ++i)
    {
      if(i > 0)
        vkCmdNextSubpass(_commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
      _context.Reset();
      func(i, _context);
    }

    // end render pass
    vkCmdEndRenderPass(_commandBuffer);
  }

  void VulkanFrame::EndFrame()
  {
    // transition frame image to proper present layout
    {
      VkImageMemoryBarrier imageMemoryBarrier =
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = _pImage->_image,
        .subresourceRange =
        {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      };
      vkCmdPipelineBarrier(_commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dstStageMask
        0, // flags
        0, nullptr, // memory barriers
        0, nullptr, // buffer memory barriers
        1, &imageMemoryBarrier // image memory barriers
      );
    }

    // end command buffer
    CheckSuccess(vkEndCommandBuffer(_commandBuffer), "ending Vulkan command buffer failed");

    // reset fence
    CheckSuccess(vkResetFences(_device._device, 1, &_fenceFrameFinished), "resetting Vulkan fence failed");
    // queue command buffer
    {
      VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      VkSubmitInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &_semaphoreImageAvailable,
        .pWaitDstStageMask = &waitDstStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &_commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &_semaphoreFrameFinished,
      };
      CheckSuccess(vkQueueSubmit(_device._graphicsQueue, 1, &info, _fenceFrameFinished), "queueing Vulkan command buffer failed");
    }

    // present
    {
      VkPresentInfoKHR info =
      {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &_semaphoreFrameFinished,
        .swapchainCount = 1,
        .pSwapchains = &_presenter._swapchain,
        .pImageIndices = &_imageIndex,
        .pResults = nullptr,
      };
      CheckSuccess<VK_SUBOPTIMAL_KHR>(vkQueuePresentKHR(_device._graphicsQueue, &info), "queueing Vulkan present failed");
    }
  }

  VulkanPass::VulkanPass(VkRenderPass renderPass, std::vector<VkClearValue>&& clearValues, uint32_t subPassesCount)
  : _renderPass(renderPass), _clearValues(std::move(clearValues)), _subPassesCount(subPassesCount) {}

  VulkanDeviceIdle::VulkanDeviceIdle(VkDevice device)
  : _device(device) {}

  VulkanDeviceIdle::~VulkanDeviceIdle()
  {
    CheckSuccess(vkDeviceWaitIdle(_device), "waiting for Vulkan device idle failed\n");
  }

  VulkanWaitForFence::VulkanWaitForFence(VkDevice device, VkFence fence)
  : _device(device), _fence(fence) {}

  VulkanWaitForFence::~VulkanWaitForFence()
  {
    CheckSuccess(vkWaitForFences(_device, 1, &_fence, VK_TRUE, std::numeric_limits<uint64_t>::max()), "waiting for Vulkan fence failed");
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

  VulkanVertexBuffer::VulkanVertexBuffer(VkBuffer buffer)
  : _buffer(buffer) {}

  VulkanIndexBuffer::VulkanIndexBuffer(VkBuffer buffer, bool is32Bit)
  : _buffer(buffer), _is32Bit(is32Bit) {}

  VulkanImage::VulkanImage(VkImage image, VkImageView imageView, VkSampler sampler)
  : _image(image), _imageView(imageView), _sampler(sampler) {}

  VulkanSampler::VulkanSampler(VkSampler sampler)
  : _sampler(sampler) {}

  VulkanShader::VulkanShader(VkShaderModule shaderModule, VkShaderStageFlags stagesMask, std::vector<SpirvDescriptorSetLayout>&& descriptorSetLayouts)
  : _shaderModule(shaderModule), _stagesMask(stagesMask), _descriptorSetLayouts(std::move(descriptorSetLayouts)) {}

  VulkanPipelineLayout::VulkanPipelineLayout(VkPipelineLayout pipelineLayout, std::vector<VkDescriptorSetLayout>&& descriptorSetLayouts)
  : _pipelineLayout(pipelineLayout), _descriptorSetLayouts(std::move(descriptorSetLayouts)) {}

  VulkanPipeline::VulkanPipeline(VkPipeline pipeline, VulkanPipelineLayout& pipelineLayout)
  : _pipeline(pipeline), _pipelineLayout(pipelineLayout) {}

  VulkanFramebuffer::VulkanFramebuffer(VkFramebuffer framebuffer, ivec2 const& size)
  : _framebuffer(framebuffer), _size(size) {}
}
