#include "graphics.hpp"

namespace Coil
{
  GraphicsPassConfig::AttachmentRef::AttachmentRef(GraphicsPassConfig& config, AttachmentId id)
  : _config(config), _id(id) {}
  GraphicsPassConfig::AttachmentId GraphicsPassConfig::AttachmentRef::GetId() const
  {
    return _id;
  }
  GraphicsPassConfig::Attachment& GraphicsPassConfig::AttachmentRef::operator*()
  {
    return _config.attachments[_id];
  }
  GraphicsPassConfig::Attachment const& GraphicsPassConfig::AttachmentRef::operator*() const
  {
    return _config.attachments[_id];
  }
  GraphicsPassConfig::Attachment* GraphicsPassConfig::AttachmentRef::operator->()
  {
    return &_config.attachments[_id];
  }
  GraphicsPassConfig::Attachment const* GraphicsPassConfig::AttachmentRef::operator->() const
  {
    return &_config.attachments[_id];
  }

  GraphicsPassConfig::SubPassRef::SubPassRef(GraphicsPassConfig& config, GraphicsSubPassId id)
  : _config(config), _id(id) {}
  GraphicsPassConfig::SubPass& GraphicsPassConfig::SubPassRef::operator*()
  {
    return _config.subPasses[_id];
  }
  GraphicsPassConfig::SubPass const& GraphicsPassConfig::SubPassRef::operator*() const
  {
    return _config.subPasses[_id];
  }
  GraphicsPassConfig::SubPass* GraphicsPassConfig::SubPassRef::operator->()
  {
    return &_config.subPasses[_id];
  }
  GraphicsPassConfig::SubPass const* GraphicsPassConfig::SubPassRef::operator->() const
  {
    return &_config.subPasses[_id];
  }

  GraphicsPassConfig::AttachmentRef GraphicsPassConfig::AddAttachment(AttachmentConfig const& config)
  {
    AttachmentId attachmentId = (AttachmentId)attachments.size();
    attachments.push_back(
    {
      .config = config,
    });
    return { *this, attachmentId };
  }

  GraphicsPassConfig::SubPassRef GraphicsPassConfig::AddSubPass()
  {
    GraphicsSubPassId subPassId = (GraphicsSubPassId)subPasses.size();
    subPasses.push_back({});
    return { *this, subPassId };
  }

  void GraphicsPassConfig::SubPass::UseColorAttachment(AttachmentRef attachmentRef, uint32_t slot)
  {
    attachments[attachmentRef.GetId()] = ColorAttachment
    {
      .slot = slot,
    };
  }
  void GraphicsPassConfig::SubPass::UseDepthStencilAttachment(AttachmentRef attachmentRef)
  {
    attachments[attachmentRef.GetId()] = DepthStencilAttachment
    {
    };
  }
  void GraphicsPassConfig::SubPass::UseInputAttachment(AttachmentRef attachmentRef, uint32_t slot)
  {
    attachments[attachmentRef.GetId()] = InputAttachment
    {
      .slot = slot,
    };
  }
  void GraphicsPassConfig::SubPass::UseShaderAttachment(AttachmentRef attachmentRef)
  {
    attachments[attachmentRef.GetId()] = ShaderAttachment
    {
    };
  }

  template <> GraphicsSamplerConfig::Filter FromString(std::string_view str)
  {
    static std::unordered_map<std::string_view, GraphicsSamplerConfig::Filter> const values =
    {{
      { "Nearest", GraphicsSamplerConfig::Filter::Nearest },
      { "Linear", GraphicsSamplerConfig::Filter::Linear },
    }};
    auto i = values.find(str);
    if(i == values.end())
      throw Exception() << "invalid sampler filter: " << str;
    return i->second;
  }

  template <> GraphicsSamplerConfig::Wrap FromString(std::string_view str)
  {
    static std::unordered_map<std::string_view, GraphicsSamplerConfig::Wrap> const values =
    {{
      { "Repeat", GraphicsSamplerConfig::Wrap::Repeat },
      { "RepeatMirror", GraphicsSamplerConfig::Wrap::RepeatMirror },
      { "Clamp", GraphicsSamplerConfig::Wrap::Clamp },
      { "Border", GraphicsSamplerConfig::Wrap::Border },
    }};
    auto i = values.find(str);
    if(i == values.end())
      throw Exception() << "invalid sampler wrap: " << str;
    return i->second;
  }

  GraphicsMesh::GraphicsMesh(GraphicsVertexBuffer& vertexBuffer, uint32_t verticesCount)
  : _vertexBuffer(vertexBuffer), _count(verticesCount) {}

  GraphicsMesh::GraphicsMesh(GraphicsVertexBuffer& vertexBuffer, GraphicsIndexBuffer& indexBuffer, uint32_t indicesCount)
  : _vertexBuffer(vertexBuffer), _pIndexBuffer(&indexBuffer), _count(indicesCount) {}

  GraphicsVertexBuffer& GraphicsMesh::GetVertexBuffer() const
  {
    return _vertexBuffer;
  }
  GraphicsIndexBuffer* GraphicsMesh::GetIndexBuffer() const
  {
    return _pIndexBuffer;
  }
  uint32_t GraphicsMesh::GetCount() const
  {
    return _count;
  }

  void GraphicsContext::BindMesh(GraphicsMesh const& mesh)
  {
    BindVertexBuffer(0, mesh._vertexBuffer);
    BindIndexBuffer(mesh._pIndexBuffer);
  }

  GraphicsAssetManager::GraphicsAssetManager(GraphicsDevice& device, GraphicsPool& pool)
  : _device(device), _pool(pool) {}

  GraphicsDevice& GraphicsAssetManager::GetDevice() const
  {
    return _device;
  }

  GraphicsPool& GraphicsAssetManager::GetPool() const
  {
    return _pool;
  }

  void GraphicsAssetManager::AddContextTask(std::function<void(GraphicsContext&)>&& contextTask)
  {
    _contextTasks.push_back(std::move(contextTask));
  }

  void GraphicsAssetManager::RunContextTasks(GraphicsContext& context)
  {
    for(size_t i = 0; i < _contextTasks.size(); ++i)
      _contextTasks[i](context);
    _contextTasks.clear();
  }

  TextureAssetLoader::TextureAssetLoader(GraphicsAssetManager& manager)
  : _manager(manager) {}

  SamplerAssetLoader::SamplerAssetLoader(GraphicsAssetManager& manager)
  : _manager(manager) {}
}
