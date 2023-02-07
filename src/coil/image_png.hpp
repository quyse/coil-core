#pragma once

#include "graphics.hpp"

namespace Coil
{
  void LoadPngImage(InputStream& stream, GraphicsImageFormat& format, std::vector<uint8_t>& data);
  void SavePngImage(OutputStream& stream, GraphicsImageFormat const& format, Buffer const& buffer);
}
