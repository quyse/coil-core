module;

#include <vulkan/vulkan.h>
#include <functional>

module coil.core.vulkan;

namespace Coil
{
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

  void VulkanFrame::Pass(GraphicsPass& graphicsPass, GraphicsFramebuffer& graphicsFramebuffer, std::function<void(GraphicsSubPassId, GraphicsContext&)> const& func)
  {
    VulkanPass& pass = static_cast<VulkanPass&>(graphicsPass);
    VulkanFramebuffer& framebuffer = static_cast<VulkanFramebuffer&>(graphicsFramebuffer);

    // start render pass
    {
      VkRenderPassBeginInfo const info =
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
      VkImageMemoryBarrier const imageMemoryBarrier =
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
      VkSubmitInfo const info =
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
      VkPresentInfoKHR const info =
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
      VkCommandBufferBeginInfo const info =
      {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
      };
      CheckSuccess(vkBeginCommandBuffer(_commandBuffer, &info), "beginning Vulkan command buffer failed");
    }
  }
}
