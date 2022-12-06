#pragma once

#include "graphics.hpp"

namespace Coil
{
  void LoadPngImage(Buffer const& file, GraphicsImageFormat& format, std::vector<uint8_t>& data);
}
