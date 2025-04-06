module;

#include <vulkan/vulkan.h>
#include <algorithm>
#include <functional>
#include <span>
#include <variant>

module coil.core.vulkan;

namespace Coil
{
  VulkanDevice::VulkanDevice(Book& book, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamilyIndex)
  : _book(book), _instance(instance), _physicalDevice(physicalDevice), _device(device), _graphicsQueueFamilyIndex(graphicsQueueFamilyIndex)
  {
    vkGetDeviceQueue(_device, _graphicsQueueFamilyIndex, 0, &_graphicsQueue);

    // query physical device properties
    vkGetPhysicalDeviceProperties(_physicalDevice, &_properties);

    // create command pool
    {
      VkCommandPoolCreateInfo const info =
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

    // get suitable depth format
    {
      VkFormat const formats[] =
      {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
      };
      for(size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i)
      {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(_physicalDevice, formats[i], &formatProperties);
        if(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
          _depthFormat = formats[i];
          break;
        }
      }
    }
  }

  VulkanPresenter& VulkanDevice::CreateWindowPresenter(Book& book, GraphicsPool& graphicsPool, GraphicsWindow& window, std::function<GraphicsRecreatePresentFunc>&& recreatePresent, std::function<GraphicsRecreatePresentPerImageFunc>&& recreatePresentPerImage)
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

  VulkanComputer& VulkanDevice::CreateComputer(Book& book, GraphicsPool& graphicsPool)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);

    VkCommandBuffer commandBuffer = VulkanCommandBuffers::Create(book, _device, _commandPool, 1, true)[0];
    VkFence fenceComputeFinished = CreateFence(book, true);
    return book.Allocate<VulkanComputer>(book.Allocate<Book>(), *this, pool, commandBuffer, fenceComputeFinished);
  }

  VulkanVertexBuffer& VulkanDevice::CreateVertexBuffer(Book& book, GraphicsPool& graphicsPool, Buffer const& buffer)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);

    VkBuffer vertexBuffer = std::get<VkBuffer>(CreateBuffer(book, pool, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, buffer));

    return book.Allocate<VulkanVertexBuffer>(vertexBuffer);
  }

  VulkanIndexBuffer& VulkanDevice::CreateIndexBuffer(Book& book, GraphicsPool& graphicsPool, Buffer const& buffer, bool is32Bit)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);

    VkBuffer indexBuffer = std::get<VkBuffer>(CreateBuffer(book, pool, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, buffer));

    return book.Allocate<VulkanIndexBuffer>(indexBuffer, is32Bit);
  }

  VulkanStorageBuffer& VulkanDevice::CreateStorageBuffer(Book& book, GraphicsPool& graphicsPool, Buffer const& buffer)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);

    auto [storageBuffer, memory, offset, size] = CreateBuffer(book, pool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer);

    return book.Allocate<VulkanStorageBuffer>(storageBuffer, memory, offset, size);
  }

  VulkanImage& VulkanDevice::CreateRenderImage(Book& book, GraphicsPool& graphicsPool, PixelFormat const& pixelFormat, ivec2 const& size, GraphicsSampler* pGraphicsSampler)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);
    VulkanSampler* pSampler = static_cast<VulkanSampler*>(pGraphicsSampler);
    VkFormat format = VulkanSystem::GetPixelFormat(pixelFormat);

    // create image
    VkImage image;
    {
      VkImageCreateInfo const info =
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
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      };
      CheckSuccess(vkCreateImage(_device, &info, nullptr, &image), "creating Vulkan render image failed");
      AllocateVulkanObject(book, _device, image);
    }

    // get memory requirements
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(_device, image, &memoryRequirements);
    // allocate memory
    std::pair<VkDeviceMemory, VkDeviceSize> memory = AllocateMemory(pool, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    // bind memory
    CheckSuccess(vkBindImageMemory(_device, image, memory.first, memory.second), "binding Vulkan memory to render image failed");

    // create image view
    VkImageView imageView;
    {
      VkImageViewCreateInfo const info =
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
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      };
      CheckSuccess(vkCreateImageView(_device, &info, nullptr, &imageView), "creating Vulkan render image view failed");
      AllocateVulkanObject(book, _device, imageView);
    }

    return book.Allocate<VulkanImage>(image, imageView, pSampler ? pSampler->_sampler : nullptr);
  }

  VulkanImage& VulkanDevice::CreateDepthImage(Book& book, GraphicsPool& graphicsPool, ivec2 const& size)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);

    // create image
    VkImage image;
    {
      VkImageCreateInfo const info =
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = _depthFormat,
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
    std::pair<VkDeviceMemory, VkDeviceSize> memory = AllocateMemory(pool, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    // bind memory
    CheckSuccess(vkBindImageMemory(_device, image, memory.first, memory.second), "binding Vulkan memory to depth stencil image failed");

    // create image view
    VkImageView imageView;
    {
      VkImageViewCreateInfo const info =
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = _depthFormat,
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
        std::visit([&]<typename Attachment>(Attachment const& attachment)
        {
          // color attachment
          if constexpr(std::same_as<Attachment, GraphicsPassConfig::SubPass::ColorAttachment>)
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
          if constexpr(std::same_as<Attachment, GraphicsPassConfig::SubPass::DepthStencilAttachment>)
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
          if constexpr(std::same_as<Attachment, GraphicsPassConfig::SubPass::InputAttachment>)
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
          if constexpr(std::same_as<Attachment, GraphicsPassConfig::SubPass::ShaderAttachment>)
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
      for(int32_t dependencySubPassId = (int32_t)subPassId - 1; dependencySubPassId >= 0; --dependencySubPassId)
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
      std::visit([&]<typename Config>(Config const& config)
      {
        if constexpr(std::same_as<Config, GraphicsPassConfig::ColorAttachmentConfig>)
        {
          format = std::visit([&]<typename Format>(Format const& attachmentPixelFormat) -> VkFormat
          {
            if constexpr(std::same_as<Format, PixelFormat>)
              return VulkanSystem::GetPixelFormat(attachmentPixelFormat);
            if constexpr(std::same_as<Format, GraphicsOpaquePixelFormat>)
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
        if constexpr(std::same_as<Config, GraphicsPassConfig::DepthStencilAttachmentConfig>)
        {
          format = _depthFormat;
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

    VkRenderPassCreateInfo const info =
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
      VkShaderModuleCreateInfo const info =
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
      if(roots.compute) stagesMask |= VK_SHADER_STAGE_COMPUTE_BIT;

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
          case SpirvDescriptorType::StorageBuffer:
            descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
          case SpirvDescriptorType::SampledImage:
            descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            break;
          default:
            throw Exception("unsupported Vulkan descriptor type from SPIR-V module");
          }

          VkShaderStageFlags stageFlags = 0;
          if(mergedBinding.stageFlags & (uint32_t)SpirvStageFlag::Vertex) stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
          if(mergedBinding.stageFlags & (uint32_t)SpirvStageFlag::TessellationControl) stageFlags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
          if(mergedBinding.stageFlags & (uint32_t)SpirvStageFlag::TessellationEvaluation) stageFlags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
          if(mergedBinding.stageFlags & (uint32_t)SpirvStageFlag::Fragment) stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
          if(mergedBinding.stageFlags & (uint32_t)SpirvStageFlag::Compute) stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

          bindings.push_back(
          {
            .binding = (uint32_t)j,
            .descriptorType = descriptorType,
            .descriptorCount = mergedBinding.descriptorCount,
            .stageFlags = stageFlags,
            .pImmutableSamplers = nullptr,
          });
        }

        VkDescriptorSetLayoutCreateInfo const info =
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

    VkPipelineLayoutCreateInfo const info =
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

    VkPipelineVertexInputStateCreateInfo const vertexInputStateInfo =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount = (uint32_t)vertexBindingDescriptions.size(),
      .pVertexBindingDescriptions = vertexBindingDescriptions.data(),
      .vertexAttributeDescriptionCount = (uint32_t)vertexAttributeDescriptions.size(),
      .pVertexAttributeDescriptions = vertexAttributeDescriptions.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo const inputAssemblyState =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport const viewport =
    {
      .x = 0,
      .y = 0,
      .width = (float)config.viewport.x(),
      .height = (float)config.viewport.y(),
      .minDepth = 0,
      .maxDepth = 1,
    };
    VkRect2D const scissor =
    {
      .offset = { 0, 0 },
      .extent = { (uint32_t)config.viewport.x(), (uint32_t)config.viewport.y() },
    };
    VkPipelineViewportStateCreateInfo const viewportStateInfo =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo const rasterizationState =
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

    VkPipelineMultisampleStateCreateInfo const multisampleState =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineDepthStencilStateCreateInfo const depthStencilState =
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
      .maxDepthBounds = 1,
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

    VkPipelineColorBlendStateCreateInfo const colorBlendStateInfo =
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

    VkGraphicsPipelineCreateInfo const info =
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
      .pMultisampleState = &multisampleState,
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
    return book.Allocate<VulkanPipeline>(pipeline, pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS);
  }

  VulkanPipeline& VulkanDevice::CreatePipeline(Book& book, GraphicsPipelineLayout& graphicsPipelineLayout, GraphicsShader& graphicsShader)
  {
    VulkanPipelineLayout& pipelineLayout = static_cast<VulkanPipelineLayout&>(graphicsPipelineLayout);
    VulkanShader& shader = static_cast<VulkanShader&>(graphicsShader);

    VkComputePipelineCreateInfo const info =
    {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage =
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader._shaderModule,
        .pName = "mainCompute",
        .pSpecializationInfo = nullptr,
      },
      .layout = pipelineLayout._pipelineLayout,
      .basePipelineHandle = nullptr,
      .basePipelineIndex = -1,
    };

    VkPipeline pipeline;
    CheckSuccess(vkCreateComputePipelines(_device, nullptr, 1, &info, nullptr, &pipeline), "creating Vulkan compute pipeline failed");
    AllocateVulkanObject(book, _device, pipeline);
    return book.Allocate<VulkanPipeline>(pipeline, pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE);
  }

  VulkanFramebuffer& VulkanDevice::CreateFramebuffer(Book& book, GraphicsPass& graphicsPass, std::span<GraphicsImage*> const& pImages, ivec2 const& size)
  {
    VulkanPass& pass = static_cast<VulkanPass&>(graphicsPass);

    std::vector<VkImageView> attachments;
    attachments.reserve(pImages.size());
    for(auto const& pImage : pImages)
      attachments.push_back(static_cast<VulkanImage&>(*pImage)._imageView);

    VkFramebufferCreateInfo const info =
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

  VulkanImage& VulkanDevice::CreateTexture(Book& book, GraphicsPool& graphicsPool, ImageFormat const& format, GraphicsSampler* pGraphicsSampler)
  {
    VulkanPool& pool = static_cast<VulkanPool&>(graphicsPool);
    VulkanSampler* pSampler = static_cast<VulkanSampler*>(pGraphicsSampler);

    VkImage image;
    {
      VkImageCreateInfo const info =
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
    std::pair<VkDeviceMemory, VkDeviceSize> memory = AllocateMemory(pool, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
      VkImageViewCreateInfo const info =
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

    VkSamplerCreateInfo const info =
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

  void VulkanDevice::SetStorageBufferData(GraphicsStorageBuffer& graphicsStorageBuffer, Buffer const& buffer)
  {
    VulkanStorageBuffer& storageBuffer = static_cast<VulkanStorageBuffer&>(graphicsStorageBuffer);

    if(buffer.size > storageBuffer._size)
      throw Exception("overflow while setting storage buffer data");

    // map memory
    void* data;
    CheckSuccess(vkMapMemory(_device, storageBuffer._memory, storageBuffer._offset, storageBuffer._size, 0, &data), "mapping Vulkan memory failed");

    // copy data
    memcpy(data, buffer.data, buffer.size);

    // unmap memory
    vkUnmapMemory(_device, storageBuffer._memory);
  }

  void VulkanDevice::GetStorageBufferData(GraphicsStorageBuffer& graphicsStorageBuffer, Buffer const& buffer)
  {
    VulkanStorageBuffer& storageBuffer = static_cast<VulkanStorageBuffer&>(graphicsStorageBuffer);

    // map memory
    void* data;
    CheckSuccess(vkMapMemory(_device, storageBuffer._memory, storageBuffer._offset, storageBuffer._size, 0, &data), "mapping Vulkan memory failed");

    // copy data
    memcpy(buffer.data, data, std::min(buffer.size, storageBuffer._size));

    // unmap memory
    vkUnmapMemory(_device, storageBuffer._memory);
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

  std::tuple<VkBuffer, VkDeviceMemory, VkDeviceSize, VkDeviceSize> VulkanDevice::CreateBuffer(Book& book, VulkanPool& pool, VkBufferUsageFlags usage, Buffer const& initialData)
  {
    // create buffer
    VkBuffer buffer;
    {
      VkBufferCreateInfo const info =
      {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = initialData.size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
      };
      CheckSuccess(vkCreateBuffer(_device, &info, nullptr, &buffer), "creating Vulkan buffer failed");
      AllocateVulkanObject(book, _device, buffer);
    }

    // get memory requirements
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(_device, buffer, &memoryRequirements);

    // allocate memory
    // temporary, for simplicity we use host coherent memory
    // Vulkan spec guarantees that buffer's memory requirements allow such memory, and it exists on device
    std::pair<VkDeviceMemory, VkDeviceSize> memory = AllocateMemory(pool, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // copy data

    if(initialData.data)
    {
      // map memory
      void* data;
      CheckSuccess(vkMapMemory(_device, memory.first, memory.second, memoryRequirements.size, 0, &data), "mapping Vulkan memory failed");

      // copy data
      memcpy(data, initialData.data, initialData.size);

      // unmap memory
      vkUnmapMemory(_device, memory.first);
    }

    // bind it to buffer
    CheckSuccess(vkBindBufferMemory(_device, buffer, memory.first, memory.second), "binding Vulkan memory to buffer failed");

    return { buffer, memory.first, memory.second, memoryRequirements.size };
  }

  VkFence VulkanDevice::CreateFence(Book& book, bool signaled)
  {
    VkFenceCreateInfo const info =
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
    VkSemaphoreCreateInfo const info =
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
}
