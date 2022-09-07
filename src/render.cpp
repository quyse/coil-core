#include "render.hpp"

namespace Coil
{
  RenderContext::RenderContext()
  {
    Reset();
  }

  void RenderContext::Begin(GraphicsContext& context)
  {
    _pContext = &context;
    Reset();
  }

  void RenderContext::Reset()
  {
    _indicesCount = 0;
    for(auto& i : _instanceData)
      i.second.clear();
    _instancesCount = 0;
  }

  void RenderContext::SetPipeline(GraphicsPipeline& pipeline)
  {
    Flush();
    _pContext->BindPipeline(pipeline);
  }

  void RenderContext::SetMesh(GraphicsMesh& mesh)
  {
    Flush();
    _pContext->BindMesh(mesh);
    _indicesCount = mesh.GetCount();
    _instancesCount = 0;
  }

  void RenderContext::SetUniformBuffer(GraphicsSlotSetId slotSetId, GraphicsSlotId slotId, Buffer const& buffer)
  {
    Flush();
    _pContext->BindUniformBuffer(slotSetId, slotId, buffer);
  }

  void RenderContext::SetInstanceData(uint32_t slot, Buffer const& buffer)
  {
    auto& slotData = _instanceData[slot];
    slotData.insert(slotData.end(), (uint8_t*)buffer.data, (uint8_t*)buffer.data + buffer.size);
  }

  void RenderContext::EndInstance()
  {
    ++_instancesCount;
  }

  void RenderContext::Flush()
  {
    if(!_instancesCount) return;

    for(auto& i : _instanceData)
    {
      if(i.second.size())
      {
        _pContext->BindDynamicVertexBuffer(i.first, i.second);
      }
    }
    _pContext->Draw(_indicesCount, _instancesCount);
    Reset();
  }
}
