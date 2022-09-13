#pragma once

#include "graphics.hpp"
#include <vector>
#include <unordered_map>
#include <concepts>

namespace Coil
{
  class RenderContext
  {
  public:
    RenderContext();

    void Begin(GraphicsContext& context);
    void Reset();

    void SetPipeline(GraphicsPipeline& pipeline);
    void SetMesh(GraphicsMesh& mesh);
    void SetUniformBuffer(GraphicsSlotSetId slotSetId, GraphicsSlotId slotId, Buffer const& buffer);
    void SetInstanceData(uint32_t slot, Buffer const& buffer);
    void EndInstance();

    void Flush();

  private:
    GraphicsContext* _pContext = nullptr;
    uint32_t _maxBufferSize = 0;
    uint32_t _indicesCount;
    struct InstanceData
    {
      uint32_t stride;
      std::vector<uint8_t> data;
    };
    std::unordered_map<uint32_t, InstanceData> _instanceData;
    uint32_t _instancesCount;
  };

  template <typename Knob>
  concept IsRenderKnob = requires(Knob const& knob, RenderContext& context)
  {
    std::totally_ordered<typename Knob::Key>;
    std::same_as<std::remove_cv_t<decltype(knob.GetKey())>, typename Knob::Key>;
    { knob.Apply(context) } -> std::same_as<void>;
  };

  class RenderPipelineKnob
  {
  public:
    RenderPipelineKnob(GraphicsPipeline& pipeline)
    : _pipeline(pipeline) {}

    using Key = GraphicsPipeline*;
    Key GetKey() const
    {
      return &_pipeline;
    }

    void Apply(RenderContext& context) const
    {
      context.SetPipeline(_pipeline);
    }

  private:
    GraphicsPipeline& _pipeline;
  };

  // Uniform buffer knob.
  // Referenced struct should not be freed until flush.
  template <typename Struct, GraphicsSlotSetId slotSetId, GraphicsSlotId slotId>
  class RenderUniformBufferKnob
  {
  public:
    RenderUniformBufferKnob(Struct const& data)
    : _data(data) {}

    using Key = Struct const*;
    Key GetKey() const
    {
      return &_data;
    }

    void Apply(RenderContext& context) const
    {
      context.SetUniformBuffer(slotSetId, slotId, Buffer(&_data, sizeof(_data)));
    }

  private:
    Struct const& _data;
  };

  class RenderMeshKnob
  {
  public:
    RenderMeshKnob(GraphicsMesh& mesh)
    : _mesh(mesh) {}

    using Key = GraphicsMesh*;
    Key GetKey() const
    {
      return &_mesh;
    }

    void Apply(RenderContext& context) const
    {
      context.SetMesh(_mesh);
    }

  private:
    GraphicsMesh& _mesh;
  };

  // Instance data knob.
  // Data is copied into knob.
  template <typename Struct, uint32_t slot = 1>
  class RenderInstanceDataKnob
  {
  public:
    RenderInstanceDataKnob(Struct const& data)
    : _data(data) {}

    // all instance data knobs should be considered different
    // use it's own address as key
    using Key = RenderInstanceDataKnob const*;
    Key GetKey() const
    {
      return this;
    }

    void Apply(RenderContext& context) const
    {
      context.SetInstanceData(slot, Buffer(&_data, sizeof(_data)));
    }

  private:
    Struct _data;
  };

  // Pseudo-knob just for ordering rendering.
  template <typename T>
  class RenderOrderKnob
  {
  public:
    RenderOrderKnob(T const& key)
    : _key(key) {}

    using Key = T;
    Key GetKey() const
    {
      return _key;
    }

    void Apply()
    {
      // nothing to do
    }

  private:
    T _key;
  };

  template <IsRenderKnob... Knobs>
  class RenderCache
  {
  public:
    void Begin(GraphicsContext& context)
    {
      Reset();
      _context.Begin(context);
    }

    void Reset()
    {
      _instances.clear();
      _instanceIndices.clear();
    }

    void Render(Knobs&&... knobs)
    {
      // add instance
      _instanceIndices.push_back(_instances.size());
      _instances.push_back(std::tuple<Knobs...>(std::forward<Knobs>(knobs)...));
    }

    void Flush()
    {
      // sort instances
      std::sort(_instanceIndices.begin(), _instanceIndices.end(), [&](size_t a, size_t b)
      {
        return KnobsKey(_instances[a]) < KnobsKey(_instances[b]);
      });

      // apply instances in order, skipping application of same knobs
      for(size_t i = 0; i < _instanceIndices.size(); ++i)
      {
        auto const& instance = _instances[_instanceIndices[i]];
        ApplyInstance(instance,
          i > 0 ? FindFirstDifferent<0>(_instances[_instanceIndices[i - 1]], instance) : 0,
          std::index_sequence_for<Knobs...>());
        _context.EndInstance();
      }
      Reset();

      _context.Flush();
    }

    static std::tuple<typename Knobs::Key...> KnobsKey(std::tuple<Knobs...> const& knobs)
    {
      return std::apply([](Knobs const&... knobs)
      {
        return std::tuple<typename Knobs::Key...>(knobs.GetKey()...);
      }, knobs);
    }

    template <size_t i>
    static size_t FindFirstDifferent(std::tuple<Knobs...> const& a, std::tuple<Knobs...> const& b)
    {
      if constexpr(i < sizeof...(Knobs))
      {
        if(std::get<i>(a).GetKey() == std::get<i>(b).GetKey())
        {
          return FindFirstDifferent<i + 1>(a, b);
        }
      }
      return i;
    }

    template <size_t... knobIndices>
    void ApplyInstance(std::tuple<Knobs...> const& instance, size_t i, std::index_sequence<knobIndices...> seq)
    {
      using ApplyFunc = void (RenderCache::*)(std::tuple<Knobs...> const&);
      static constinit ApplyFunc const applyFuncs[] = { &RenderCache::ApplyKnob<knobIndices>... };
      for(; i < sizeof...(Knobs); ++i)
        (this->*applyFuncs[i])(instance);
    }

    template <size_t i>
    void ApplyKnob(std::tuple<Knobs...> const& instance)
    {
      std::get<i>(instance).Apply(_context);
    }

  private:
    std::vector<std::tuple<Knobs...>> _instances;
    std::vector<size_t> _instanceIndices;
    RenderContext _context;
  };
}
