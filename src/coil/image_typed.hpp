#pragma once

#include "image.hpp"
#include "image_format.hpp"

namespace Coil
{
  template <typename F>
  bool PixelFormat::ApplyTyped(F const& f) const
  {
    if(type != Type::Uncompressed) return false;

    switch(components)
    {
    case Components::R:
      switch(format)
      {
      case Format::Uint:
        switch(size)
        {
        case Size::_8bit:
          f.template operator()<uint8_t>();
          return true;
        case Size::_16bit:
          f.template operator()<uint16_t>();
          return true;
        case Size::_32bit:
          f.template operator()<uint32_t>();
          return true;
        case Size::_64bit:
          f.template operator()<uint64_t>();
          return true;
        default:
          return false;
        }
      case Format::Float:
        switch(size)
        {
        case Size::_32bit:
          f.template operator()<float>();
          return true;
        case Size::_64bit:
          f.template operator()<double>();
          return true;
        default:
          return false;
        }
      default:
        return false;
      }
    case Components::RG:
      switch(format)
      {
      case Format::Uint:
        switch(size)
        {
        case Size::_16bit:
          f.template operator()<xvec_ua<uint8_t, 2>>();
          return true;
        case Size::_32bit:
          f.template operator()<xvec_ua<uint16_t, 2>>();
          return true;
        case Size::_64bit:
          f.template operator()<xvec_ua<uint32_t, 2>>();
          return true;
        default:
          return false;
        }
      case Format::Float:
        switch(size)
        {
        case Size::_64bit:
          f.template operator()<xvec<float, 2>>();
          return true;
        default:
          return false;
        }
      default:
        return false;
      }
    case Components::RGB:
      switch(format)
      {
      case Format::Uint:
        switch(size)
        {
        case Size::_24bit:
          f.template operator()<xvec_ua<uint8_t, 3>>();
          return true;
        case Size::_48bit:
          f.template operator()<xvec_ua<uint16_t, 3>>();
          return true;
        case Size::_96bit:
          f.template operator()<xvec_ua<uint32_t, 3>>();
          return true;
        default:
          return false;
        }
      case Format::Float:
        switch(size)
        {
        case Size::_96bit:
          f.template operator()<xvec<float, 3>>();
          return true;
        default:
          return false;
        }
      default:
        return false;
      }
    case Components::RGBA:
      switch(format)
      {
      case Format::Uint:
        switch(size)
        {
        case Size::_32bit:
          f.template operator()<xvec_ua<uint8_t, 4>>();
          return true;
        case Size::_64bit:
          f.template operator()<xvec_ua<uint16_t, 4>>();
          return true;
        case Size::_128bit:
          f.template operator()<xvec_ua<uint32_t, 4>>();
          return true;
        default:
          return false;
        }
      case Format::Float:
        switch(size)
        {
        case Size::_128bit:
          f.template operator()<xvec<float, 4>>();
          return true;
        default:
          return false;
        }
      default:
        return false;
      }
    default:
      return false;
    }
  }

  // call templated function with every mip of uncompressed image buffer
  // the function is called with typed RawImageSlice, image index, and mip index
  template <typename F>
  void ProcessImageBufferMips(ImageBuffer const& imageBuffer, F const& f)
  {
    auto metrics = imageBuffer.format.GetMetrics();
    size_t imagesCount = imageBuffer.format.count ? imageBuffer.format.count : 1;

    auto process = [&]<size_t n>()
    {
      if(!imageBuffer.format.format.ApplyTyped([&]<typename T>()
      {
        for(size_t imageIndex = 0; imageIndex < imagesCount; ++imageIndex)
        {
          for(size_t mipIndex = 0; mipIndex < metrics.mips.size(); ++mipIndex)
          {
            auto const& mip = metrics.mips[mipIndex];

            RawImageSlice<T, n> imageSlice =
            {
              .pixels = (T*)((uint8_t*)imageBuffer.buffer.data + imageIndex * metrics.imageSize + mip.offset),
            };

            if constexpr(0 < n)
            {
              imageSlice.size.x() = mip.width;
              imageSlice.pitch.x() = metrics.pixelSize;
            }
            if constexpr(1 < n)
            {
              imageSlice.size.y() = mip.height;
              imageSlice.pitch.y() = imageSlice.pitch.x() * imageSlice.size.x();
            }
            if constexpr(2 < n)
            {
              imageSlice.size.z() = mip.depth;
              imageSlice.pitch.z() = imageSlice.pitch.y() * imageSlice.size.y();
            }

            f(imageSlice, imageIndex, mipIndex);
          }
        }
      }))
        throw Exception("Unsupported format to process mips");
    };

    if(imageBuffer.format.height > 0)
    {
      if(imageBuffer.format.depth > 0)
      {
        process.template operator()<3>();
      }
      else
      {
        process.template operator()<2>();
      }
    }
    else
    {
      process.template operator()<1>();
    }
  }
}
