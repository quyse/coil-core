#pragma once

#include "graphics.hpp"

namespace Coil
{
  ImageBuffer LoadPngImage(Book& book, InputStream& stream);
  void SavePngImage(OutputStream& stream, ImageBuffer const& imageBuffer);
}
