module;

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <vector>

export module coil.core.wayland.vulkan;

import coil.core.graphics;
import coil.core.vulkan;
import coil.core.wayland;

export namespace Coil
{
  class WaylandVulkanSystem
  {
  public:
    static void Init()
    {
      VulkanSystem::RegisterInstanceExtensionsHandler([](GraphicsWindow& window, std::vector<char const*>& extensions)
      {
        WaylandWindow* waylandWindow = dynamic_cast<WaylandWindow*>(&window);
        if(!waylandWindow) return;

        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
      });

      VulkanSystem::RegisterDeviceSurfaceHandler([](VkInstance instance, GraphicsWindow& window) -> VkSurfaceKHR
      {
        WaylandWindow* waylandWindow = dynamic_cast<WaylandWindow*>(&window);
        if(!waylandWindow) return nullptr;

        VkSurfaceKHR surface;
        VkWaylandSurfaceCreateInfoKHR const info =
        {
          .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
          .pNext = nullptr,
          .flags = 0,
          .display = waylandWindow->GetSystem().GetDisplay(),
          .surface = waylandWindow->GetSurface(),
        };
        if(vkCreateWaylandSurfaceKHR(instance, &info, nullptr, &surface) != VK_SUCCESS)
          return nullptr;

        return surface;
      });
    }
  };
}
