#include "asset_gltf.hpp"
#include "data.hpp"
#include "json.hpp"

namespace
{
  using namespace Coil;

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
}

namespace Coil
{
  template <>
  struct JsonDecoder<GLTF::Accessor> : public JsonDecoderBase<GLTF::Accessor>
  {
    static GLTF::Accessor Decode(json const& j)
    {
      return
      {
        .bufferView = JsonDecoder<std::optional<GLTF::BufferViewIndex>>::DecodeField(j, "bufferView", {}),
        .byteOffset = JsonDecoder<uint32_t>::DecodeField(j, "byteOffset", 0),
        .componentType = JsonDecoder<uint32_t>::DecodeField(j, "componentType"),
        .normalized = JsonDecoder<bool>::DecodeField(j, "normalized", false),
        .count = JsonDecoder<uint32_t>::DecodeField(j, "count"),
        .type = JsonDecoder<std::string>::DecodeField(j, "type"),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Buffer> : public JsonDecoderBase<GLTF::Buffer>
  {
    static GLTF::Buffer Decode(json const& j)
    {
      return
      {
        .uri = JsonDecoder<std::string>::DecodeField(j, "uri", {}),
        .byteLength = JsonDecoder<uint32_t>::DecodeField(j, "byteLength"),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::BufferView> : public JsonDecoderBase<GLTF::BufferView>
  {
    static GLTF::BufferView Decode(json const& j)
    {
      return
      {
        .buffer = JsonDecoder<GLTF::BufferIndex>::DecodeField(j, "buffer"),
        .byteOffset = JsonDecoder<uint32_t>::DecodeField(j, "byteOffset", 0),
        .byteLength = JsonDecoder<uint32_t>::DecodeField(j, "byteLength"),
        .byteStride = JsonDecoder<std::optional<uint32_t>>::DecodeField(j, "byteStride", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Image> : public JsonDecoderBase<GLTF::Image>
  {
    static GLTF::Image Decode(json const& j)
    {
      return
      {
        .uri = JsonDecoder<std::optional<std::string>>::DecodeField(j, "uri", {}),
        .mimeType = JsonDecoder<std::optional<std::string>>::DecodeField(j, "mimeType", {}),
        .bufferView = JsonDecoder<std::optional<GLTF::BufferViewIndex>>::DecodeField(j, "bufferView", {}),
        .name = JsonDecoder<std::optional<std::string>>::DecodeField(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Material> : public JsonDecoderBase<GLTF::Material>
  {
    static GLTF::Material Decode(json const& j)
    {
      return
      {
        .name = JsonDecoder<std::optional<std::string>>::DecodeField(j, "name"),
        .pbrMetallicRoughness = JsonDecoder<std::optional<GLTF::Material::PbrMetallicRoughness>>::DecodeField(j, "pbrMetallicRoughness", {}),
        .normalTexture = JsonDecoder<std::optional<GLTF::Material::NormalTextureInfo>>::DecodeField(j, "normalTexture", {}),
        .occlusionTexture = JsonDecoder<std::optional<GLTF::Material::OcclusionTextureInfo>>::DecodeField(j, "occlusionTexture", {}),
        .emissiveTexture = JsonDecoder<std::optional<GLTF::Material::TextureInfo>>::DecodeField(j, "emissiveTexture", {}),
        .emissiveFactor = JsonDecoder<vec3>::DecodeField(j, "emissiveFactor", vec3(0, 0, 0)),
        .alphaMode = JsonDecoder<std::string>::DecodeField(j, "alphaMode", "OPAQUE"),
        .alphaCutoff = JsonDecoder<float>::DecodeField(j, "alphaCutoff", 0.5f),
        .doubleSided = JsonDecoder<bool>::DecodeField(j, "doubleSided", false),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Material::TextureInfo> : public JsonDecoderBase<GLTF::Material::TextureInfo>
  {
    static GLTF::Material::TextureInfo Decode(json const& j)
    {
      return
      {
        .index = JsonDecoder<GLTF::TextureIndex>::DecodeField(j, "index"),
        .texCoord = JsonDecoder<uint32_t>::DecodeField(j, "texCoord", 0),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Material::NormalTextureInfo> : public JsonDecoderBase<GLTF::Material::NormalTextureInfo>
  {
    static GLTF::Material::NormalTextureInfo Decode(json const& j)
    {
      return
      {
        .index = JsonDecoder<GLTF::TextureIndex>::DecodeField(j, "index"),
        .texCoord = JsonDecoder<uint32_t>::DecodeField(j, "texCoord", 0),
        .scale = JsonDecoder<float>::DecodeField(j, "scale", 1),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Material::OcclusionTextureInfo> : public JsonDecoderBase<GLTF::Material::OcclusionTextureInfo>
  {
    static GLTF::Material::OcclusionTextureInfo Decode(json const& j)
    {
      return
      {
        .index = JsonDecoder<GLTF::TextureIndex>::DecodeField(j, "index"),
        .texCoord = JsonDecoder<uint32_t>::DecodeField(j, "texCoord", 0),
        .strength = JsonDecoder<float>::DecodeField(j, "strength", 1),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Material::PbrMetallicRoughness> : public JsonDecoderBase<GLTF::Material::PbrMetallicRoughness>
  {
    static GLTF::Material::PbrMetallicRoughness Decode(json const& j)
    {
      return
      {
        .baseColorFactor = JsonDecoder<vec4>::DecodeField(j, "baseColorFactor", vec4(1, 1, 1, 1)),
        .baseColorTexture = JsonDecoder<std::optional<GLTF::Material::TextureInfo>>::DecodeField(j, "baseColorTexture", {}),
        .metallicFactor = JsonDecoder<float>::DecodeField(j, "metallicFactor", 1),
        .roughnessFactor = JsonDecoder<float>::DecodeField(j, "roughnessFactor", 1),
        .metallicRoughnessTexture = JsonDecoder<std::optional<GLTF::Material::TextureInfo>>::DecodeField(j, "metallicRoughnessTexture", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Mesh> : public JsonDecoderBase<GLTF::Mesh>
  {
    static GLTF::Mesh Decode(json const& j)
    {
      return
      {
        .primitives = JsonDecoder<std::vector<GLTF::Mesh::MeshPrimitive>>::DecodeField(j, "primitives"),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Mesh::MeshPrimitive> : public JsonDecoderBase<GLTF::Mesh::MeshPrimitive>
  {
    static GLTF::Mesh::MeshPrimitive Decode(json const& j)
    {
      return
      {
        .attributes = j.at("attributes").get<std::unordered_map<std::string, GLTF::AccessorIndex>>(),
        .indices = JsonDecoder<std::optional<GLTF::AccessorIndex>>::DecodeField(j, "indices", {}),
        .material = JsonDecoder<std::optional<GLTF::MaterialIndex>>::DecodeField(j, "material", {}),
        .mode = (GLTF::Mesh::MeshPrimitive::Mode)JsonDecoder<uint32_t>::DecodeField(j, "mode", 4),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Node> : public JsonDecoderBase<GLTF::Node>
  {
    static GLTF::Node Decode(json const& j)
    {
      return
      {
        .children = JsonDecoder<std::vector<GLTF::NodeIndex>>::DecodeField(j, "children", {}),
        .mesh = JsonDecoder<std::optional<GLTF::MeshIndex>>::DecodeField(j, "mesh", {}),
        .translation = JsonDecoder<vec3>::DecodeField(j, "translation", {}),
        .rotation = JsonDecoder<quat>::DecodeField(j, "rotation", {}),
        .scale = JsonDecoder<vec3>::DecodeField(j, "scale", { 1, 1, 1 }),
        .name = JsonDecoder<std::optional<std::string>>::DecodeField(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Sampler> : public JsonDecoderBase<GLTF::Sampler>
  {
    static GLTF::Sampler Decode(json const& j)
    {
      return
      {
        .magFilter = JsonDecoder<uint32_t>::DecodeField(j, "magFilter", 0),
        .minFilter = JsonDecoder<uint32_t>::DecodeField(j, "minFilter", 0),
        .wrapS = JsonDecoder<uint32_t>::DecodeField(j, "wrapS", 10497),
        .wrapT = JsonDecoder<uint32_t>::DecodeField(j, "wrapT", 10497),
        .name = JsonDecoder<std::optional<std::string>>::DecodeField(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Scene> : public JsonDecoderBase<GLTF::Scene>
  {
    static GLTF::Scene Decode(json const& j)
    {
      return
      {
        .nodes = JsonDecoder<std::vector<GLTF::NodeIndex>>::DecodeField(j, "nodes", {}),
        .name = JsonDecoder<std::optional<std::string>>::DecodeField(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonDecoder<GLTF::Texture> : public JsonDecoderBase<GLTF::Texture>
  {
    static GLTF::Texture Decode(json const& j)
    {
      return
      {
        .sampler = JsonDecoder<std::optional<GLTF::SamplerIndex>>::DecodeField(j, "sampler", {}),
        .source = JsonDecoder<std::optional<GLTF::ImageIndex>>::DecodeField(j, "source", {}),
        .name = JsonDecoder<std::optional<std::string>>::DecodeField(j, "name", {}),
      };
    }
  };

  template <>
  struct JsonDecoder<GLTF> : public JsonDecoderBase<GLTF>
  {
    static GLTF Decode(json const& j)
    {
      auto meshes = JsonDecoder<std::vector<GLTF::Mesh>>::DecodeField(j, "meshes", {});
      auto meshesByName = IndexByName(meshes);
      auto nodes = JsonDecoder<std::vector<GLTF::Node>>::DecodeField(j, "nodes", {});
      auto nodesByName = IndexByName(nodes);
      auto scenes = JsonDecoder<std::vector<GLTF::Scene>>::DecodeField(j, "scenes", {});
      auto scenesByName = IndexByName(scenes);
      return
      {
        .accessors = JsonDecoder<std::vector<GLTF::Accessor>>::DecodeField(j, "accessors", {}),
        .buffers = JsonDecoder<std::vector<GLTF::Buffer>>::DecodeField(j, "buffers", {}),
        .bufferViews = JsonDecoder<std::vector<GLTF::BufferView>>::DecodeField(j, "bufferViews", {}),
        .images = JsonDecoder<std::vector<GLTF::Image>>::DecodeField(j, "images", {}),
        .materials = JsonDecoder<std::vector<GLTF::Material>>::DecodeField(j, "materials", {}),
        .meshes = std::move(meshes),
        .meshesByName = std::move(meshesByName),
        .nodes = std::move(nodes),
        .nodesByName = std::move(nodesByName),
        .samplers = JsonDecoder<std::vector<GLTF::Sampler>>::DecodeField(j, "samplers", {}),
        .scenes = std::move(scenes),
        .scenesByName = std::move(scenesByName),
        .textures = JsonDecoder<std::vector<GLTF::Texture>>::DecodeField(j, "textures", {}),
      };
    }
  };

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
    uint32_t totalLength = reader.ReadLE<uint32_t>();

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
    json const jsonRoot = ParseJsonBuffer(jsonChunk);
    GLTF gltf = JsonDecoder<GLTF>::Decode(jsonRoot);

    // check if there should be binary chunk
    bool shouldBeBinaryChunk = false;
    try
    {
      shouldBeBinaryChunk = jsonRoot.at("/buffers/0/uri"_json_pointer).is_null();
    }
    catch(json::exception const&)
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
