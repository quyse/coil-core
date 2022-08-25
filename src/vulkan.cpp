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

    VulkanDevice& deviceObj = book.Allocate<VulkanDevice>(_instance, physicalDevice, device, graphicsQueueFamilyIndex);
    deviceObj.Init(book);
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

  std::vector<VulkanSystem::InstanceExtensionsHandler> VulkanSystem::_instanceExtensionsHandlers;
  std::vector<VulkanSystem::DeviceSurfaceHandler> VulkanSystem::_deviceSurfaceHandlers;

  VulkanDevice::VulkanDevice(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamilyIndex)
  : _instance(instance), _physicalDevice(physicalDevice), _device(device), _graphicsQueueFamilyIndex(graphicsQueueFamilyIndex)
  {
    vkGetDeviceQueue(_device, _graphicsQueueFamilyIndex, 0, &_graphicsQueue);
  }

  void VulkanDevice::Init(Book& book)
  {
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

  VulkanPool& VulkanDevice::CreatePool(Book& book, uint64_t chunkSize)
  {
    return book.Allocate<VulkanPool>(*this, chunkSize);
  }

  VulkanPresenter& VulkanDevice::CreateWindowPresenter(Book& book, Window& window, std::function<GraphicsRecreatePresentPassFunc>&& recreatePresentPass, std::function<GraphicsRecreatePresentFrameFunc>&& recreatePresentFrame)
  {
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
    VulkanPresenter& presenter = book.Allocate<VulkanPresenter>(*this, book.Allocate<Book>(), surface, std::move(recreatePresentPass), std::move(recreatePresentFrame));

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
      AllocateVulkanObject(pool.GetBook(), _device, vertexBuffer);
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
    for(AttachmentId attachmentId = 0; attachmentId < attachmentsCount; ++attachmentId)
    {
      GraphicsPassConfig::Attachment const& attachment = config.attachments[attachmentId];
      attachmentsDescriptions[attachmentId] =
      {
        .flags = 0,
        .format = VulkanSystem::GetPixelFormat(attachment.format),
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
    return book.Allocate<VulkanPass>(renderPass, subPassesCount);
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

      VkShaderStageFlags stagesMask = 0;
      if(roots.vertex) stagesMask |= VK_SHADER_STAGE_VERTEX_BIT;
      if(roots.fragment) stagesMask |= VK_SHADER_STAGE_FRAGMENT_BIT;

      AllocateVulkanObject(book, _device, shaderModule);
      return book.Allocate<VulkanShader>(shaderModule, stagesMask);
    }
  }

  VulkanPipelineLayout& VulkanDevice::CreatePipelineLayout(Book& book)
  {
    // just empty layout for now
    VkPipelineLayoutCreateInfo info =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 0,
      .pSetLayouts = nullptr,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
    };
    VkPipelineLayout pipelineLayout;
    CheckSuccess(vkCreatePipelineLayout(_device, &info, nullptr, &pipelineLayout), "creating Vulkan pipeline layout failed");
    AllocateVulkanObject(book, _device, pipelineLayout);
    return book.Allocate<VulkanPipelineLayout>(pipelineLayout);
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
    for(size_t i = 0; i < config.vertexSlots.size(); ++i)
    {
      GraphicsPipelineConfig::VertexSlot const& slot = config.vertexSlots[i];
      vertexBindingDescriptions.push_back(
      {
        .binding = (uint32_t)i,
        .stride = slot.stride,
        .inputRate = slot.perInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX,
      });
    }

    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
    for(size_t i = 0; i < config.vertexAttributes.size(); ++i)
    {
      GraphicsPipelineConfig::VertexAttribute const& attribute = config.vertexAttributes[i];
      vertexAttributeDescriptions.push_back(
      {
        .location = attribute.location,
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

    VkPipelineViewportStateCreateInfo viewportStateInfo =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .viewportCount = 1,
      .pViewports = nullptr, // dynamic
      .scissorCount = 1,
      .pScissors = nullptr, // dynamic
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
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .depthBiasConstantFactor = 0,
      .depthBiasClamp = 0,
      .depthBiasSlopeFactor = 0,
      .lineWidth = 1,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
    {
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_CLEAR,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachmentState,
      .blendConstants = { 0 },
    };

    VkDynamicState dynamicStates[] =
    {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicStateInfo =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]),
      .pDynamicStates = dynamicStates,
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
      .pDepthStencilState = nullptr,
      .pColorBlendState = &colorBlendStateInfo,
      .pDynamicState = &dynamicStateInfo,
      .layout = pipelineLayout._pipelineLayout,
      .renderPass = pass._renderPass,
      .subpass = 0,
      .basePipelineHandle = nullptr,
      .basePipelineIndex = -1,
    };

    VkPipeline pipeline;
    CheckSuccess(vkCreateGraphicsPipelines(_device, nullptr, 1, &info, nullptr, &pipeline), "creating Vulkan graphics pipeline failed");
    AllocateVulkanObject(book, _device, pipeline);
    return book.Allocate<VulkanPipeline>(pipeline);
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
      .width = (uint32_t)size.x,
      .height = (uint32_t)size.y,
      .layers = 1,
    };
    VkFramebuffer framebuffer;
    CheckSuccess(vkCreateFramebuffer(_device,  &info, nullptr, &framebuffer), "creating Vulkan framebuffer failed");
    AllocateVulkanObject(book, _device, framebuffer);
    return book.Allocate<VulkanFramebuffer>(framebuffer, size);
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

  VulkanContext::VulkanContext(VkCommandBuffer commandBuffer)
  : _commandBuffer(commandBuffer) {}

  void VulkanContext::BindVertexBuffer(GraphicsVertexBuffer& graphicsVertexBuffer)
  {
    VulkanVertexBuffer& vertexBuffer = static_cast<VulkanVertexBuffer&>(graphicsVertexBuffer);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(_commandBuffer, 0, 1, &vertexBuffer._buffer, &offset);
  }

  void VulkanContext::BindPipeline(GraphicsPipeline& graphicsPipeline)
  {
    VulkanPipeline& pipeline = static_cast<VulkanPipeline&>(graphicsPipeline);

    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);
  }

  void VulkanContext::Draw(uint32_t verticesCount)
  {
    vkCmdDraw(_commandBuffer, verticesCount, 1, 0, 0);
  }

  VulkanPresenter::VulkanPresenter(
    VulkanDevice& device,
    Book& book,
    VkSurfaceKHR surface,
    std::function<GraphicsRecreatePresentPassFunc>&& recreatePresentPass,
    std::function<GraphicsRecreatePresentFrameFunc>&& recreatePresentFrame
    ) :
  _device(device),
  _book(book),
  _surface(surface),
  _recreatePresentPass(std::move(recreatePresentPass)),
  _recreatePresentFrame(std::move(recreatePresentFrame))
  {}

  void VulkanPresenter::Init()
  {
    // get surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    CheckSuccess(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device._physicalDevice, _surface, &surfaceCapabilities), "getting Vulkan physical device surface capabilities failed");

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

      CheckSuccess(vkCreateSwapchainKHR(_device._device, &info, nullptr, &_swapchain), "creating Vulkan swapchain failed");
      AllocateVulkanObject(_book, _device._device, _swapchain);
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
          .format = VK_FORMAT_R8G8B8A8_SRGB,
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
        CheckSuccess(vkCreateImageView(_device._device, &info, nullptr, &imageView), "creating Vulkan image view failed");
        AllocateVulkanObject(_book, _device._device, imageView);
        _images.push_back(&_book.Allocate<VulkanImage>(images[i], imageView));
      }
    }

    // create a few frames
    std::vector<VkCommandBuffer> commandBuffers = VulkanCommandBuffers::Create(_book, _device._device, _device._commandPool, _framesCount, true);
    _frames.reserve(_framesCount);
    for(size_t i = 0; i < _framesCount; ++i)
    {
      _frames.emplace_back(_device, *this);
      _frames[i].Init(_book, commandBuffers[i]);
    }

    // recreate resources linked to images
    ivec2 size = { (int32_t)extent.width, (int32_t)extent.height };
    _recreatePresentPass(_book, size, _images.size());
    for(size_t i = 0; i < _images.size(); ++i)
      _recreatePresentFrame(_book, size, i, *_images[i]);

    _book.Allocate<VulkanDeviceIdle>(_device._device);
  }

  void VulkanPresenter::Clear()
  {
    _book.Free();
    _swapchain = nullptr;
    _images.clear();
    _frames.clear();
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

  VulkanFrame::VulkanFrame(VulkanDevice& device, VulkanPresenter& presenter)
  : _device(device), _presenter(presenter) {}

  void VulkanFrame::Init(Book& book, VkCommandBuffer commandBuffer)
  {
    _commandBuffer = commandBuffer;
    _fenceFrameFinished = _device.CreateFence(book, true);
    _semaphoreImageAvailable = _device.CreateSemaphore(book);
    _semaphoreFrameFinished = _device.CreateSemaphore(book);
  }

  void VulkanFrame::Begin(std::vector<VulkanImage*> const& pImages, bool& isSubOptimal)
  {
    // wait until previous use of the frame is done
    CheckSuccess(vkWaitForFences(_device._device, 1, &_fenceFrameFinished, VK_TRUE, std::numeric_limits<uint64_t>::max()), "waiting for Vulkan frame finished failed");
    // reset fence
    CheckSuccess(vkResetFences(_device._device, 1, &_fenceFrameFinished), "resetting Vulkan fence failed");

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

  void VulkanFrame::Pass(GraphicsPass& graphicsPass, GraphicsFramebuffer& graphicsFramebuffer, std::function<void(GraphicsSubPassId, GraphicsContext&)> const& func)
  {
    VulkanPass& pass = static_cast<VulkanPass&>(graphicsPass);
    VulkanFramebuffer& framebuffer = static_cast<VulkanFramebuffer&>(graphicsFramebuffer);

    // start render pass
    {
      VkClearValue clearValues[] =
      {
        { .color = { .float32 = { 0, 0, 0, 0 } } },
      };
      VkRenderPassBeginInfo info =
      {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = pass._renderPass,
        .framebuffer = framebuffer._framebuffer,
        .renderArea =
        {
          .offset = { 0, 0 },
          .extent = { (uint32_t)framebuffer._size.x, (uint32_t)framebuffer._size.y },
        },
        .clearValueCount = 1,
        .pClearValues = clearValues,
      };
      vkCmdBeginRenderPass(_commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // set viewport & scissor
    {
      VkViewport viewport =
      {
        .x = 0,
        .y = 0,
        .width = (float)framebuffer._size.x,
        .height = (float)framebuffer._size.y,
        .minDepth = 0,
        .maxDepth = 1,
      };
      vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
      VkRect2D scissor =
      {
        .offset = { 0, 0 },
        .extent = { (uint32_t)framebuffer._size.x, (uint32_t)framebuffer._size.y },
      };
      vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
    }

    // perform subpasses
    VulkanContext context(_commandBuffer);
    for(uint32_t i = 0; i < pass._subPassesCount; ++i)
    {
      if(i > 0)
        vkCmdNextSubpass(_commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
      func(i, context);
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

  VulkanPass::VulkanPass(VkRenderPass renderPass, uint32_t subPassesCount)
  : _renderPass(renderPass), _subPassesCount(subPassesCount) {}

  VulkanDeviceIdle::VulkanDeviceIdle(VkDevice device)
  : _device(device) {}

  VulkanDeviceIdle::~VulkanDeviceIdle()
  {
    CheckSuccess(vkDeviceWaitIdle(_device), "waiting for Vulkan device idle failed\n");
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

  VulkanImage::VulkanImage(VkImage image, VkImageView imageView)
  : _image(image), _imageView(imageView) {}

  VulkanShader::VulkanShader(VkShaderModule shaderModule, VkShaderStageFlags stagesMask)
  : _shaderModule(shaderModule), _stagesMask(stagesMask) {}

  VulkanPipelineLayout::VulkanPipelineLayout(VkPipelineLayout pipelineLayout)
  : _pipelineLayout(pipelineLayout) {}

  VulkanPipeline::VulkanPipeline(VkPipeline pipeline)
  : _pipeline(pipeline) {}

  VulkanFramebuffer::VulkanFramebuffer(VkFramebuffer framebuffer, ivec2 const& size)
  : _framebuffer(framebuffer), _size(size) {}
}
