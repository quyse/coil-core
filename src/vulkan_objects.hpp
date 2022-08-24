#pragma once

namespace Coil
{
  void DestroyVulkanObject(VkDevice device)                                  { vkDestroyDevice(device, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkBuffer buffer)                 { vkDestroyBuffer(device, buffer, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkCommandPool commandPool)       { vkDestroyCommandPool(device, commandPool, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkDeviceMemory memory)           { vkFreeMemory(device, memory, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkFence fence)                   { vkDestroyFence(device, fence, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkFramebuffer framebuffer)       { vkDestroyFramebuffer(device, framebuffer, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkImageView imageView)           { vkDestroyImageView(device, imageView, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkPipeline pipeline)             { vkDestroyPipeline(device, pipeline, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkPipelineLayout pipelineLayout) { vkDestroyPipelineLayout(device, pipelineLayout, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkRenderPass renderPass)         { vkDestroyRenderPass(device, renderPass, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkSemaphore semaphore)           { vkDestroySemaphore(device, semaphore, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkShaderModule shaderModule)     { vkDestroyShaderModule(device, shaderModule, nullptr); }
  void DestroyVulkanObject(VkDevice device, VkSwapchainKHR swapchain)        { vkDestroySwapchainKHR(device, swapchain, nullptr); }
  void DestroyVulkanObject(VkInstance instance)                              { vkDestroyInstance(instance, nullptr); }
  void DestroyVulkanObject(VkInstance instance, VkSurfaceKHR surface)        { vkDestroySurfaceKHR(instance, surface, nullptr); }

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
  void AllocateVulkanObject(Coil::Book& book, Args... args)
  {
    book.Allocate<VulkanObject<Args...>>(args...);
  }
}
