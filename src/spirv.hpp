#pragma once

#include "graphics.hpp"

namespace Coil
{
  using SpirvCode = std::vector<uint32_t>;

  SpirvCode SpirvCompile(GraphicsShaderRoots const& roots);
}
