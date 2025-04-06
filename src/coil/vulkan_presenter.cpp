module;

#include <vulkan/vulkan.h>
#include <functional>
#include <optional>

module coil.core.vulkan;

import :utils;

namespace Coil
{
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
    {
      VkFormat const desiredFormats[] =
      {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM,
        VK_FORMAT_R8G8B8_UNORM,
      };
      for(size_t i = 0; !pSurfaceFormat && i < sizeof(desiredFormats) / sizeof(desiredFormats[0]); ++i)
      {
        VkFormat const desiredFormat = desiredFormats[i];

        for(size_t j = 0; !pSurfaceFormat && j < surfaceFormats.size(); ++j)
        {
          auto const& surfaceFormat = surfaceFormats[j];

          if(surfaceFormat.format == desiredFormat && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
          {
            pSurfaceFormat = &surfaceFormat;
          }
        }
      }
    }
    if(!pSurfaceFormat) throw Exception("no suitable Vulkan surface format supported");

    _surfaceFormat = pSurfaceFormat->format;

    VkExtent2D extent =
      // use size passed by window if it's available
      size.has_value() ? VkExtent2D
      {
        .width = std::min(std::max((uint32_t)size.value().x(), surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width),
        .height = std::min(std::max((uint32_t)size.value().y(), surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height),
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
      VkSwapchainCreateInfoKHR const info =
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
        VkImageViewCreateInfo const info =
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
      .book = _sizeDependentBook.Allocate<Book>(),
      .size = { (int32_t)extent.width, (int32_t)extent.height },
      .pixelFormat = (GraphicsOpaquePixelFormat)_surfaceFormat,
    };
    _recreatePresent(presentConfig, _images.size());
    for(size_t i = 0; i < _images.size(); ++i)
      _recreatePresentPerImage(presentConfig, i, *_images[i]);

    _sizeDependentBook.Allocate<VulkanDeviceIdle>(_device._device);
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
}
