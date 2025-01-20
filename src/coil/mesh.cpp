module;

#include <vector>

export module coil.core.mesh;

import coil.core.math;

export namespace Coil
{
  template <typename Vertex>
  concept IsVertexWithPosition = requires(Vertex v)
  {
    VectorTraits<decltype(v.position)>::N == 3;
  };

  template <typename Vertex>
  concept IsVertexWithNormal = requires(Vertex v)
  {
    VectorTraits<decltype(v.normal)>::N == 3;
  };

  template <typename Vertex>
  concept IsVertexWithTexcoord = requires(Vertex v)
  {
    VectorTraits<decltype(v.texcoord)>::N == 2;
  };

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

    AssetMeshBuffer ToBuffer() const
    {
      AssetMeshBuffer buffer;
      buffer.verticesCount = (uint32_t)vertices.size();
      buffer.vertexStride = sizeof(Vertex);
      buffer.vertices.resize(buffer.verticesCount * buffer.vertexStride);
      buffer.indicesCount = (uint32_t)indices.size();
      buffer.indexStride = buffer.verticesCount <= 0xFFFF ? sizeof(uint16_t) : sizeof(uint32_t);
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
