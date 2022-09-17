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
  struct JsonParser<GLTF::Accessor> : public JsonParserBase<GLTF::Accessor>
  {
    static GLTF::Accessor Parse(json const& j)
    {
      return
      {
        .bufferView = JsonParser<std::optional<GLTF::BufferViewIndex>>::ParseField(j, "bufferView", {}),
        .byteOffset = JsonParser<uint32_t>::ParseField(j, "byteOffset", 0),
        .componentType = JsonParser<uint32_t>::ParseField(j, "componentType"),
        .normalized = JsonParser<bool>::ParseField(j, "normalized", false),
        .count = JsonParser<uint32_t>::ParseField(j, "count"),
        .type = JsonParser<std::string>::ParseField(j, "type"),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Buffer> : public JsonParserBase<GLTF::Buffer>
  {
    static GLTF::Buffer Parse(json const& j)
    {
      return
      {
        .uri = JsonParser<std::string>::ParseField(j, "uri", {}),
        .byteLength = JsonParser<uint32_t>::ParseField(j, "byteLength"),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::BufferView> : public JsonParserBase<GLTF::BufferView>
  {
    static GLTF::BufferView Parse(json const& j)
    {
      return
      {
        .buffer = JsonParser<GLTF::BufferIndex>::ParseField(j, "buffer"),
        .byteOffset = JsonParser<uint32_t>::ParseField(j, "byteOffset", 0),
        .byteLength = JsonParser<uint32_t>::ParseField(j, "byteLength"),
        .byteStride = JsonParser<std::optional<uint32_t>>::ParseField(j, "byteStride", {}),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Image> : public JsonParserBase<GLTF::Image>
  {
    static GLTF::Image Parse(json const& j)
    {
      return
      {
        .uri = JsonParser<std::optional<std::string>>::ParseField(j, "uri", {}),
        .mimeType = JsonParser<std::optional<std::string>>::ParseField(j, "mimeType", {}),
        .bufferView = JsonParser<std::optional<GLTF::BufferViewIndex>>::ParseField(j, "bufferView", {}),
        .name = JsonParser<std::optional<std::string>>::ParseField(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Material> : public JsonParserBase<GLTF::Material>
  {
    static GLTF::Material Parse(json const& j)
    {
      return
      {
        .name = JsonParser<std::optional<std::string>>::ParseField(j, "name"),
        .pbrMetallicRoughness = JsonParser<std::optional<GLTF::Material::PbrMetallicRoughness>>::ParseField(j, "pbrMetallicRoughness", {}),
        .normalTexture = JsonParser<std::optional<GLTF::Material::NormalTextureInfo>>::ParseField(j, "normalTexture", {}),
        .occlusionTexture = JsonParser<std::optional<GLTF::Material::OcclusionTextureInfo>>::ParseField(j, "occlusionTexture", {}),
        .emissiveTexture = JsonParser<std::optional<GLTF::Material::TextureInfo>>::ParseField(j, "emissiveTexture", {}),
        .emissiveFactor = JsonParser<vec3>::ParseField(j, "emissiveFactor", vec3(0, 0, 0)),
        .alphaMode = JsonParser<std::string>::ParseField(j, "alphaMode", "OPAQUE"),
        .alphaCutoff = JsonParser<float>::ParseField(j, "alphaCutoff", 0.5f),
        .doubleSided = JsonParser<bool>::ParseField(j, "doubleSided", false),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Material::TextureInfo> : public JsonParserBase<GLTF::Material::TextureInfo>
  {
    static GLTF::Material::TextureInfo Parse(json const& j)
    {
      return
      {
        .index = JsonParser<GLTF::TextureIndex>::ParseField(j, "index"),
        .texCoord = JsonParser<uint32_t>::ParseField(j, "texCoord", 0),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Material::NormalTextureInfo> : public JsonParserBase<GLTF::Material::NormalTextureInfo>
  {
    static GLTF::Material::NormalTextureInfo Parse(json const& j)
    {
      return
      {
        .index = JsonParser<GLTF::TextureIndex>::ParseField(j, "index"),
        .texCoord = JsonParser<uint32_t>::ParseField(j, "texCoord", 0),
        .scale = JsonParser<float>::ParseField(j, "scale", 1),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Material::OcclusionTextureInfo> : public JsonParserBase<GLTF::Material::OcclusionTextureInfo>
  {
    static GLTF::Material::OcclusionTextureInfo Parse(json const& j)
    {
      return
      {
        .index = JsonParser<GLTF::TextureIndex>::ParseField(j, "index"),
        .texCoord = JsonParser<uint32_t>::ParseField(j, "texCoord", 0),
        .strength = JsonParser<float>::ParseField(j, "strength", 1),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Material::PbrMetallicRoughness> : public JsonParserBase<GLTF::Material::PbrMetallicRoughness>
  {
    static GLTF::Material::PbrMetallicRoughness Parse(json const& j)
    {
      return
      {
        .baseColorFactor = JsonParser<vec4>::ParseField(j, "baseColorFactor", vec4(1, 1, 1, 1)),
        .baseColorTexture = JsonParser<std::optional<GLTF::Material::TextureInfo>>::ParseField(j, "baseColorTexture", {}),
        .metallicFactor = JsonParser<float>::ParseField(j, "metallicFactor", 1),
        .roughnessFactor = JsonParser<float>::ParseField(j, "roughnessFactor", 1),
        .metallicRoughnessTexture = JsonParser<std::optional<GLTF::Material::TextureInfo>>::ParseField(j, "metallicRoughnessTexture", {}),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Mesh> : public JsonParserBase<GLTF::Mesh>
  {
    static GLTF::Mesh Parse(json const& j)
    {
      return
      {
        .primitives = JsonParser<std::vector<GLTF::Mesh::MeshPrimitive>>::ParseField(j, "primitives"),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Mesh::MeshPrimitive> : public JsonParserBase<GLTF::Mesh::MeshPrimitive>
  {
    static GLTF::Mesh::MeshPrimitive Parse(json const& j)
    {
      return
      {
        .attributes = j.at("attributes").get<std::unordered_map<std::string, GLTF::AccessorIndex>>(),
        .indices = JsonParser<std::optional<GLTF::AccessorIndex>>::ParseField(j, "indices", {}),
        .material = JsonParser<std::optional<GLTF::MaterialIndex>>::ParseField(j, "material", {}),
        .mode = (GLTF::Mesh::MeshPrimitive::Mode)JsonParser<uint32_t>::ParseField(j, "mode", 4),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Node> : public JsonParserBase<GLTF::Node>
  {
    static GLTF::Node Parse(json const& j)
    {
      return
      {
        .children = JsonParser<std::vector<GLTF::NodeIndex>>::ParseField(j, "children", {}),
        .mesh = JsonParser<std::optional<GLTF::MeshIndex>>::ParseField(j, "mesh", {}),
        .translation = JsonParser<vec3>::ParseField(j, "translation", {}),
        .rotation = JsonParser<quat>::ParseField(j, "rotation", {}),
        .scale = JsonParser<vec3>::ParseField(j, "scale", { 1, 1, 1 }),
        .name = JsonParser<std::optional<std::string>>::ParseField(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Sampler> : public JsonParserBase<GLTF::Sampler>
  {
    static GLTF::Sampler Parse(json const& j)
    {
      return
      {
        .magFilter = JsonParser<uint32_t>::ParseField(j, "magFilter", 0),
        .minFilter = JsonParser<uint32_t>::ParseField(j, "minFilter", 0),
        .wrapS = JsonParser<uint32_t>::ParseField(j, "wrapS", 10497),
        .wrapT = JsonParser<uint32_t>::ParseField(j, "wrapT", 10497),
        .name = JsonParser<std::optional<std::string>>::ParseField(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Scene> : public JsonParserBase<GLTF::Scene>
  {
    static GLTF::Scene Parse(json const& j)
    {
      return
      {
        .nodes = JsonParser<std::vector<GLTF::NodeIndex>>::ParseField(j, "nodes", {}),
        .name = JsonParser<std::optional<std::string>>::ParseField(j, "name", {}),
      };
    }
  };
  template <>
  struct JsonParser<GLTF::Texture> : public JsonParserBase<GLTF::Texture>
  {
    static GLTF::Texture Parse(json const& j)
    {
      return
      {
        .sampler = JsonParser<std::optional<GLTF::SamplerIndex>>::ParseField(j, "sampler", {}),
        .source = JsonParser<std::optional<GLTF::ImageIndex>>::ParseField(j, "source", {}),
        .name = JsonParser<std::optional<std::string>>::ParseField(j, "name", {}),
      };
    }
  };

  template <>
  struct JsonParser<GLTF> : public JsonParserBase<GLTF>
  {
    static GLTF Parse(json const& j)
    {
      auto meshes = JsonParser<std::vector<GLTF::Mesh>>::ParseField(j, "meshes", {});
      auto meshesByName = IndexByName(meshes);
      auto nodes = JsonParser<std::vector<GLTF::Node>>::ParseField(j, "nodes", {});
      auto nodesByName = IndexByName(nodes);
      auto scenes = JsonParser<std::vector<GLTF::Scene>>::ParseField(j, "scenes", {});
      auto scenesByName = IndexByName(scenes);
      return
      {
        .accessors = JsonParser<std::vector<GLTF::Accessor>>::ParseField(j, "accessors", {}),
        .buffers = JsonParser<std::vector<GLTF::Buffer>>::ParseField(j, "buffers", {}),
        .bufferViews = JsonParser<std::vector<GLTF::BufferView>>::ParseField(j, "bufferViews", {}),
        .images = JsonParser<std::vector<GLTF::Image>>::ParseField(j, "images", {}),
        .materials = JsonParser<std::vector<GLTF::Material>>::ParseField(j, "materials", {}),
        .meshes = std::move(meshes),
        .meshesByName = std::move(meshesByName),
        .nodes = std::move(nodes),
        .nodesByName = std::move(nodesByName),
        .samplers = JsonParser<std::vector<GLTF::Sampler>>::ParseField(j, "samplers", {}),
        .scenes = std::move(scenes),
        .scenesByName = std::move(scenesByName),
        .textures = JsonParser<std::vector<GLTF::Texture>>::ParseField(j, "textures", {}),
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
    // parse JSON
    json const jsonRoot = [&]()
    {
      try
      {
        return json::parse(jsonChunk.begin(), jsonChunk.end());
      }
      catch(json::exception const&)
      {
        throw Exception("invalid glTF JSON");
      }
    }();

    GLTF gltf = JsonParser<GLTF>::Parse(jsonRoot);

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
