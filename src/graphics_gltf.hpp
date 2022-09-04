#pragma once

#include "math.hpp"
#include <vector>
#include <unordered_map>
#include <optional>
#include <string>

namespace Coil
{
  struct GLTF
  {
    using ObjectsByName = std::unordered_map<std::string, uint32_t>;

    using AccessorIndex = uint32_t;
    using BufferIndex = uint32_t;
    using BufferViewIndex = uint32_t;
    using MeshIndex = uint32_t;
    using NodeIndex = uint32_t;
    using SceneIndex = uint32_t;

    struct Accessor
    {
      std::optional<BufferViewIndex> bufferView;
      uint32_t byteOffset;
      uint32_t componentType;
      bool normalized;
      uint32_t count;
      std::string type;
    };
    std::vector<Accessor> accessors;

    struct Buffer
    {
      std::string uri;
      uint32_t byteLength;
      std::vector<uint8_t> data;
    };
    std::vector<Buffer> buffers;

    struct BufferView
    {
      BufferIndex buffer;
      uint32_t byteOffset;
      uint32_t byteLength;
      uint32_t byteStride;
    };
    std::vector<BufferView> bufferViews;

    struct MeshPrimitive
    {
      enum class Mode : uint32_t
      {
        Points = 0,
        Lines = 1,
        LineLoop = 2,
        LineStrip = 3,
        Triangles = 4,
        TriangleStrip = 5,
        TriangleFan = 6,
      };
      std::unordered_map<std::string, AccessorIndex> attributes;
      std::optional<AccessorIndex> indices;
      Mode mode;
    };

    struct Mesh
    {
      std::vector<MeshPrimitive> primitives;
      std::optional<std::string> name;
    };
    std::vector<Mesh> meshes;
    ObjectsByName meshesByName;

    struct Node
    {
      std::vector<NodeIndex> children;
      std::optional<MeshIndex> mesh;
      vec3 translation;
      quat rotation;
      vec3 scale;
      std::optional<std::string> name;
    };
    std::vector<Node> nodes;
    ObjectsByName nodesByName;

    struct Scene
    {
      std::vector<NodeIndex> nodes;
      std::optional<std::string> name;
    };
    std::vector<Scene> scenes;
    ObjectsByName scenesByName;
  };

  GLTF ReadGLTF(InputStream& stream);
  GLTF ReadBinaryGLTF(InputStream& stream);
}
