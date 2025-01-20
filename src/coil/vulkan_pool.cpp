module;

#include <vulkan/vulkan.h>
#include <utility>

module coil.core.vulkan;

namespace Coil
{
  std::pair<VkDeviceMemory, VkDeviceSize> VulkanPool::AllocateMemory(uint32_t memoryTypeIndex, VkDeviceSize size, VkDeviceSize alignment)
  {
    MemoryType& memoryType = _memoryTypes[memoryTypeIndex];

    // if requested size is bigger than chunk, perform dedicated allocation
    if(size > _chunkSize)
    {
      VkMemoryAllocateInfo const info =
      {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = size,
        .memoryTypeIndex = memoryTypeIndex,
      };
      VkDeviceMemory memory;
      CheckSuccess(vkAllocateMemory(_device._device, &info, nullptr, &memory), "allocating Vulkan memory failed");
      AllocateVulkanObject(_book, _device._device, memory);
      return { memory, 0 };
    }

    // align up
    VkDeviceSize offset = (memoryType.offset + alignment - 1) & ~(alignment - 1);

    if(offset + size > memoryType.size)
    {
      memoryType = {};
      offset = 0;

      VkMemoryAllocateInfo const info =
      {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = _chunkSize,
        .memoryTypeIndex = memoryTypeIndex,
      };
      CheckSuccess(vkAllocateMemory(_device._device, &info, nullptr, &memoryType.memory), "allocating Vulkan memory failed");
      AllocateVulkanObject(_book, _device._device, memoryType.memory);

      memoryType.size = _chunkSize;
    }

    memoryType.offset = offset + size;
    // round up offset if memory is not host coherent
    if(!(_device._memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
      memoryType.offset = (memoryType.offset + _device._properties.limits.nonCoherentAtomSize - 1) & ~(_device._properties.limits.nonCoherentAtomSize - 1);
    }

    return { memoryType.memory, offset };
  }

  VkDescriptorSet VulkanPool::AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
  {
    auto allocateDescriptorSet = [&](bool requireSuccess) -> VkDescriptorSet
    {
      VkDescriptorSetAllocateInfo const info =
      {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = _descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout,
      };
      VkDescriptorSet descriptorSet;
      VkResult result = vkAllocateDescriptorSets(_device._device, &info, &descriptorSet);
      if((requireSuccess ? CheckSuccess<> : CheckSuccess<VK_ERROR_OUT_OF_POOL_MEMORY>)(result, "allocating Vulkan descriptor sets failed") == VK_SUCCESS)
        return descriptorSet;
      return nullptr;
    };

    // if there's descriptor pool
    if(_descriptorPool)
    {
      // try to allocate set from it
      VkDescriptorSet descriptorSet = allocateDescriptorSet(false);
      if(descriptorSet) return descriptorSet;
    }

    // otherwise there's no descriptor pool, or the allocation failed due to no space in pool
    // create new descriptor pool
    {
      VkDescriptorPoolSize const poolSizes[] =
      {
        {
          .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = 4096,
        },
        {
          .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 4096,
        },
        {
          .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 4096,
        },
      };
      VkDescriptorPoolCreateInfo const info =
      {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 4096,
        .poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]),
        .pPoolSizes = poolSizes,
      };
      CheckSuccess(vkCreateDescriptorPool(_device._device, &info, nullptr, &_descriptorPool), "creating Vulkan descriptor pool failed");
      AllocateVulkanObject(_book, _device._device, _descriptorPool);
    }

    // allocate
    return allocateDescriptorSet(true);
  }
}
