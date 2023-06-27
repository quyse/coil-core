#pragma once

#include "graphics.hpp"

namespace Coil
{
  using SpirvCode = std::vector<uint32_t>;

  enum class SpirvDescriptorType
  {
    Unused,
    UniformBuffer,
    StorageBuffer,
    SampledImage,
  };

  enum class SpirvStageFlag
  {
    Vertex = 1,
    TessellationControl = 2,
    TessellationEvaluation = 4,
    Fragment = 8,
    Compute = 16,
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
