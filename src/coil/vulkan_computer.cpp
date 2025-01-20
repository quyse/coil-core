module;

#include <vulkan/vulkan.h>
#include <functional>

module coil.core.vulkan;

namespace Coil
{
  void VulkanComputer::Compute(std::function<void(GraphicsContext&)> const& func)
  {
    // reset context caches
    _context.BeginFrame();

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

    // send commands
    func(_context);

    // global memory barrier for reading results on host
    {
      VkMemoryBarrier const memoryBarrier =
      {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
      };
      vkCmdPipelineBarrier(_commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // srcStageMask
        VK_PIPELINE_STAGE_HOST_BIT, // dstStageMask
        0, // flags
        1, &memoryBarrier, // memory barriers
        0, nullptr, // buffer memory barriers
        0, nullptr // image memory barriers
      );
    }

    // end command buffer
    CheckSuccess(vkEndCommandBuffer(_commandBuffer), "ending Vulkan command buffer failed");

    // reset fence
    CheckSuccess(vkResetFences(_device._device, 1, &_fenceComputeFinished), "resetting Vulkan fence failed");
    // queue command buffer
    {
      VkSubmitInfo const info =
      {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &_commandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
      };
      CheckSuccess(vkQueueSubmit(_device._graphicsQueue, 1, &info, _fenceComputeFinished), "queueing Vulkan command buffer failed");
    }

    // wait until compute is done
    CheckSuccess(vkWaitForFences(_device._device, 1, &_fenceComputeFinished, VK_TRUE, std::numeric_limits<uint64_t>::max()), "waiting for Vulkan compute finished failed");
  }
}
