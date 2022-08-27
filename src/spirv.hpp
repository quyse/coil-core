#pragma once

#include "graphics.hpp"

namespace Coil
{
  using SpirvCode = std::vector<uint32_t>;

  enum class SpirvDescriptorType
  {
    Unused,
    UniformBuffer,
  };

  enum class SpirvStageFlag
  {
    Vertex = 1,
    Fragment = 2,
  };

  struct SpirvDescriptorSetLayoutBinding
  {
    SpirvDescriptorType descriptorType = SpirvDescriptorType::Unused;
    uint32_t descriptorCount = 0;
    uint32_t stageFlags = 0;
  };

  struct SpirvDescriptorSetLayout
  {
    std::vector<SpirvDescriptorSetLayoutBinding> bindings;
  };

  struct SpirvModule
  {
    SpirvCode code;
    std::vector<SpirvDescriptorSetLayout> descriptorSetLayouts;
  };

  SpirvModule SpirvCompile(GraphicsShaderRoots const& roots);
}
