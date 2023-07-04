#pragma once

#include "image_format.hpp"

namespace Coil
{
  ImageBuffer CompressImage(Book& book, ImageBuffer const& imageBuffer, PixelFormat::Compression compression);
}
