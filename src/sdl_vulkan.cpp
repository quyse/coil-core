#include "sdl_vulkan.hpp"
#include "vulkan.hpp"
#include "sdl.hpp"
#include <SDL2/SDL_vulkan.h>

namespace Coil
{
  void SdlVulkanSystem::Init()
  {
    static Sdl sdl(SDL_INIT_VIDEO);
    // MSYS shit
#if defined(__MINGW32__)
    static int _libraryLoaded = SDL_Vulkan_LoadLibrary("libvulkan-1.dll");
#endif

    VulkanSystem::RegisterInstanceExtensionsHandler([](Window& window, std::vector<char const*>& extensions)
    {
      SdlWindow* sdlWindow = dynamic_cast<SdlWindow*>(&window);
      if(!sdlWindow) return;

      // get necessary extensions from window

      // get extensions count
      unsigned int count = 0;
      if(!SDL_Vulkan_GetInstanceExtensions(sdlWindow->GetSdlWindow(), &count, nullptr))
        throw Exception("getting Vulkan instance extensions for SDL failed");

      size_t initialCount = extensions.size();
      extensions.resize(initialCount + count);

      // get extensions
      if(!SDL_Vulkan_GetInstanceExtensions(sdlWindow->GetSdlWindow(), &count, extensions.data() + initialCount))
        throw Exception("getting Vulkan instance extensions for SDL failed");
      // theoretically returned count could become less, so resize again
      extensions.resize(initialCount + count);
    });

    VulkanSystem::RegisterDeviceSurfaceHandler([](VkInstance instance, Window& window) -> VkSurfaceKHR
    {
      SdlWindow* sdlWindow = dynamic_cast<SdlWindow*>(&window);
      if(!sdlWindow) return nullptr;

      VkSurfaceKHR surface;
      if(!SDL_Vulkan_CreateSurface(sdlWindow->GetSdlWindow(), instance, &surface))
        throw Exception("creating Vulkan surface for SDL failed");

      return surface;
    });
  }
}
