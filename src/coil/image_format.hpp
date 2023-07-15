#pragma once

#include "base.hpp"
#include <vector>

namespace Coil
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
    constexpr PixelFormat(Components components, Format format, Size size, bool srgb = false);
    // Compressed format constructor.
    constexpr PixelFormat(Compression compression, bool srgb = false);

    // Set pixel components, preserving correct size.
    void SetComponents(Components newComponents);

    // apply function to type representing pixel format
    // returns false if format is not supported
    template <typename F>
    bool ApplyTyped(F const& f) const;

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

  template <> PixelFormat::Compression FromString(std::string_view str);

  struct PixelFormats
  {
    static PixelFormat const uintR8;
    static PixelFormat const uintR8S;
    static PixelFormat const uintRGB24;
    static PixelFormat const uintRGB24S;
    static PixelFormat const uintRGBA32;
    static PixelFormat const uintRGBA32S;
    static PixelFormat const floatR16;
    static PixelFormat const floatR32;
    static PixelFormat const floatRGB32;
    static PixelFormat const floatRGBA64;
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

    ImageMetrics GetMetrics() const;
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
