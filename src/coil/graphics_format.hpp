#pragma once

#include "math.hpp"

namespace Coil
{
  // Vertex attribute format.
  struct VertexFormat
  {
    enum class Format : uint8_t
    {
      // float
      Float,
      // signed int
      SInt,
      // unsigned int
      UInt,
      // signed int, normalized to -1..1 float
      SNorm,
      // unsigned int, normalized to 0..1 float
      UNorm,
    };
    Format format : 3;

    // number of components, 1..4
    uint8_t components : 3 = 1;

    // size of attribute, in bytes
    uint8_t size : 5;

    constexpr VertexFormat(Format format, uint8_t components, uint8_t size)
    : format(format), components(components), size(size) {}
  };

  // scalar traits
  template <typename T>
  struct VertexFormatScalarTraits;
  template <>
  struct VertexFormatScalarTraits<float>
  {
    static constexpr VertexFormat::Format format = VertexFormat::Format::Float;
  };
  template <>
  struct VertexFormatScalarTraits<uint32_t>
  {
    static constexpr VertexFormat::Format format = VertexFormat::Format::UInt;
    static constexpr VertexFormat::Format normFormat = VertexFormat::Format::UNorm;
  };
  template <>
  struct VertexFormatScalarTraits<int32_t>
  {
    static constexpr VertexFormat::Format format = VertexFormat::Format::SInt;
    static constexpr VertexFormat::Format normFormat = VertexFormat::Format::SNorm;
  };

  template <typename T>
  concept IsVertexFormatScalar = requires
  {
    VertexFormatScalarTraits<T>::format;
  };
  template <typename T>
  concept IsVertexFormatNormScalar = requires
  {
    VertexFormatScalarTraits<T>::normFormat;
  };

  // vector of normalized values (always unaligned)
  template <typename T, size_t n>
  struct norm_xvec : public xvec<T, n, 0>
  {
  };

  template <typename T>
  struct VertexFormatTraits;
  template <IsVertexFormatScalar T, size_t n>
  struct VertexFormatTraits<xvec<T, n, 0>>
  {
    using Value = xvec<T, n>;
    static constexpr size_t components = n;
    static constexpr size_t size = sizeof(xvec<T, n, 0>);
    static constexpr VertexFormat format = VertexFormat(VertexFormatScalarTraits<T>::format, n, sizeof(xvec<T, n, 0>));
  };
  template <typename T>
  struct VertexFormatTraits<xquat<T, 0>> : public VertexFormatTraits<xvec<T, 4, 0>>
  {
    using Value = xquat<T>;
  };
  template <IsVertexFormatNormScalar T, size_t n>
  struct VertexFormatTraits<norm_xvec<T, n>>
  {
    using Value = xvec<float, n>;
    static constexpr size_t components = n;
    static constexpr size_t size = sizeof(norm_xvec<T, n>);
    static constexpr VertexFormat format = VertexFormat(VertexFormatScalarTraits<T>::normFormat, n, sizeof(norm_xvec<T, n>));
  };

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
