module;

#include <functional>
#include <unordered_map>
#include <vector>

export module coil.core.render;

import coil.core.base;
import coil.core.graphics;
import coil.core.math;

export namespace Coil
{
  class RenderContext
  {
  public:
    RenderContext()
    {
      Reset();
    }

    void Begin(GraphicsContext& context)
    {
      _pContext = &context;
      _maxBufferSize = _pContext->GetMaxBufferSize();
      Reset();
    }

    void Reset()
    {
      _indicesCount = 0;
      for(auto& i : _instanceData)
        i.second.data.clear();
      _instancesCount = 0;
    }

    void SetPipeline(GraphicsPipeline& pipeline)
    {
      Flush();
      _pContext->BindPipeline(pipeline);
    }

    void SetMesh(GraphicsMesh& mesh)
    {
      Flush();
      _pContext->BindMesh(mesh);
      _indicesCount = mesh.GetCount();
      _instancesCount = 0;
    }

    void SetUniformBuffer(GraphicsSlotSetId slotSetId, GraphicsSlotId slotId, Buffer const& buffer)
    {
      Flush();
      _pContext->BindUniformBuffer(slotSetId, slotId, buffer);
    }

    void SetImage(GraphicsSlotSetId slotSetId, GraphicsSlotId slotId, GraphicsImage& image)
    {
      Flush();
      _pContext->BindImage(slotSetId, slotId, image);
    }

    void SetInstanceData(uint32_t slot, Buffer const& buffer)
    {
      auto& slotData = _instanceData[slot];
      slotData.data.insert(slotData.data.end(), (uint8_t*)buffer.data, (uint8_t*)buffer.data + buffer.size);
    }

    void EndInstance()
    {
      ++_instancesCount;
    }

    void Flush()
    {
      if(!_instancesCount) return;

      // calculate number of instances per step
      uint32_t instancesPerStep = std::numeric_limits<uint32_t>::max();
      for(auto& i : _instanceData)
      {
        if(!i.second.data.empty())
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
          if(!i.second.data.empty())
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

      Reset();
    }

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
    { knob.Apply(context, knob) } -> std::same_as<bool>;
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
    bool Apply(RenderContext& context, RenderPipelineKnob const& previousKnob) const
    {
      if(GetKey() == previousKnob.GetKey())
        return false;

      Apply(context);
      return true;
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
    bool Apply(RenderContext& context, RenderUniformBufferKnob const& previousKnob) const
    {
      if(GetKey() == previousKnob.GetKey())
        return false;

      Apply(context);
      return true;
    }

  private:
    Struct const& _data;
  };

  // Image knob.
  template <GraphicsSlotSetId slotSetId, GraphicsSlotId slotId>
  class RenderImageKnob
  {
  public:
    RenderImageKnob(GraphicsImage& image)
    : _image(image) {}

    using Key = GraphicsImage*;
    Key GetKey() const
    {
      return &_image;
    }

    void Apply(RenderContext& context) const
    {
      context.SetImage(slotSetId, slotId, _image);
    }
    bool Apply(RenderContext& context, RenderImageKnob const& previousKnob) const
    {
      if(GetKey() == previousKnob.GetKey())
        return false;

      Apply(context);
      return true;
    }

  private:
    GraphicsImage& _image;
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
    bool Apply(RenderContext& context, RenderMeshKnob const& previousKnob) const
    {
      if(GetKey() == previousKnob.GetKey())
        return false;

      Apply(context);
      return true;
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
    bool Apply(RenderContext& context, RenderInstanceDataKnob const& previousKnob) const
    {
      Apply(context);
      return true;
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

    void Apply(RenderContext& context) const
    {
      // nothing to do
    }
    bool Apply(RenderContext& context, RenderOrderKnob const& previousKnob) const
    {
      // nothing to do
      return false;
    }

  private:
    T _key;
  };

  // Tuple knob for combining knobs.
  template <IsRenderKnob... Knobs>
  class RenderTupleKnob
  {
  public:
    RenderTupleKnob(Knobs&&... knobs)
    : _knobs(std::move(knobs)...) {}

    using Key = std::tuple<typename Knobs::Key...>;
    Key GetKey() const
    {
      return std::apply([](Knobs const&... knobs) -> Key
      {
        return { knobs.GetKey()... };
      }, _knobs);
    }

    void Apply(RenderContext& context) const
    {
      std::apply([&](Knobs const&... knobs)
      {
        (knobs.Apply(context), ...);
      }, _knobs);
    }
    bool Apply(RenderContext& context, RenderTupleKnob const& previousKnob) const
    {
      return PartialApply(context, previousKnob, std::index_sequence_for<Knobs...>());
    }

  private:
    template <size_t... knobIndices>
    bool PartialApply(RenderContext& context, RenderTupleKnob const& previousKnob, std::index_sequence<knobIndices...> seq) const
    {
      using ApplySubKnobMethod = void (RenderTupleKnob::*)(RenderContext&) const;
      static constinit ApplySubKnobMethod const applyMethods[] = { &RenderTupleKnob::ApplySubKnob<knobIndices>... };

      using PartialApplySubKnobMethod = bool (RenderTupleKnob::*)(RenderContext&, RenderTupleKnob const&) const;
      static constinit PartialApplySubKnobMethod const partialApplyMethods[] = { &RenderTupleKnob::PartialApplySubKnob<knobIndices>... };

      bool applied = false;
      for(size_t i = 0; i < sizeof...(Knobs); ++i)
      {
        if(applied)
          (this->*applyMethods[i])(context);
        else if((this->*partialApplyMethods[i])(context, previousKnob))
          applied = true;
      }

      return applied;
    }

    template <size_t i>
    void ApplySubKnob(RenderContext& context) const
    {
      std::get<i>(_knobs).Apply(context);
    }
    template <size_t i>
    bool PartialApplySubKnob(RenderContext& context, RenderTupleKnob const& previousKnob) const
    {
      return std::get<i>(_knobs).Apply(context, std::get<i>(previousKnob._knobs));
    }

    std::tuple<Knobs...> _knobs;
  };

  // Variant knob for providing multiple paths for knobs.
  template <IsRenderKnob... Knobs>
  class RenderVariantKnob
  {
  public:
    template <IsRenderKnob Knob>
    requires (std::constructible_from<std::variant<Knobs...>, Knob&&>)
    RenderVariantKnob(Knob&& knob)
    : _knob(std::move(knob)) {}

    using Key = std::variant<typename Knobs::Key...>;
    Key GetKey() const
    {
      return std::visit([](auto const& knob) -> Key
      {
        return knob;
      }, _knob);
    }

    void Apply(RenderContext& context) const
    {
      std::apply([&](auto const& knob)
      {
        knob.Apply(context);
      }, _knob);
    }
    bool Apply(RenderContext& context, RenderVariantKnob const& previousKnob) const
    {
      return std::apply([&]<typename SubKnob, typename PreviousSubKnob>(SubKnob const& subKnob, PreviousSubKnob const& previousSubKnob) -> bool
      {
        if constexpr(std::same_as<SubKnob, PreviousSubKnob>)
        {
          return subKnob.Apply(context, previousSubKnob);
        }
        else
        {
          subKnob.Apply(context);
          return true;
        }
      }, _knob, previousKnob._knob);
    }

  private:
    std::variant<Knobs...> _knob;
  };

  // render cache implementation
  template <IsRenderKnob Knob>
  class RenderCacheImpl
  {
  public:
    void Reset()
    {
      _instances.clear();
      _instanceIndices.clear();
    }

    template <typename... Args>
    void Render(Args&&... args)
    {
      // add instance
      _instanceIndices.push_back(_instances.size());
      _instances.push_back({ std::forward<Args>(args)... });
    }

    void Flush(GraphicsContext& context)
    {
      _context.Begin(context);

      // sort instances
      std::sort(_instanceIndices.begin(), _instanceIndices.end(), [&](size_t a, size_t b)
      {
        return _instances[a].GetKey() < _instances[b].GetKey();
      });

      // apply instances in order
      for(size_t i = 0; i < _instanceIndices.size(); ++i)
      {
        auto const& instance = _instances[_instanceIndices[i]];

        if(i > 0)
          instance.Apply(_context, _instances[_instanceIndices[i - 1]]);
        else
          instance.Apply(_context);

        _context.EndInstance();
      }

      Reset();

      _context.Flush();
    }

  private:
    std::vector<Knob> _instances;
    std::vector<size_t> _instanceIndices;
    RenderContext _context;
  };

  // convenience wrapper
  // wraps knobs types into tuple, but doesn't wrap single knob
  template <IsRenderKnob... Knobs>
  struct RenderCacheHelper
  {
    using RenderCache = RenderCacheImpl<RenderTupleKnob<Knobs...>>;
  };
  template <IsRenderKnob Knob>
  struct RenderCacheHelper<Knob>
  {
    using RenderCache = RenderCacheImpl<Knob>;
  };
  template <IsRenderKnob... Knobs>
  using RenderCache = typename RenderCacheHelper<Knobs...>::RenderCache;
}
