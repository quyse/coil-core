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

  // untyped mesh container
  struct AssetMeshBuffer
  {
    uint32_t verticesCount;
    uint32_t vertexStride;
    std::vector<uint8_t> vertices;
    uint32_t indicesCount;
    uint32_t indexStride;
    std::vector<uint8_t> indices;
  };

  // typed mesh container
  template <typename V>
  struct AssetMesh
  {
    using Vertex = V;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    AssetMeshBuffer ToBuffer()
    {
      AssetMeshBuffer buffer;
      buffer.verticesCount = (uint32_t)vertices.size();
      buffer.vertexStride = sizeof(Vertex);
      buffer.vertices.resize(buffer.verticesCount * buffer.vertexStride);
      buffer.indicesCount = (uint32_t)indices.size();
      buffer.indexStride = buffer.indicesCount <= 0xFFFF ? sizeof(uint16_t) : sizeof(uint32_t);
      buffer.indices.resize(buffer.indicesCount * buffer.indexStride);

      std::copy_n(vertices.data(), buffer.verticesCount, (Vertex*)buffer.vertices.data());
      if(buffer.indexStride == sizeof(uint16_t))
      {
        std::copy_n(indices.data(), buffer.indicesCount, (uint16_t*)buffer.indices.data());
      }
      else
      {
        std::copy_n(indices.data(), buffer.indicesCount, (uint32_t*)buffer.indices.data());
      }

      return std::move(buffer);
    }
  };
}
