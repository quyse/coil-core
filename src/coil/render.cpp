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
    _maxBufferSize = _pContext->GetMaxBufferSize();
    Reset();
  }

  void RenderContext::Reset()
  {
    _indicesCount = 0;
    for(auto& i : _instanceData)
      i.second.data.clear();
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

  void RenderContext::SetImage(GraphicsSlotSetId slotSetId, GraphicsSlotId slotId, GraphicsImage& image)
  {
    Flush();
    _pContext->BindImage(slotSetId, slotId, image);
  }

  void RenderContext::SetInstanceData(uint32_t slot, Buffer const& buffer)
  {
    auto& slotData = _instanceData[slot];
    slotData.data.insert(slotData.data.end(), (uint8_t*)buffer.data, (uint8_t*)buffer.data + buffer.size);
  }

  void RenderContext::EndInstance()
  {
    ++_instancesCount;
  }

  void RenderContext::Flush()
  {
    if(!_instancesCount) return;

    if(!_instanceData.empty())
    {
      // calculate number of instances per step
      uint32_t instancesPerStep = std::numeric_limits<uint32_t>::max();
      for(auto& i : _instanceData)
      {
        if(i.second.data.size())
        {
          i.second.stride = i.second.data.size() / _instancesCount;
          instancesPerStep = std::min(instancesPerStep, _maxBufferSize / i.second.stride);
        }
      }

      // perform steps
      for(uint32_t k = 0; k < _instancesCount; k += instancesPerStep)
      {
        uint32_t instancesToRender = std::min(_instancesCount - k, instancesPerStep);
        for(auto& i : _instanceData)
        {
          if(i.second.data.size())
          {
            _pContext->BindDynamicVertexBuffer(i.first,
              Buffer(
                (uint8_t const*)i.second.data.data() + k * i.second.stride,
                instancesToRender * i.second.stride
              )
            );
          }
        }
        _pContext->Draw(_indicesCount, instancesToRender);
      }
    }

    Reset();
  }
}
