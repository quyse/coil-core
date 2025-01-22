module;

#include <vulkan/vulkan.h>
#include <tuple>

export module coil.core.vulkan:utils;

import coil.core.base;

namespace Coil
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

  void DestroyVulkanObject(VkDevice device)                                            { vkDestroyDevice(device, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkBuffer buffer)                           { vkDestroyBuffer(device, buffer, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkCommandPool commandPool)                 { vkDestroyCommandPool(device, commandPool, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkDescriptorPool descriptorPool)           { vkDestroyDescriptorPool(device, descriptorPool, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkDescriptorSetLayout descriptorSetLayout) { vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkDeviceMemory memory)                     { vkFreeMemory(device, memory, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkFence fence)                             { vkDestroyFence(device, fence, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkFramebuffer framebuffer)                 { vkDestroyFramebuffer(device, framebuffer, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkImage image)                             { vkDestroyImage(device, image, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkImageView imageView)                     { vkDestroyImageView(device, imageView, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkPipeline pipeline)                       { vkDestroyPipeline(device, pipeline, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkPipelineLayout pipelineLayout)           { vkDestroyPipelineLayout(device, pipelineLayout, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkRenderPass renderPass)                   { vkDestroyRenderPass(device, renderPass, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkSampler sampler)                         { vkDestroySampler(device, sampler, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkSemaphore semaphore)                     { vkDestroySemaphore(device, semaphore, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkShaderModule shaderModule)               { vkDestroyShaderModule(device, shaderModule, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkSwapchainKHR swapchain)                  { vkDestroySwapchainKHR(device, swapchain, nullptr); }
  void DestroyVulkanObject(VkInstance instance)                                        { vkDestroyInstance(instance, nullptr); }
  void DestroyVulkanObject(VkInstance instance, VkSurfaceKHR surface)                  { vkDestroySurfaceKHR(instance, surface, nullptr); }

  template <typename... Args>
  class VulkanObject
  {
  public:
    VulkanObject(Args... args)
    : _args(args...) {}
    ~VulkanObject()
    {
      std::apply((void(&)(Args...))DestroyVulkanObject, _args);
    }

  private:
    std::tuple<Args...> _args;
  };

  template <typename... Args>
  void AllocateVulkanObject(Book& book, Args... args)
  {
    book.Allocate<VulkanObject<Args...>>(args...);
  }
}
