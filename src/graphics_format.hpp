#pragma once

#include "base.hpp"

namespace Coil
{
  // Pixel format.
  struct PixelFormat
  {
    // General type of the pixel data.
    enum class Type : uint8_t
    {
      Unknown, Uncompressed, Compressed
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
      Untyped, Uint, Float
    };
    // Size of one pixel, in bytes.
    enum class Size : uint8_t
    {
      _8bit,
      _16bit,
      _24bit,
      _32bit,
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

    // Unknown format constructor.
    PixelFormat() = default;
    // Uncompressed format constructor.
    PixelFormat(Components components, Format format, Size size, bool srgb = false);
    // Compressed format constructor.
    PixelFormat(Compression compression, bool srgb = false);

    // Set pixel components, preserving correct size.
    void SetComponents(Components newComponents);

    static size_t GetPixelSize(Size size);

    friend auto operator<=>(PixelFormat const&, PixelFormat const&) = default;
  };

  struct PixelFormats
  {
    static PixelFormat const uintRGBA32;
    static PixelFormat const uintRGBA32S;
    static PixelFormat const uintRGB24;
    static PixelFormat const uintRGB24S;
    static PixelFormat const floatR16;
    static PixelFormat const floatR32;
    static PixelFormat const floatRGB32;
    static PixelFormat const floatRGBA64;
  };
}
