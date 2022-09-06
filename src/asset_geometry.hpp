#pragma once

#include "math.hpp"

namespace Coil
{
  template <typename Vertex>
  concept IsVertexWithPosition = requires(Vertex v)
  {
    { v.position } -> std::same_as<xvec<typename Vertex::PositionComponent, 3, 0>&>;
  };

  template <typename Vertex>
  concept IsVertexWithNormal = requires(Vertex v)
  {
    { v.normal } -> std::same_as<xvec<typename Vertex::NormalComponent, 3, 0>&>;
  };

  template <typename Vertex>
  concept IsVertexWithTexcoord = requires(Vertex v)
  {
    { v.texcoord } -> std::same_as<xvec<typename Vertex::TexcoordComponent, 2, 0>&>;
  };

  template <typename T = float>
  struct VertexP
  {
    using PositionComponent = T;

    xvec<T, 3, 0> position;
  };

  template <typename T = float>
  struct VertexPQ
  {
    using PositionComponent = T;
    using RotationComponent = T;

    xvec<T, 3, 0> position;
    xquat<T, 0> rotation;
  };

  // position+normal+texcoord vertex struct
  template <typename T = float>
  struct VertexPNT
  {
    using PositionComponent = T;
    using NormalComponent = T;
    using TexcoordComponent = T;

    xvec<T, 3, 0> position;
    xvec<T, 3, 0> normal;
    xvec<T, 2, 0> texcoord;

    friend auto operator<=>(VertexPNT const&, VertexPNT const&) = default;
  };

  static_assert(IsVertexWithPosition<VertexPNT<>>);
  static_assert(IsVertexWithNormal<VertexPNT<>>);
  static_assert(IsVertexWithTexcoord<VertexPNT<>>);
  static_assert(sizeof(VertexPNT<>) == 32 && alignof(VertexPNT<>) == 4);

  template <typename V, typename I>
  struct AssetGeometry
  {
    using Vertex = V;
    using Index = I;

    std::vector<Vertex> vertices;
    std::vector<Index> indices;
  };

  template <size_t n>
  struct OptimalIndexHelper
  {
    using Index = uint32_t;
  };
  template <size_t n>
  requires(n <= 0xFFFF)
  struct OptimalIndexHelper<n>
  {
    using Index = uint16_t;
  };
  template <size_t n>
  using OptimalIndex = typename OptimalIndexHelper<n>::Index;
}
