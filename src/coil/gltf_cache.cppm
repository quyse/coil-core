module;

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

export module coil.core.gltf.cache;

import coil.core.base;
import coil.core.gltf;
import coil.core.math;
import coil.core.mesh;

export namespace Coil
{
  class GLTFCache
  {
  protected:
    GLTFCache(GLTF const& gltf)
    : _gltf(gltf) {}

    template <typename Output, typename OutputField>
    void CopyAccessorData(std::vector<Output>& output, size_t outputOffset, GLTF::AccessorIndex accessorIndex)
    {
      auto const& accessor = GetAccessor(accessorIndex);

      if(output.empty())
      {
        output.resize(accessor.count);
      }
      else if(output.size() != accessor.count)
      {
        throw Exception("inconsistent accessor count");
      }

      if(accessor.bufferView.has_value())
      {
        auto const& bufferView = GetBufferView(accessor.bufferView.value());
        auto const& buffer = GetBuffer(bufferView.buffer);

        uint8_t* outputPtr = (uint8_t*)output.data() + outputOffset;
        uint8_t const* inputPtr = buffer.data.data() + bufferView.byteOffset;

        constexpr size_t componentsCount = VectorTraits<OutputField>::N;
        using OutputComponent = typename VectorTraits<OutputField>::Scalar;

        CheckAccessorType(accessor.type, componentsCount);

        auto copyData = [&buffer, &bufferView, n = output.size(), outputPtr, inputPtr, normalized = accessor.normalized]<typename InputComponent>()
        {
          size_t inputStride = bufferView.byteStride.value_or(componentsCount * sizeof(InputComponent));

          if(buffer.data.size() < bufferView.byteOffset + n * inputStride)
            throw Exception("buffer too small");

          // multiplier for normalize conversion
          OutputComponent k = normalized ? 1 / (OutputComponent)std::numeric_limits<InputComponent>::max() : 1;

          for(size_t i = 0; i < n; ++i)
            for(size_t j = 0; j < componentsCount; ++j)
              ((OutputComponent*)(outputPtr + i * sizeof(Output)))[j] = (OutputComponent)((InputComponent*)(inputPtr + i * inputStride))[j] * k;
        };

        switch(accessor.componentType)
        {
        case 5120: // BYTE
          copyData.template operator()<int8_t>();
          break;
        case 5121: // UNSIGNED_BYTE
          copyData.template operator()<uint8_t>();
          break;
        case 5122: // SHORT
          copyData.template operator()<int16_t>();
          break;
        case 5123: // UNSIGNED_SHORT
          copyData.template operator()<uint16_t>();
          break;
        case 5125: // UNSIGNED_INT
          copyData.template operator()<uint32_t>();
          break;
        case 5126: // FLOAT
          copyData.template operator()<float>();
          break;
        default:
          throw Exception() << "unsupported accessor component type: " << accessor.componentType;
        }
      }
      // otherwise, no buffer view means zero data
      // assuming it's already zero-initialized, there's nothing to do
    }

    GLTF::Accessor const& GetAccessor(GLTF::AccessorIndex accessorIndex)
    {
      if(accessorIndex >= _gltf.accessors.size())
        throw Exception() << "invalid accessor index " << accessorIndex;
      return _gltf.accessors[accessorIndex];
    }
    GLTF::Buffer const& GetBuffer(GLTF::BufferIndex bufferIndex)
    {
      if(bufferIndex >= _gltf.buffers.size())
        throw Exception() << "invalid buffer index " << bufferIndex;
      return _gltf.buffers[bufferIndex];
    }
    GLTF::BufferView const& GetBufferView(GLTF::BufferViewIndex bufferViewIndex)
    {
      if(bufferViewIndex >= _gltf.bufferViews.size())
        throw Exception() << "invalid buffer view index " << bufferViewIndex;
      return _gltf.bufferViews[bufferViewIndex];
    }
    GLTF::Mesh const& GetMesh(GLTF::MeshIndex meshIndex)
    {
      if(meshIndex >= _gltf.meshes.size())
        throw Exception() << "invalid mesh index " << meshIndex;
      return _gltf.meshes[meshIndex];
    }

    void CheckAccessorType(std::string const& accessorType, size_t componentsCount)
    {
      static std::unordered_map<std::string, size_t> const accessorTypeCounts =
      {
        { "SCALAR", 1 },
        { "VEC2", 2 },
        { "VEC3", 3 },
        { "VEC4", 4 },
        { "MAT2", 4 },
        { "MAT3", 9 },
        { "MAT4", 16 },
      };

      auto i = accessorTypeCounts.find(accessorType);
      if(i == accessorTypeCounts.end() || i->second != componentsCount)
        throw Exception() << "wrong accessor type: " << accessorType;
    }

    GLTF const& _gltf;
  };

  template <typename Vertex>
  class GLTFMeshCache : public GLTFCache
  {
  public:
    GLTFMeshCache(GLTF const& gltf)
    : GLTFCache(gltf), _meshes(_gltf.meshes.size()) {}

    AssetMesh<Vertex> const& operator[](GLTF::MeshIndex meshIndex)
    {
      auto& mesh = _meshes[meshIndex];
      if(!mesh.has_value())
      {
        Import(meshIndex);
      }
      return mesh.value();
    }

  protected:
    void Import(GLTF::MeshIndex meshIndex)
    {
      try
      {
        AssetMesh<Vertex> geometry;

        auto const& mesh = GetMesh(meshIndex);

        // supporting only one primitive currently
        if(mesh.primitives.size() != 1)
          throw Exception("only exactly one mesh primitive supported");

        GLTF::Mesh::MeshPrimitive const& meshPrimitive = mesh.primitives[0];

        // supporting only triangles
        if(meshPrimitive.mode != GLTF::Mesh::MeshPrimitive::Mode::Triangles)
          throw Exception("only triangles supported");

        // import vertex attributes
        for(auto const& i : meshPrimitive.attributes)
        {
          std::string const& attributeName = i.first;

          if constexpr(IsVertexWithPosition<Vertex>)
          {
            if(attributeName == "POSITION")
            {
              CopyAccessorData<Vertex, vec3_ua>(geometry.vertices, offsetof(Vertex, position), i.second);
            }
          }
          if constexpr(IsVertexWithNormal<Vertex>)
          {
            if(attributeName == "NORMAL")
            {
              CopyAccessorData<Vertex, vec3_ua>(geometry.vertices, offsetof(Vertex, normal), i.second);
            }
          }
          if constexpr(IsVertexWithTexcoord<Vertex>)
          {
            if(attributeName == "TEXCOORD_0")
            {
              CopyAccessorData<Vertex, vec2_ua>(geometry.vertices, offsetof(Vertex, texcoord), i.second);
            }
          }
        }

        // import indices
        if(meshPrimitive.indices.has_value())
        {
          CopyAccessorData<uint32_t, uint32_t>(geometry.indices, 0, meshPrimitive.indices.value());
        }

        _meshes[meshIndex] = std::move(geometry);
      }
      catch(Exception const& exception)
      {
        throw Exception() << "getting glTF mesh " << meshIndex << " failed" << exception;
      }
    }

    std::vector<std::optional<AssetMesh<Vertex>>> _meshes;
  };
}
