module;

#include <vulkan/vulkan.h>
#include <vector>

module coil.core.vulkan;

import coil.core.appidentity;

namespace Coil
{
  VulkanSystem& VulkanSystem::Create(Book& book, GraphicsWindow* window, GraphicsCapabilities const& capabilities)
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
      if(window)
      {
        for(auto const& handler : _instanceExtensionsHandlers)
          handler(*window, extensions);
      }
      else if(capabilities.render)
      {
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
      }

      VkApplicationInfo const appInfo =
      {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = AppIdentity::GetInstance().Name().c_str(),
        .applicationVersion = AppIdentity::GetInstance().Version(),
        .pEngineName = "Coil Core",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_0,
      };

      VkInstanceCreateInfo const info =
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

    return book.Allocate<VulkanSystem>(instance, capabilities);
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
      VkDeviceQueueCreateInfo const queueInfo =
      {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
      };

      std::vector<char const*> enabledExtensionNames;
      if(_capabilities.render)
        enabledExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
      if(_capabilities.compute)
        enabledExtensionNames.push_back(VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME);
      if(enablePortabilitySubsetExtension)
        enabledExtensionNames.push_back("VK_KHR_portability_subset");

      VkPhysicalDeviceFeatures const enabledFeatures =
      {
        .tessellationShader = _capabilities.tessellation,
      };
      VkDeviceCreateInfo const info =
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
        .pEnabledFeatures = &enabledFeatures,
      };

      CheckSuccess(vkCreateDevice(physicalDevice, &info, nullptr, &device), "creating Vulkan device failed");
      AllocateVulkanObject(book, device);
    }

    VulkanDevice& vulkanDevice = book.Allocate<VulkanDevice>(book.Allocate<Book>(), _instance, physicalDevice, device, graphicsQueueFamilyIndex);
    book.Allocate<VulkanDeviceIdle>(device);
    return vulkanDevice;
  }
}
