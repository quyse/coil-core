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
    return AttachmentRef(*this, attachmentId);
  }

  GraphicsPassConfig::SubPassRef GraphicsPassConfig::AddSubPass()
  {
    GraphicsSubPassId subPassId = (GraphicsSubPassId)subPasses.size();
    subPasses.push_back({});
    return SubPassRef(*this, subPassId);
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

  GraphicsMesh::GraphicsMesh(GraphicsVertexBuffer& vertexBuffer, GraphicsIndexBuffer* pIndexBuffer)
  : _vertexBuffer(vertexBuffer), _pIndexBuffer(pIndexBuffer) {}

  void GraphicsContext::BindMesh(GraphicsMesh const& mesh, uint32_t vertexBufferSlot)
  {
    BindVertexBuffer(vertexBufferSlot, mesh._vertexBuffer);
    BindIndexBuffer(mesh._pIndexBuffer);
  }
}
