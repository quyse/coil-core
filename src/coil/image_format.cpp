#include "image_format.hpp"

namespace Coil
{
  constexpr PixelFormat::PixelFormat(Components components, Format format, Size size, bool srgb)
  : type(Type::Uncompressed), components(components), format(format), size(size), srgb(srgb)
  {}

  constexpr PixelFormat::PixelFormat(Compression compression, bool srgb)
  : type(Type::Compressed), compression(compression), srgb(srgb)
  {
    switch(compression)
    {
    case Compression::Bc1:
    case Compression::Bc1Alpha:
    case Compression::Bc2:
    case Compression::Bc3:
      break;
    case Compression::Bc4:
    case Compression::Bc4Signed:
    case Compression::Bc5:
    case Compression::Bc5Signed:
      if(srgb)
        throw Exception("wrong compression format for sRGB");
      break;
    default:
      throw Exception("wrong compression format");
    }
  }

  void PixelFormat::SetComponents(Components newComponents)
  {
    switch(components)
    {
    case Components::R:
      switch(newComponents)
      {
      case Components::R:
        break;
      case Components::RG:
        switch(size)
        {
        case Size::_8bit: size = Size::_16bit; break;
        case Size::_16bit: size = Size::_32bit; break;
        case Size::_32bit: size = Size::_64bit; break;
        case Size::_64bit: size = Size::_128bit; break;
        default: throw Exception("unsupported size for R->RG pixel conversion");
        }
        break;
      case Components::RGB:
        switch(size)
        {
        case Size::_8bit: size = Size::_24bit; break;
        case Size::_32bit: size = Size::_96bit; break;
        default: throw Exception("unsupported size for R->RGB pixel conversion");
        }
        break;
      case Components::RGBA:
        switch(size)
        {
        case Size::_8bit: size = Size::_32bit; break;
        case Size::_16bit: size = Size::_64bit; break;
        case Size::_32bit: size = Size::_128bit; break;
        default: throw Exception("unsupported size for R->RGBA pixel conversion");
        }
        break;
      }
      break;
    case Components::RG:
      switch(newComponents)
      {
      case Components::R:
        switch(size)
        {
        case Size::_16bit: size = Size::_8bit; break;
        case Size::_32bit: size = Size::_16bit; break;
        case Size::_64bit: size = Size::_32bit; break;
        case Size::_128bit: size = Size::_64bit; break;
        default: throw Exception("unsupported size for RG->R pixel conversion");
        }
        break;
      case Components::RG:
        break;
      case Components::RGB:
        switch(size)
        {
        case Size::_16bit: size = Size::_24bit; break;
        case Size::_64bit: size = Size::_96bit; break;
        default: throw Exception("unsupported size for RG->RGB pixel conversion");
        }
        break;
      case Components::RGBA:
        switch(size)
        {
        case Size::_16bit: size = Size::_32bit; break;
        case Size::_32bit: size = Size::_64bit; break;
        case Size::_64bit: size = Size::_128bit; break;
        default: throw Exception("unsupported size for RG->RGBA pixel conversion");
        }
        break;
      }
      break;
    case Components::RGB:
      switch(newComponents)
      {
      case Components::R:
        switch(size)
        {
        case Size::_24bit: size = Size::_8bit; break;
        case Size::_96bit: size = Size::_32bit; break;
        default: throw Exception("unsupported size for RGB->R pixel conversion");
        }
        break;
      case Components::RG:
        switch(size)
        {
        case Size::_24bit: size = Size::_16bit; break;
        case Size::_96bit: size = Size::_64bit; break;
        default: throw Exception("unsupported size for RGB->RG pixel conversion");
        }
        break;
      case Components::RGB:
        break;
      case Components::RGBA:
        switch(size)
        {
        case Size::_24bit: size = Size::_32bit; break;
        case Size::_96bit: size = Size::_128bit; break;
        default: throw Exception("unsupported size for RGB->RGBA pixel conversion");
        }
        break;
      }
      break;
    case Components::RGBA:
      switch(newComponents)
      {
      case Components::R:
        switch(size)
        {
        case Size::_32bit: size = Size::_8bit; break;
        case Size::_64bit: size = Size::_16bit; break;
        case Size::_128bit: size = Size::_32bit; break;
        default: throw Exception("unsupported size for RGBA->R pixel compression");
        }
        break;
      case Components::RG:
        switch(size)
        {
        case Size::_32bit: size = Size::_16bit; break;
        case Size::_64bit: size = Size::_32bit; break;
        case Size::_128bit: size = Size::_64bit; break;
        default: throw Exception("unsupported size for RGBA->RG pixel compression");
        }
        break;
      case Components::RGB:
        switch(size)
        {
        case Size::_32bit: size = Size::_24bit; break;
        case Size::_128bit: size = Size::_96bit; break;
        default: throw Exception("unsupported size for RGBA->RGB pixel conversion");
        }
        break;
      case Components::RGBA:
        break;
      }
      break;
    }
  }

  PixelFormat const PixelFormats::uintR8(
    PixelFormat::Components::R,
    PixelFormat::Format::Uint,
    PixelFormat::Size::_8bit);
  PixelFormat const PixelFormats::uintR8S(
    PixelFormat::Components::R,
    PixelFormat::Format::Uint,
    PixelFormat::Size::_8bit,
    true);
  PixelFormat const PixelFormats::uintRGB24(
    PixelFormat::Components::RGB,
    PixelFormat::Format::Uint,
    PixelFormat::Size::_24bit);
  PixelFormat const PixelFormats::uintRGB24S(
    PixelFormat::Components::RGB,
    PixelFormat::Format::Uint,
    PixelFormat::Size::_24bit,
    true);
  PixelFormat const PixelFormats::uintRGBA32(
    PixelFormat::Components::RGBA,
    PixelFormat::Format::Uint,
    PixelFormat::Size::_32bit);
  PixelFormat const PixelFormats::uintRGBA32S(
    PixelFormat::Components::RGBA,
    PixelFormat::Format::Uint,
    PixelFormat::Size::_32bit,
    true);
  PixelFormat const PixelFormats::floatR16(
    PixelFormat::Components::R,
    PixelFormat::Format::Float,
    PixelFormat::Size::_16bit);
  PixelFormat const PixelFormats::floatR32(
    PixelFormat::Components::R,
    PixelFormat::Format::Float,
    PixelFormat::Size::_32bit);
  PixelFormat const PixelFormats::floatRGB32(
    PixelFormat::Components::RGB,
    PixelFormat::Format::Float,
    PixelFormat::Size::_32bit);
  PixelFormat const PixelFormats::floatRGBA64(
    PixelFormat::Components::RGBA,
    PixelFormat::Format::Float,
    PixelFormat::Size::_64bit);

  ImageMetrics ImageFormat::GetMetrics() const
  {
    ImageMetrics metrics;
    metrics.pixelSize = format.type == PixelFormat::Type::Uncompressed ? PixelFormat::GetPixelSize(format.size) : 0;
    metrics.mips.resize(mips);
    uint32_t mipOffset = 0;
    for(int32_t i = 0; i < mips; ++i)
    {
      auto& mip = metrics.mips[i];
      mip.width = std::max(width >> i, 1);
      mip.height = std::max(height >> i, 1);
      mip.depth = std::max(depth >> i, 1);

      switch(format.type)
      {
      case PixelFormat::Type::Uncompressed:
        mip.size = mip.width * mip.height * mip.depth * metrics.pixelSize;
        break;
      case PixelFormat::Type::Compressed:
        {
          auto compressionMetrics = PixelFormat::GetCompressionMetrics(format.compression);
          mip.size =
            ((mip.width + compressionMetrics.blockWidth - 1) / compressionMetrics.blockWidth) *
            ((mip.height + compressionMetrics.blockHeight - 1) / compressionMetrics.blockHeight) *
            mip.depth *
            compressionMetrics.blockSize;
        }
        break;
      }

      mip.offset = mipOffset;
      mipOffset += mip.size;
    }
    metrics.imageSize = mipOffset;
    return metrics;
  }
}
