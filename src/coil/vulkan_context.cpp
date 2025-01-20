module;

#include <vulkan/vulkan.h>
#include <concepts>
#include <cstring>
#include <vector>

module coil.core.vulkan;

namespace Coil
{
  void VulkanContext::BindVertexBuffer(uint32_t slot, GraphicsVertexBuffer& graphicsVertexBuffer)
  {
    VulkanVertexBuffer& vertexBuffer = static_cast<VulkanVertexBuffer&>(graphicsVertexBuffer);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(_commandBuffer, slot, 1, &vertexBuffer._buffer, &offset);
  }

  void VulkanContext::BindDynamicVertexBuffer(uint32_t slot, Buffer const& buffer)
  {
    // allocate buffer
    auto buf = AllocateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, buffer.size);
    // upload data
    void* data;
    CheckSuccess(vkMapMemory(_device._device, buf.buffer.memory, buf.buffer.memoryOffset + buf.bufferOffset, buffer.size, 0, &data), "mapping Vulkan dynamic vertex buffer memory failed");
    memcpy(data, buffer.data, buffer.size);
    vkUnmapMemory(_device._device, buf.buffer.memory);

    // bind buffer
    vkCmdBindVertexBuffers(_commandBuffer, slot, 1, &buf.buffer.buffer, &buf.bufferOffset);
  }

  void VulkanContext::BindIndexBuffer(GraphicsIndexBuffer* pGraphicsIndexBuffer)
  {
    VulkanIndexBuffer* pIndexBuffer = static_cast<VulkanIndexBuffer*>(pGraphicsIndexBuffer);

    _hasBoundIndexBuffer = !!pIndexBuffer;
    if(_hasBoundIndexBuffer)
    {
      vkCmdBindIndexBuffer(_commandBuffer, pIndexBuffer->_buffer, 0, pIndexBuffer->_is32Bit ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
    }
  }

  void VulkanContext::BindUniformBuffer(GraphicsSlotSetId slotSet, GraphicsSlotId slot, Buffer const& buffer)
  {
    // allocate buffer
    auto buf = AllocateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, buffer.size, _device._properties.limits.minUniformBufferOffsetAlignment);
    // upload data
    void* data;
    CheckSuccess(vkMapMemory(_device._device, buf.buffer.memory, buf.buffer.memoryOffset + buf.bufferOffset, buffer.size, 0, &data), "mapping Vulkan uniform buffer memory failed");
    memcpy(data, buffer.data, buffer.size);
    vkUnmapMemory(_device._device, buf.buffer.memory);

    // set buffer to descriptor set
    if(_descriptorSets.size() <= slotSet)
      _descriptorSets.resize(slotSet + 1);
    auto& descriptorSet = _descriptorSets[slotSet];
    descriptorSet.bindings[slot] = BindingUniformBuffer
    {
      .info =
      {
        .buffer = buf.buffer.buffer,
        .offset = buf.bufferOffset,
        .range = buffer.size,
      },
    };
    descriptorSet.descriptorSet = nullptr;
  }

  void VulkanContext::BindStorageBuffer(GraphicsSlotSetId slotSet, GraphicsSlotId slot, GraphicsStorageBuffer& graphicsStorageBuffer)
  {
    VulkanStorageBuffer& storageBuffer = static_cast<VulkanStorageBuffer&>(graphicsStorageBuffer);

    if(_descriptorSets.size() <= slotSet)
      _descriptorSets.resize(slotSet + 1);
    auto& descriptorSet = _descriptorSets[slotSet];
    descriptorSet.bindings[slot] = BindingStorageBuffer
    {
      .info =
      {
        .buffer = storageBuffer._buffer,
        .offset = 0,
        .range = storageBuffer._size,
      },
    };
    descriptorSet.descriptorSet = nullptr;
  }

  void VulkanContext::BindImage(GraphicsSlotSetId slotSet, GraphicsSlotId slot, GraphicsImage& graphicsImage)
  {
    VulkanImage& image = static_cast<VulkanImage&>(graphicsImage);

    if(_descriptorSets.size() <= slotSet)
      _descriptorSets.resize(slotSet + 1);
    auto& descriptorSet = _descriptorSets[slotSet];
    descriptorSet.bindings[slot] = BindingImage
    {
      .info =
      {
        .sampler = image._sampler,
        .imageView = image._imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
    };
    descriptorSet.descriptorSet = nullptr;
  }

  void VulkanContext::BindPipeline(GraphicsPipeline& graphicsPipeline)
  {
    VulkanPipeline& pipeline = static_cast<VulkanPipeline&>(graphicsPipeline);

    _pPipeline = &pipeline;
  }

  void VulkanContext::Draw(uint32_t indicesCount, uint32_t instancesCount)
  {
    Prepare();
    if(_hasBoundIndexBuffer)
    {
      vkCmdDrawIndexed(_commandBuffer, indicesCount, instancesCount, 0, 0, 0);
    }
    else
    {
      vkCmdDraw(_commandBuffer, indicesCount, instancesCount, 0, 0);
    }
  }

  void VulkanContext::Dispatch(ivec3 size)
  {
    Prepare();
    vkCmdDispatch(_commandBuffer, size.x(), size.y(), size.z());
  }

  void VulkanContext::SetTextureData(GraphicsImage& graphicsImage, ImageBuffer const& imageBuffer)
  {
    VulkanImage& image = static_cast<VulkanImage&>(graphicsImage);

    auto metrics = imageBuffer.format.GetMetrics();

    std::vector<VkBufferImageCopy> regions;
    int32_t count = imageBuffer.format.count ? imageBuffer.format.count : 1;

    uint32_t textureSize = metrics.imageSize * count;
    if(textureSize > imageBuffer.buffer.size)
      throw Exception("texture data buffer is too small");

    auto buf = AllocateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, textureSize);

    for(int32_t imageIndex = 0; imageIndex < count; ++imageIndex)
    {
      for(size_t mipIndex = 0; mipIndex < metrics.mips.size(); ++mipIndex)
      {
        auto const& mip = metrics.mips[mipIndex];
        regions.push_back(
        {
          .bufferOffset = buf.bufferOffset + imageIndex * metrics.imageSize + mip.offset,
          .bufferRowLength = (uint32_t)mip.bufferWidth,
          .bufferImageHeight = (uint32_t)mip.bufferHeight,
          .imageSubresource =
          {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = (uint32_t)mipIndex,
            .baseArrayLayer = (uint32_t)imageIndex,
            .layerCount = 1,
          },
          .imageOffset =
          {
            .x = 0,
            .y = 0,
            .z = 0,
          },
          .imageExtent =
          {
            .width = (uint32_t)mip.width,
            .height = (uint32_t)mip.height,
            .depth = (uint32_t)mip.depth,
          },
        });
      }
    }

    // upload data to buffer
    {
      void* data;
      CheckSuccess(vkMapMemory(_device._device, buf.buffer.memory, buf.buffer.memoryOffset + buf.bufferOffset, imageBuffer.buffer.size, 0, &data), "mapping Vulkan texture staging buffer memory failed");
      memcpy(data, imageBuffer.buffer.data, textureSize);
      vkUnmapMemory(_device._device, buf.buffer.memory);
    }

    // transition image to transfer dst layout
    {
      VkImageMemoryBarrier const imageMemoryBarrier =
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image._image,
        .subresourceRange =
        {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = (uint32_t)metrics.mips.size(),
          .baseArrayLayer = 0,
          .layerCount = (uint32_t)count,
        },
      };
      vkCmdPipelineBarrier(_commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, // srcStageMask
        VK_PIPELINE_STAGE_TRANSFER_BIT, // dstStageMask
        0, // flags
        0, nullptr, // memory barriers
        0, nullptr, // buffer memory barriers
        1, &imageMemoryBarrier // image memory barriers
      );
    }

    // copy to image
    vkCmdCopyBufferToImage(_commandBuffer, buf.buffer.buffer, image._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());

    // transition to shader read only layout
    {
      VkImageMemoryBarrier const imageMemoryBarrier =
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image._image,
        .subresourceRange =
        {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = (uint32_t)metrics.mips.size(),
          .baseArrayLayer = 0,
          .layerCount = (uint32_t)count,
        },
      };
      vkCmdPipelineBarrier(_commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, // srcStageMask
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, // dstStageMask
        0, // flags
        0, nullptr, // memory barriers
        0, nullptr, // buffer memory barriers
        1, &imageMemoryBarrier // image memory barriers
      );
    }
  }

  VkDescriptorSet VulkanContext::AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
  {
    DescriptorSetLayoutCache& cache = _descriptorSetLayoutCaches[descriptorSetLayout];

    // if there's existing descriptor set in cache, use it
    if(cache.nextDescriptorSet < cache.descriptorSets.size())
      return cache.descriptorSets[cache.nextDescriptorSet++];

    // otherwise there's no free descriptor sets, allocate one
    VkDescriptorSet descriptorSet = _pool.AllocateDescriptorSet(descriptorSetLayout);
    cache.descriptorSets.push_back(descriptorSet);
    cache.nextDescriptorSet = cache.descriptorSets.size();
    return descriptorSet;
  }

  VulkanContext::AllocatedBuffer VulkanContext::AllocateBuffer(VkBufferUsageFlagBits usage, uint32_t size, uint32_t alignment)
  {
    BufferCache& cache = _bufferCaches[usage];

    // if size is too big
    if(size > _maxBufferSize)
      throw Exception("too big Vulkan buffer to allocate");

    // if there's existing buffer, but not enough space
    if(cache.nextBuffer < cache.buffers.size() && ((cache.nextBufferOffset + alignment - 1) & ~(alignment - 1)) + size > _maxBufferSize)
    {
      // use next buffer
      ++cache.nextBuffer;
      cache.nextBufferOffset = 0;
    }

    // if there's no existing buffer
    if(cache.nextBuffer >= cache.buffers.size())
    {
      // create buffer
      VkBuffer buffer;
      {
        VkBufferCreateInfo const info =
        {
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .size = _maxBufferSize,
          .usage = (VkBufferUsageFlags)usage,
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          .queueFamilyIndexCount = 0,
          .pQueueFamilyIndices = nullptr,
        };
        CheckSuccess(vkCreateBuffer(_device._device, &info, nullptr, &buffer), "creating Vulkan buffer failed");
        AllocateVulkanObject(_book, _device._device, buffer);
      }

      // get memory requirements
      VkMemoryRequirements memoryRequirements;
      vkGetBufferMemoryRequirements(_device._device, buffer, &memoryRequirements);

      // allocate memory
      std::pair<VkDeviceMemory, VkDeviceSize> memory = _device.AllocateMemory(_pool, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

      // bind memory to buffer
      CheckSuccess(vkBindBufferMemory(_device._device, buffer, memory.first, memory.second), "binding Vulkan memory to buffer failed");

      // put in cache
      cache.buffers.push_back({ buffer, memory.first, memory.second });
      cache.nextBuffer = cache.buffers.size() - 1;
      cache.nextBufferOffset = 0;
    }

    // there's now existing buffer and enough space
    VkDeviceSize offset = (cache.nextBufferOffset + alignment - 1) & ~(alignment - 1);
    cache.nextBufferOffset = offset + size;
    return { cache.buffers[cache.nextBuffer], offset };
  }

  void VulkanContext::Prepare()
  {
    if(!_pPipeline) throw Exception("no Vulkan pipeline bound to context");

    bool pipelineLayoutChanged = _pPipeline->_pipelineLayout._pipelineLayout != _pBoundPipelineLayout;

    if(_descriptorSets.size() < _pPipeline->_pipelineLayout._descriptorSetLayouts.size())
      throw Exception("not all Vulkan descriptor sets are bound to context");

    // update descriptor sets if needed
    for(size_t i = 0; i < _pPipeline->_pipelineLayout._descriptorSetLayouts.size(); ++i)
    {
      auto& descriptorSet = _descriptorSets[i];

      // skip if descriptor set is up-to-date and pipeline layout has not changed
      if(descriptorSet.descriptorSet && !pipelineLayoutChanged) continue;

      // allocate new descriptor set
      descriptorSet.descriptorSet = AllocateDescriptorSet(_pPipeline->_pipelineLayout._descriptorSetLayouts[i]);

      // collect info
      for(auto const& bindingIt : descriptorSet.bindings)
      {
        std::visit([&]<typename Binding>(Binding const& binding)
        {
          if constexpr(std::same_as<Binding, BindingUniformBuffer>)
          {
            _bufWriteDescriptorSets.push_back(
            {
              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .pNext = nullptr,
              .dstSet = descriptorSet.descriptorSet,
              .dstBinding = bindingIt.first,
              .dstArrayElement = 0,
              .descriptorCount = 1,
              .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
              .pImageInfo = nullptr,
              .pBufferInfo = &binding.info,
              .pTexelBufferView = nullptr,
            });
          }
          if constexpr(std::same_as<Binding, BindingStorageBuffer>)
          {
            _bufWriteDescriptorSets.push_back(
            {
              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .pNext = nullptr,
              .dstSet = descriptorSet.descriptorSet,
              .dstBinding = bindingIt.first,
              .dstArrayElement = 0,
              .descriptorCount = 1,
              .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
              .pImageInfo = nullptr,
              .pBufferInfo = &binding.info,
              .pTexelBufferView = nullptr,
            });
          }
          if constexpr(std::same_as<Binding, BindingImage>)
          {
            _bufWriteDescriptorSets.push_back(
            {
              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .pNext = nullptr,
              .dstSet = descriptorSet.descriptorSet,
              .dstBinding = bindingIt.first,
              .dstArrayElement = 0,
              .descriptorCount = 1,
              .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              .pImageInfo = &binding.info,
              .pBufferInfo = nullptr,
              .pTexelBufferView = nullptr,
            });
          }
        }, bindingIt.second);
      }
    }
    // update descriptor sets
    if(!_bufWriteDescriptorSets.empty())
    {
      vkUpdateDescriptorSets(_device._device,
        _bufWriteDescriptorSets.size(),
        _bufWriteDescriptorSets.data(),
        0, nullptr // descriptor copies
      );
      _bufWriteDescriptorSets.clear();
    }

    // bind descriptor sets if needed
    bool needBindDescriptorSets = pipelineLayoutChanged;
    if(!needBindDescriptorSets)
    {
      for(size_t i = 0; i < _descriptorSets.size(); ++i)
      {
        if(!_descriptorSets[i].descriptorSet || _pBoundDescriptorSets.size() <= i || _descriptorSets[i].descriptorSet != _pBoundDescriptorSets[i])
        {
          needBindDescriptorSets = true;
          break;
        }
      }
    }
    if(needBindDescriptorSets)
    {
      _bufDescriptorSets.resize(_descriptorSets.size());
      for(size_t i = 0; i < _descriptorSets.size(); ++i)
        _bufDescriptorSets[i] = _descriptorSets[i].descriptorSet;

      if(!_bufDescriptorSets.empty())
      {
        vkCmdBindDescriptorSets(_commandBuffer,
          _pPipeline->_bindPoint,
          _pPipeline->_pipelineLayout._pipelineLayout,
          0, // first set
          _bufDescriptorSets.size(),
          _bufDescriptorSets.data(),
          0, nullptr // dynamic offsets
        );
      }

      _bufDescriptorSets.clear();
      _pBoundPipelineLayout = _pPipeline->_pipelineLayout._pipelineLayout;
    }

    // bind pipeline if needed
    if(_pPipeline->_pipeline != _pBoundPipeline)
    {
      vkCmdBindPipeline(_commandBuffer, _pPipeline->_bindPoint, _pPipeline->_pipeline);
      _pBoundPipeline = _pPipeline->_pipeline;
    }
  }
}
