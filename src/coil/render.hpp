#pragma once

#include "graphics.hpp"
#include <algorithm>
#include <concepts>
#include <unordered_map>
#include <vector>

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
    void SetImage(GraphicsSlotSetId slotSetId, GraphicsSlotId slotId, GraphicsImage& image);
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
