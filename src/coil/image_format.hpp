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
    Type type = Type::Uncompressed;
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

    // Uncompressed format constructor.
    constexpr PixelFormat(Components components, Format format, Size size, bool srgb = false);
    // Compressed format constructor.
    constexpr PixelFormat(Compression compression, bool srgb = false);

    // Set pixel components, preserving correct size.
    void SetComponents(Components newComponents);

    static size_t GetPixelSize(Size size);

    friend auto operator<=>(PixelFormat const&, PixelFormat const&) = default;
  };

  struct PixelFormats
  {
    static PixelFormat const uintR8;
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
    int32_t width;
    int32_t height;
    int32_t depth;
    // must be >= 1
    int32_t mips;
    // zero means non-array
    int32_t count;

    ImageMetrics GetMetrics() const;
  };

  struct ImageBuffer
  {
    ImageFormat format;
    Buffer buffer;
  };
}
