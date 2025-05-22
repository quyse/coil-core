module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>

export module coil.core.sdl.vulkan;

import coil.core.base;
import coil.core.graphics;
import coil.core.sdl;
import coil.core.vulkan;

export namespace Coil
{
  class SdlVulkanSystem
  {
  public:
    static void Init()
    {
      static Sdl sdl{SDL_INIT_VIDEO};
      // MSYS shit
#if defined(__MINGW32__)
      static int _libraryLoaded = SDL_Vulkan_LoadLibrary("libvulkan-1.dll");
#endif

      VulkanSystem::RegisterInstanceExtensionsHandler([](GraphicsWindow& window, std::vector<char const*>& extensions)
      {
        SdlWindow* sdlWindow = dynamic_cast<SdlWindow*>(&window);
        if(!sdlWindow) return;

        // get necessary extensions from window

        // get extensions count
        uint32_t count = 0;
        char const* const* pExtensions = SDL_Vulkan_GetInstanceExtensions(&count);
        if(!pExtensions)
          throw Exception("getting Vulkan instance extensions for SDL failed");

        size_t initialCount = extensions.size();
        extensions.resize(initialCount + count);

        for(uint32_t i = 0; i < count; ++i)
          extensions[initialCount + i] = pExtensions[i];
      });

      VulkanSystem::RegisterDeviceSurfaceHandler([](VkInstance instance, GraphicsWindow& window) -> VkSurfaceKHR
      {
        SdlWindow* sdlWindow = dynamic_cast<SdlWindow*>(&window);
        if(!sdlWindow) return nullptr;

        VkSurfaceKHR surface;
        if(!SDL_Vulkan_CreateSurface(sdlWindow->GetSdlWindow(), instance, nullptr, &surface))
          throw Exception("creating Vulkan surface for SDL failed");

        return surface;
      });
    }
  };
}
