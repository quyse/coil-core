module;

#include <cstddef>
#include <cstdint>

export module coil.core.graphics.format;

import coil.core.math;

export namespace Coil
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

  template <IsVertexFormatScalar T>
  struct VertexFormatTraits<T>
  {
    using Value = T;
    static constexpr size_t components = 1;
    static constexpr size_t size = sizeof(T);
    static constexpr VertexFormat format = VertexFormat{VertexFormatScalarTraits<T>::format, 1, sizeof(T)};
  };

  template <IsVertexFormatScalar T, size_t n>
  struct VertexFormatTraits<xvec<T, n, 0>>
  {
    using Value = xvec<T, n>;
    static constexpr size_t components = n;
    static constexpr size_t size = sizeof(xvec<T, n, 0>);
    static constexpr VertexFormat format = VertexFormat{VertexFormatScalarTraits<T>::format, n, sizeof(xvec<T, n, 0>)};
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
    static constexpr VertexFormat format = VertexFormat{VertexFormatScalarTraits<T>::normFormat, n, sizeof(norm_xvec<T, n>)};
  };
}
