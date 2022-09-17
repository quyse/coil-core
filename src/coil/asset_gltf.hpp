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
    using ImageIndex = uint32_t;
    using MaterialIndex = uint32_t;
    using MeshIndex = uint32_t;
    using NodeIndex = uint32_t;
    using SamplerIndex = uint32_t;
    using SceneIndex = uint32_t;
    using TextureIndex = uint32_t;

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
      std::optional<uint32_t> byteStride;
    };
    std::vector<BufferView> bufferViews;

    struct Image
    {
      std::optional<std::string> uri;
      std::optional<std::string> mimeType;
      std::optional<BufferViewIndex> bufferView;
      std::optional<std::string> name;
    };
    std::vector<Image> images;

    struct Material
    {
      struct TextureInfo
      {
        TextureIndex index;
        uint32_t texCoord;
      };
      struct NormalTextureInfo
      {
        TextureIndex index;
        uint32_t texCoord;
        float scale;
      };
      struct OcclusionTextureInfo
      {
        TextureIndex index;
        uint32_t texCoord;
        float strength;
      };
      struct PbrMetallicRoughness
      {
        vec4 baseColorFactor;
        std::optional<TextureInfo> baseColorTexture;
        float metallicFactor;
        float roughnessFactor;
        std::optional<TextureInfo> metallicRoughnessTexture;
      };

      std::optional<std::string> name;
      std::optional<PbrMetallicRoughness> pbrMetallicRoughness;
      std::optional<NormalTextureInfo> normalTexture;
      std::optional<OcclusionTextureInfo> occlusionTexture;
      std::optional<TextureInfo> emissiveTexture;
      vec3 emissiveFactor;
      std::string alphaMode;
      float alphaCutoff;
      bool doubleSided;
    };
    std::vector<Material> materials;

    struct Mesh
    {
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
        std::optional<MaterialIndex> material;
        Mode mode;
      };

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

    struct Sampler
    {
      uint32_t magFilter;
      uint32_t minFilter;
      uint32_t wrapS;
      uint32_t wrapT;
      std::optional<std::string> name;
    };
    std::vector<Sampler> samplers;

    struct Scene
    {
      std::vector<NodeIndex> nodes;
      std::optional<std::string> name;
    };
    std::vector<Scene> scenes;
    ObjectsByName scenesByName;

    struct Texture
    {
      std::optional<SamplerIndex> sampler;
      std::optional<ImageIndex> source;
      std::optional<std::string> name;
    };
    std::vector<Texture> textures;
  };

  GLTF ReadGLTF(InputStream& stream);
  GLTF ReadBinaryGLTF(InputStream& stream);
}
