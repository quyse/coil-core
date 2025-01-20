module;

#include "json.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

export module coil.core.gltf;

import coil.core.base;
import coil.core.data;
import coil.core.math;
import coil.core.json;

export namespace Coil
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
}

namespace Coil
{
  template <>
  struct JsonDecoder<GLTF::Accessor> : public JsonDecoderBase<GLTF::Accessor>
  {
    static GLTF::Accessor Decode(Json const& j)
    {
      return
      {
        .bufferView = JsonDecodeField<std::optional<GLTF::BufferViewIndex>>(j, "bufferView", {}),
        .byteOffset = JsonDecodeField<uint32_t>(j, "byteOffset", 0),
        .componentType = JsonDecodeField<uint32_t>(j, "componentType"),
        .normalized = JsonDecodeField<bool>(j, "normalized", false),
        .count = JsonDecodeField<uint32_t>(j, "count"),
        .type = JsonDecodeField<std::string>(j, "type"),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Buffer> : public JsonDecoderBase<GLTF::Buffer>
  {
    static GLTF::Buffer Decode(Json const& j)
    {
      return
      {
        .uri = JsonDecodeField<std::string>(j, "uri", {}),
        .byteLength = JsonDecodeField<uint32_t>(j, "byteLength"),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::BufferView> : public JsonDecoderBase<GLTF::BufferView>
  {
    static GLTF::BufferView Decode(Json const& j)
    {
      return
      {
        .buffer = JsonDecodeField<GLTF::BufferIndex>(j, "buffer"),
        .byteOffset = JsonDecodeField<uint32_t>(j, "byteOffset", 0),
        .byteLength = JsonDecodeField<uint32_t>(j, "byteLength"),
        .byteStride = JsonDecodeField<std::optional<uint32_t>>(j, "byteStride", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Image> : public JsonDecoderBase<GLTF::Image>
  {
    static GLTF::Image Decode(Json const& j)
    {
      return
      {
        .uri = JsonDecodeField<std::optional<std::string>>(j, "uri", {}),
        .mimeType = JsonDecodeField<std::optional<std::string>>(j, "mimeType", {}),
        .bufferView = JsonDecodeField<std::optional<GLTF::BufferViewIndex>>(j, "bufferView", {}),
        .name = JsonDecodeField<std::optional<std::string>>(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Material> : public JsonDecoderBase<GLTF::Material>
  {
    static GLTF::Material Decode(Json const& j)
    {
      return
      {
        .name = JsonDecodeField<std::optional<std::string>>(j, "name"),
        .pbrMetallicRoughness = JsonDecodeField<std::optional<GLTF::Material::PbrMetallicRoughness>>(j, "pbrMetallicRoughness", {}),
        .normalTexture = JsonDecodeField<std::optional<GLTF::Material::NormalTextureInfo>>(j, "normalTexture", {}),
        .occlusionTexture = JsonDecodeField<std::optional<GLTF::Material::OcclusionTextureInfo>>(j, "occlusionTexture", {}),
        .emissiveTexture = JsonDecodeField<std::optional<GLTF::Material::TextureInfo>>(j, "emissiveTexture", {}),
        .emissiveFactor = JsonDecodeField<vec3>(j, "emissiveFactor", vec3(0, 0, 0)),
        .alphaMode = JsonDecodeField<std::string>(j, "alphaMode", "OPAQUE"),
        .alphaCutoff = JsonDecodeField<float>(j, "alphaCutoff", 0.5f),
        .doubleSided = JsonDecodeField<bool>(j, "doubleSided", false),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Material::TextureInfo> : public JsonDecoderBase<GLTF::Material::TextureInfo>
  {
    static GLTF::Material::TextureInfo Decode(Json const& j)
    {
      return
      {
        .index = JsonDecodeField<GLTF::TextureIndex>(j, "index"),
        .texCoord = JsonDecodeField<uint32_t>(j, "texCoord", 0),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Material::NormalTextureInfo> : public JsonDecoderBase<GLTF::Material::NormalTextureInfo>
  {
    static GLTF::Material::NormalTextureInfo Decode(Json const& j)
    {
      return
      {
        .index = JsonDecodeField<GLTF::TextureIndex>(j, "index"),
        .texCoord = JsonDecodeField<uint32_t>(j, "texCoord", 0),
        .scale = JsonDecodeField<float>(j, "scale", 1),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Material::OcclusionTextureInfo> : public JsonDecoderBase<GLTF::Material::OcclusionTextureInfo>
  {
    static GLTF::Material::OcclusionTextureInfo Decode(Json const& j)
    {
      return
      {
        .index = JsonDecodeField<GLTF::TextureIndex>(j, "index"),
        .texCoord = JsonDecodeField<uint32_t>(j, "texCoord", 0),
        .strength = JsonDecodeField<float>(j, "strength", 1),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Material::PbrMetallicRoughness> : public JsonDecoderBase<GLTF::Material::PbrMetallicRoughness>
  {
    static GLTF::Material::PbrMetallicRoughness Decode(Json const& j)
    {
      return
      {
        .baseColorFactor = JsonDecodeField<vec4>(j, "baseColorFactor", vec4(1, 1, 1, 1)),
        .baseColorTexture = JsonDecodeField<std::optional<GLTF::Material::TextureInfo>>(j, "baseColorTexture", {}),
        .metallicFactor = JsonDecodeField<float>(j, "metallicFactor", 1),
        .roughnessFactor = JsonDecodeField<float>(j, "roughnessFactor", 1),
        .metallicRoughnessTexture = JsonDecodeField<std::optional<GLTF::Material::TextureInfo>>(j, "metallicRoughnessTexture", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Mesh> : public JsonDecoderBase<GLTF::Mesh>
  {
    static GLTF::Mesh Decode(Json const& j)
    {
      return
      {
        .primitives = JsonDecodeField<std::vector<GLTF::Mesh::MeshPrimitive>>(j, "primitives"),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Mesh::MeshPrimitive> : public JsonDecoderBase<GLTF::Mesh::MeshPrimitive>
  {
    static GLTF::Mesh::MeshPrimitive Decode(Json const& j)
    {
      return
      {
        .attributes = j.at("attributes").get<std::unordered_map<std::string, GLTF::AccessorIndex>>(),
        .indices = JsonDecodeField<std::optional<GLTF::AccessorIndex>>(j, "indices", {}),
        .material = JsonDecodeField<std::optional<GLTF::MaterialIndex>>(j, "material", {}),
        .mode = (GLTF::Mesh::MeshPrimitive::Mode)JsonDecodeField<uint32_t>(j, "mode", 4),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Node> : public JsonDecoderBase<GLTF::Node>
  {
    static GLTF::Node Decode(Json const& j)
    {
      return
      {
        .children = JsonDecodeField<std::vector<GLTF::NodeIndex>>(j, "children", {}),
        .mesh = JsonDecodeField<std::optional<GLTF::MeshIndex>>(j, "mesh", {}),
        .translation = JsonDecodeField<vec3>(j, "translation", {}),
        .rotation = JsonDecodeField<quat>(j, "rotation", {}),
        .scale = JsonDecodeField<vec3>(j, "scale", { 1, 1, 1 }),
        .name = JsonDecodeField<std::optional<std::string>>(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Sampler> : public JsonDecoderBase<GLTF::Sampler>
  {
    static GLTF::Sampler Decode(Json const& j)
    {
      return
      {
        .magFilter = JsonDecodeField<uint32_t>(j, "magFilter", 0),
        .minFilter = JsonDecodeField<uint32_t>(j, "minFilter", 0),
        .wrapS = JsonDecodeField<uint32_t>(j, "wrapS", 10497),
        .wrapT = JsonDecodeField<uint32_t>(j, "wrapT", 10497),
        .name = JsonDecodeField<std::optional<std::string>>(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Scene> : public JsonDecoderBase<GLTF::Scene>
  {
    static GLTF::Scene Decode(Json const& j)
    {
      return
      {
        .nodes = JsonDecodeField<std::vector<GLTF::NodeIndex>>(j, "nodes", {}),
        .name = JsonDecodeField<std::optional<std::string>>(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Texture> : public JsonDecoderBase<GLTF::Texture>
  {
    static GLTF::Texture Decode(Json const& j)
    {
      return
      {
        .sampler = JsonDecodeField<std::optional<GLTF::SamplerIndex>>(j, "sampler", {}),
        .source = JsonDecodeField<std::optional<GLTF::ImageIndex>>(j, "source", {}),
        .name = JsonDecodeField<std::optional<std::string>>(j, "name", {}),
      };
    }
  };

  template <typename T>
  GLTF::ObjectsByName IndexByName(std::vector<T> const& objects)
  {
    GLTF::ObjectsByName objectsByName;
    for(size_t i = 0; i < objects.size(); ++i)
    {
      T const& object = objects[i];
      if(object.name.has_value())
      {
        objectsByName.insert({ object.name.value(), i });
      }
    }
    return std::move(objectsByName);
  }

  template <>
  struct JsonDecoder<GLTF> : public JsonDecoderBase<GLTF>
  {
    static GLTF Decode(Json const& j)
    {
      auto meshes = JsonDecodeField<std::vector<GLTF::Mesh>>(j, "meshes", {});
      auto meshesByName = IndexByName(meshes);
      auto nodes = JsonDecodeField<std::vector<GLTF::Node>>(j, "nodes", {});
      auto nodesByName = IndexByName(nodes);
      auto scenes = JsonDecodeField<std::vector<GLTF::Scene>>(j, "scenes", {});
      auto scenesByName = IndexByName(scenes);
      return
      {
        .accessors = JsonDecodeField<std::vector<GLTF::Accessor>>(j, "accessors", {}),
        .buffers = JsonDecodeField<std::vector<GLTF::Buffer>>(j, "buffers", {}),
        .bufferViews = JsonDecodeField<std::vector<GLTF::BufferView>>(j, "bufferViews", {}),
        .images = JsonDecodeField<std::vector<GLTF::Image>>(j, "images", {}),
        .materials = JsonDecodeField<std::vector<GLTF::Material>>(j, "materials", {}),
        .meshes = std::move(meshes),
        .meshesByName = std::move(meshesByName),
        .nodes = std::move(nodes),
        .nodesByName = std::move(nodesByName),
        .samplers = JsonDecodeField<std::vector<GLTF::Sampler>>(j, "samplers", {}),
        .scenes = std::move(scenes),
        .scenesByName = std::move(scenesByName),
        .textures = JsonDecodeField<std::vector<GLTF::Texture>>(j, "textures", {}),
      };
    }
  };
}

export namespace Coil
{
  GLTF ReadBinaryGLTF(InputStream& stream)
  {
    StreamReader reader(stream);

    // read header
    // magic
    if(reader.ReadLE<uint32_t>() != 0x46546C67 /* glTF */)
      throw Exception("wrong glTF magic");
    // version
    if(reader.ReadLE<uint32_t>() != 2)
      throw Exception("glTF version must be 2");
    // total length
    uint32_t const totalLength = reader.ReadLE<uint32_t>();

    auto readChunk = [&](uint32_t chunkType) -> std::vector<uint8_t>
    {
      // align
      reader.ReadGap<4>();

      // read header
      uint32_t chunkLength = reader.ReadLE<uint32_t>();
      if(reader.ReadLE<uint32_t>() != chunkType)
        throw Exception("wrong binary glTF chunk type");
      // read data
      std::vector<uint8_t> chunkData(chunkLength);
      reader.Read(chunkData.data(), chunkData.size());

      return std::move(chunkData);
    };

    // read JSON chunk
    std::vector<uint8_t> jsonChunk = readChunk(0x4E4F534A /* JSON */);
    // parse & decode JSON
    Json const jsonRoot = JsonFromBuffer(jsonChunk);
    GLTF gltf = JsonDecoder<GLTF>::Decode(jsonRoot);

    // check if there should be binary chunk
    bool shouldBeBinaryChunk = false;
    try
    {
      shouldBeBinaryChunk = jsonRoot.at("/buffers/0/uri"_json_pointer).is_null();
    }
    catch(Json::exception const&)
    {
      shouldBeBinaryChunk = true;
    }

    // read binary chunk if it exists
    if(shouldBeBinaryChunk)
    {
      gltf.buffers[0].data = readChunk(0x004E4942 /* BIN\0 */);
    }

    return std::move(gltf);
  }
}
