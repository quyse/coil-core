module;

#include <unordered_map>
#include <vector>

export module coil.core.image.format;

import coil.core.base;
import coil.core.math;

export namespace Coil
{
  // Pixel format.
  struct PixelFormat
  {
    // General type of the pixel data.
    enum class Type : uint8_t
    {
      Uncompressed, Compressed
    };
    Type type;
    // What components are in pixel.
    enum class Components : uint8_t
    {
      R, RG, RGB, RGBA
    };
    // Format of one component.
    enum class Format : uint8_t
    {
      Uint, Float
    };
    // Size of one pixel, in bytes.
    enum class Size : uint8_t
    {
      _8bit,
      _16bit,
      _24bit,
      _32bit,
      _48bit,
      _64bit,
      _96bit,
      _128bit
    };
    // Compression format.
    enum class Compression : uint8_t
    {
      // BC1, or DXT1: RGB, size of 4x4 = 8 bytes
      Bc1,
      Bc1Alpha,
      // BC2, or DXT3: RGBA, size of 4x4 = 16 bytes (low cogerency of color and alpha)
      Bc2,
      // BC3, or DXT5: RGBA, size of 4x4 = 16 bytes (high cogerency of color data)
      Bc3,
      // BC4, or ATI1: R, size of 4x4 = 8 bytes
      Bc4,
      Bc4Signed,
      // BC5, or ATI2: RG, size of 4x4 = 16 bytes
      Bc5,
      Bc5Signed
    };

    union
    {
      // Struct for uncompressed data.
      struct
      {
        Components components;
        Format format;
        Size size;
      };
      // Struct for compressed data.
      struct
      {
        Compression compression;
      };
    };

    // Is color data is in sRGB space.
    /** Valid only for type != typeUnknown. */
    bool srgb = false;

    constexpr PixelFormat() = default;
    // Uncompressed format constructor.
    constexpr PixelFormat(Components components, Format format, Size size, bool srgb = false)
    : type(Type::Uncompressed), components(components), format(format), size(size), srgb(srgb)
    {}
    // Compressed format constructor.
    constexpr PixelFormat(Compression compression, bool srgb = false)
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

    // Set pixel components, preserving correct size.
    void SetComponents(Components newComponents)
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

    // apply function to type representing pixel format
    // returns false if format is not supported
    template <typename F>
    bool ApplyTyped(F const& f) const
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

    static constexpr size_t GetPixelSize(Size size)
    {
      switch(size)
      {
      case Size::_8bit: return 1;
      case Size::_16bit: return 2;
      case Size::_24bit: return 3;
      case Size::_32bit: return 4;
      case Size::_64bit: return 8;
      case Size::_96bit: return 12;
      case Size::_128bit: return 16;
      default: return 0;
      }
    }

    struct CompressionMetrics
    {
      int32_t blockWidth = 0;
      int32_t blockHeight = 0;
      size_t blockSize = 0;
    };

    static constexpr CompressionMetrics GetCompressionMetrics(Compression compression)
    {
      switch(compression)
      {
      case PixelFormat::Compression::Bc1:
      case PixelFormat::Compression::Bc1Alpha:
        return
        {
          .blockWidth = 4,
          .blockHeight = 4,
          .blockSize = 8,
        };
      case PixelFormat::Compression::Bc2:
      case PixelFormat::Compression::Bc3:
        return
        {
          .blockWidth = 4,
          .blockHeight = 4,
          .blockSize = 16,
        };
      case PixelFormat::Compression::Bc4:
      case PixelFormat::Compression::Bc4Signed:
        return
        {
          .blockWidth = 4,
          .blockHeight = 4,
          .blockSize = 8,
        };
      case PixelFormat::Compression::Bc5:
      case PixelFormat::Compression::Bc5Signed:
        return
        {
          .blockWidth = 4,
          .blockHeight = 4,
          .blockSize = 16,
        };
      default: return {};
      }
    }

    friend auto operator<=>(PixelFormat const&, PixelFormat const&) = default;
  };

  template <> PixelFormat::Compression FromString(std::string_view str)
  {
    static std::unordered_map<std::string_view, PixelFormat::Compression> const values =
    {{
      { "Bc1", PixelFormat::Compression::Bc1 },
      { "Bc1Alpha", PixelFormat::Compression::Bc1Alpha },
      { "Bc2", PixelFormat::Compression::Bc2 },
      { "Bc3", PixelFormat::Compression::Bc3 },
      { "Bc4", PixelFormat::Compression::Bc4 },
      { "Bc4Signed", PixelFormat::Compression::Bc4Signed },
      { "Bc5", PixelFormat::Compression::Bc5 },
      { "Bc5Signed", PixelFormat::Compression::Bc5Signed },
    }};
    auto i = values.find(str);
    if(i == values.end())
      throw Exception() << "invalid pixel format compression: " << str;
    return i->second;
  }

  namespace PixelFormats
  {
    constinit PixelFormat const uintR8
    {
      PixelFormat::Components::R,
      PixelFormat::Format::Uint,
      PixelFormat::Size::_8bit,
    };
    constinit PixelFormat const uintR8S
    {
      PixelFormat::Components::R,
      PixelFormat::Format::Uint,
      PixelFormat::Size::_8bit,
      true,
    };
    constinit PixelFormat const uintRGB24
    {
      PixelFormat::Components::RGB,
      PixelFormat::Format::Uint,
      PixelFormat::Size::_24bit,
    };
    constinit PixelFormat const uintRGB24S
    {
      PixelFormat::Components::RGB,
      PixelFormat::Format::Uint,
      PixelFormat::Size::_24bit,
      true,
    };
    constinit PixelFormat const uintRGBA32
    {
      PixelFormat::Components::RGBA,
      PixelFormat::Format::Uint,
      PixelFormat::Size::_32bit,
    };
    constinit PixelFormat const uintRGBA32S
    {
      PixelFormat::Components::RGBA,
      PixelFormat::Format::Uint,
      PixelFormat::Size::_32bit,
      true,
    };
    constinit PixelFormat const floatR16
    {
      PixelFormat::Components::R,
      PixelFormat::Format::Float,
      PixelFormat::Size::_16bit,
    };
    constinit PixelFormat const floatR32
    {
      PixelFormat::Components::R,
      PixelFormat::Format::Float,
      PixelFormat::Size::_32bit,
    };
    constinit PixelFormat const floatRGB32
    {
      PixelFormat::Components::RGB,
      PixelFormat::Format::Float,
      PixelFormat::Size::_32bit,
    };
    constinit PixelFormat const floatRGBA64
    {
      PixelFormat::Components::RGBA,
      PixelFormat::Format::Float,
      PixelFormat::Size::_64bit,
    };
  };

  struct ImageMetrics
  {
    uint32_t pixelSize;
    struct Mip
    {
      int32_t width;
      int32_t height;
      int32_t depth;
      uint32_t size;
      uint32_t offset;
      // buffer size (for block formats it's rounded up from width/height)
      int32_t bufferWidth;
      int32_t bufferHeight;
    };
    std::vector<Mip> mips;
    uint32_t imageSize;
  };

  struct ImageFormat
  {
    PixelFormat format;
    // acceptable sizes:
    // width height depth
    // W     0      0      1D texture
    // W     1      0      2D texture Wx1
    // W     H      0      2D texture WxH
    // W     H      1      3D texture WxHx1
    // W     H      D      3D texture WxHxD
    int32_t width = 0;
    int32_t height = 0;
    int32_t depth = 0;
    // must be >= 1
    int32_t mips = 1;
    // zero means non-array
    int32_t count = 0;

    ImageMetrics GetMetrics() const
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
          mip.bufferWidth = mip.width;
          mip.bufferHeight = mip.height;
          break;
        case PixelFormat::Type::Compressed:
          {
            auto compressionMetrics = PixelFormat::GetCompressionMetrics(format.compression);
            int32_t blockWidth = (mip.width + compressionMetrics.blockWidth - 1) / compressionMetrics.blockWidth;
            int32_t blockHeight = (mip.height + compressionMetrics.blockHeight - 1) / compressionMetrics.blockHeight;
            mip.size = blockWidth * blockHeight * mip.depth * compressionMetrics.blockSize;
            mip.bufferWidth = blockWidth * compressionMetrics.blockWidth;
            mip.bufferHeight = blockHeight * compressionMetrics.blockHeight;
          }
          break;
        }

        mip.offset = mipOffset;
        mipOffset += mip.size;
      }
      metrics.imageSize = mipOffset;
      return metrics;
    }
  };

  struct ImageBuffer
  {
    ImageFormat format;
    Buffer buffer;
  };

  template <>
  struct AssetTraits<ImageBuffer>
  {
    static constexpr std::string_view assetTypeName = "image";
  };
  static_assert(IsAsset<ImageBuffer>);
}
