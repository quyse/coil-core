module;

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <variant>
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
    enum class RenderType
    {
      Triangles,
      Instances,
    };

    RenderContext(RenderType renderType)
    : renderType_{renderType}
    {
      Reset();
    }

    void Begin(GraphicsContext& context)
    {
      pContext_ = &context;
      maxBufferSize_ = pContext_->GetMaxBufferSize();
      Reset();
    }

    void Reset()
    {
      indicesCount_ = 0;
      for(auto& i : instanceData_)
        i.second.data.clear();
      instancesCount_ = 0;
    }

    void SetPipeline(GraphicsPipeline& pipeline)
    {
      Flush();
      pContext_->BindPipeline(pipeline);
    }

    void SetMesh(GraphicsMesh& mesh)
    {
      Flush();
      pContext_->BindMesh(mesh);
      indicesCount_ = mesh.GetCount();
      instancesCount_ = 0;
    }

    void SetUniformBuffer(GraphicsSlotSetId slotSetId, GraphicsSlotId slotId, Buffer const& buffer)
    {
      Flush();
      pContext_->BindUniformBuffer(slotSetId, slotId, buffer);
    }

    void SetImage(GraphicsSlotSetId slotSetId, GraphicsSlotId slotId, GraphicsImage& image)
    {
      Flush();
      pContext_->BindImage(slotSetId, slotId, image);
    }

    void SetInstanceData(uint32_t slot, Buffer const& buffer)
    {
      auto& slotData = instanceData_[slot];
      slotData.data.insert(slotData.data.end(), (uint8_t*)buffer.data, (uint8_t*)buffer.data + buffer.size);
    }

    void EndInstance()
    {
      ++instancesCount_;
    }

    void Flush()
    {
      if(!instancesCount_) return;

      // calculate number of instances per step
      uint32_t instancesPerStep = std::numeric_limits<uint32_t>::max();
      for(auto& i : instanceData_)
      {
        if(!i.second.data.empty())
        {
          i.second.stride = i.second.data.size() / instancesCount_;
          instancesPerStep = std::min(instancesPerStep, maxBufferSize_ / i.second.stride);
        }
      }

      // perform steps
      for(uint32_t k = 0; k < instancesCount_; k += instancesPerStep)
      {
        uint32_t instancesToRender = std::min(instancesCount_ - k, instancesPerStep);
        for(auto& i : instanceData_)
        {
          if(!i.second.data.empty())
          {
            pContext_->BindDynamicVertexBuffer(i.first,
              Buffer(
                (uint8_t const*)i.second.data.data() + k * i.second.stride,
                instancesToRender * i.second.stride
              )
            );
          }
        }
        switch(renderType_)
        {
        case RenderType::Triangles:
          pContext_->Draw(instancesToRender * 3);
          break;
        case RenderType::Instances:
          pContext_->Draw(indicesCount_, instancesToRender);
          break;
        }
      }

      Reset();
    }

  private:
    GraphicsContext* pContext_ = nullptr;
    uint32_t maxBufferSize_ = 0;
    uint32_t indicesCount_;
    struct InstanceData
    {
      uint32_t stride;
      std::vector<uint8_t> data;
    };
    std::unordered_map<uint32_t, InstanceData> instanceData_;
    uint32_t instancesCount_;
    RenderType renderType_;
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
    : pipeline_(pipeline) {}

    using Key = GraphicsPipeline*;
    Key GetKey() const
    {
      return &pipeline_;
    }

    void Apply(RenderContext& context) const
    {
      context.SetPipeline(pipeline_);
    }
    bool Apply(RenderContext& context, RenderPipelineKnob const& previousKnob) const
    {
      if(GetKey() == previousKnob.GetKey())
        return false;

      Apply(context);
      return true;
    }

  private:
    GraphicsPipeline& pipeline_;
  };

  // Uniform buffer knob.
  // Referenced struct should not be freed until flush.
  template <typename Struct, GraphicsSlotSetId slotSetId, GraphicsSlotId slotId>
  class RenderUniformBufferKnob
  {
  public:
    RenderUniformBufferKnob(Struct const& data)
    : data_(data) {}

    using Key = Struct const*;
    Key GetKey() const
    {
      return &data_;
    }

    void Apply(RenderContext& context) const
    {
      context.SetUniformBuffer(slotSetId, slotId, Buffer(&data_, sizeof(data_)));
    }
    bool Apply(RenderContext& context, RenderUniformBufferKnob const& previousKnob) const
    {
      if(GetKey() == previousKnob.GetKey())
        return false;

      Apply(context);
      return true;
    }

  private:
    Struct const& data_;
  };

  // Image knob.
  template <GraphicsSlotSetId slotSetId, GraphicsSlotId slotId>
  class RenderImageKnob
  {
  public:
    RenderImageKnob(GraphicsImage& image)
    : image_(image) {}

    using Key = GraphicsImage*;
    Key GetKey() const
    {
      return &image_;
    }

    void Apply(RenderContext& context) const
    {
      context.SetImage(slotSetId, slotId, image_);
    }
    bool Apply(RenderContext& context, RenderImageKnob const& previousKnob) const
    {
      if(GetKey() == previousKnob.GetKey())
        return false;

      Apply(context);
      return true;
    }

  private:
    GraphicsImage& image_;
  };

  class RenderMeshKnob
  {
  public:
    RenderMeshKnob(GraphicsMesh& mesh)
    : mesh_(mesh) {}

    using Key = GraphicsMesh*;
    Key GetKey() const
    {
      return &mesh_;
    }

    void Apply(RenderContext& context) const
    {
      context.SetMesh(mesh_);
    }
    bool Apply(RenderContext& context, RenderMeshKnob const& previousKnob) const
    {
      if(GetKey() == previousKnob.GetKey())
        return false;

      Apply(context);
      return true;
    }

  private:
    GraphicsMesh& mesh_;
  };

  // dynamic triangle mesh data
  template <typename Vertex, uint32_t slot = 0>
  class RenderTriangleKnob
  {
  public:
    RenderTriangleKnob(Vertex const (&vertices)[3])
    {
      std::copy(vertices, vertices + 3, vertices_);
    }

    // all triangle knobs should be considered different
    // use it's own address as key
    using Key = RenderTriangleKnob const*;
    Key GetKey() const
    {
      return this;
    }

    void Apply(RenderContext& context) const
    {
      context.SetInstanceData(slot, Buffer{&vertices_, sizeof(vertices_)});
    }
    bool Apply(RenderContext& context, RenderTriangleKnob const& previousKnob) const
    {
      Apply(context);
      return true;
    }

    static constexpr RenderContext::RenderType GetRenderType()
    {
      return RenderContext::RenderType::Triangles;
    }

  private:
    Vertex vertices_[3];
  };

  // Instance data knob.
  // Data is copied into knob.
  template <typename Struct, uint32_t slot = 1>
  class RenderInstanceDataKnob
  {
  public:
    template <typename T>
    RenderInstanceDataKnob(T&& data)
    : data_{std::forward<T>(data)} {}

    // all instance data knobs should be considered different
    // use it's own address as key
    using Key = RenderInstanceDataKnob const*;
    Key GetKey() const
    {
      return this;
    }

    void Apply(RenderContext& context) const
    {
      if constexpr(sizeof(data_) > 0)
      {
        context.SetInstanceData(slot, Buffer(&data_, sizeof(data_)));
      }
    }
    bool Apply(RenderContext& context, RenderInstanceDataKnob const& previousKnob) const
    {
      Apply(context);
      return true;
    }

    static constexpr RenderContext::RenderType GetRenderType()
    {
      return RenderContext::RenderType::Instances;
    }

  private:
    Struct data_;
  };

  // Pseudo-knob just for ordering rendering.
  template <typename T>
  class RenderOrderKnob
  {
  public:
    RenderOrderKnob(T const& key)
    : key_(key) {}

    using Key = T;
    Key GetKey() const
    {
      return key_;
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
    T key_;
  };

  template <typename Knob>
  concept RenderKnobSetsRenderType = IsRenderKnob<Knob> && requires
  {
    { Knob::GetRenderType() } -> std::same_as<RenderContext::RenderType>;
  };

  // get render type of knobs
  // should be the same for all knobs which set it
  template <IsRenderKnob... Knobs>
  requires (RenderKnobSetsRenderType<Knobs> || ...)
  consteval RenderContext::RenderType GetRenderKnobsRenderType()
  {
    std::optional<RenderContext::RenderType> renderType;
    ([&]<typename Knob>()
    {
      if constexpr(RenderKnobSetsRenderType<Knob>)
      {
        RenderContext::RenderType const knobRenderType = Knob::GetRenderType();
        if(renderType.has_value())
        {
          if(renderType.value() != knobRenderType)
          {
            throw std::runtime_error{"render knobs set different render types"};
          }
        }
        else
        {
          renderType = {knobRenderType};
        }
      }
    }.template operator()<Knobs>(), ...);
    return renderType.value();
  }

  // Tuple knob for combining knobs.
  template <IsRenderKnob... Knobs>
  class RenderTupleKnob
  {
  public:
    RenderTupleKnob(Knobs&&... knobs)
    : knobs_(std::move(knobs)...) {}

    using Key = std::tuple<typename Knobs::Key...>;
    Key GetKey() const
    {
      return std::apply([](Knobs const&... knobs) -> Key
      {
        return { knobs.GetKey()... };
      }, knobs_);
    }

    void Apply(RenderContext& context) const
    {
      std::apply([&](Knobs const&... knobs)
      {
        (knobs.Apply(context), ...);
      }, knobs_);
    }
    bool Apply(RenderContext& context, RenderTupleKnob const& previousKnob) const
    {
      return PartialApply(context, previousKnob, std::index_sequence_for<Knobs...>());
    }

    static constexpr RenderContext::RenderType GetRenderType() requires (RenderKnobSetsRenderType<Knobs> || ...)
    {
      return GetRenderKnobsRenderType<Knobs...>();
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
      std::get<i>(knobs_).Apply(context);
    }
    template <size_t i>
    bool PartialApplySubKnob(RenderContext& context, RenderTupleKnob const& previousKnob) const
    {
      return std::get<i>(knobs_).Apply(context, std::get<i>(previousKnob.knobs_));
    }

    std::tuple<Knobs...> knobs_;
  };

  // Variant knob for providing multiple paths for knobs.
  template <IsRenderKnob... Knobs>
  class RenderVariantKnob
  {
  public:
    template <IsRenderKnob Knob>
    requires (std::constructible_from<std::variant<Knobs...>, Knob&&>)
    RenderVariantKnob(Knob&& knob)
    : knob_(std::move(knob)) {}

    using Key = std::variant<typename Knobs::Key...>;
    Key GetKey() const
    {
      return std::visit([](auto const& knob) -> Key
      {
        return knob;
      }, knob_);
    }

    void Apply(RenderContext& context) const
    {
      std::apply([&](auto const& knob)
      {
        knob.Apply(context);
      }, knob_);
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
      }, knob_, previousKnob.knob_);
    }

  private:
    std::variant<Knobs...> knob_;
  };

  // render cache implementation
  template <RenderKnobSetsRenderType Knob>
  class RenderCacheImpl
  {
  public:
    RenderCacheImpl()
    : context_{Knob::GetRenderType()} {}

    void Reset()
    {
      instances_.clear();
      instanceIndices_.clear();
    }

    template <typename... Args>
    void Render(Args&&... args)
    {
      // add instance
      instanceIndices_.push_back(instances_.size());
      instances_.push_back({ std::forward<Args>(args)... });
    }

    void Flush(GraphicsContext& context)
    {
      context_.Begin(context);

      // sort instances
      std::sort(instanceIndices_.begin(), instanceIndices_.end(), [&](size_t a, size_t b)
      {
        return instances_[a].GetKey() < instances_[b].GetKey();
      });

      // apply instances in order
      for(size_t i = 0; i < instanceIndices_.size(); ++i)
      {
        auto const& instance = instances_[instanceIndices_[i]];

        if(i > 0)
          instance.Apply(context_, instances_[instanceIndices_[i - 1]]);
        else
          instance.Apply(context_);

        context_.EndInstance();
      }

      Reset();

      context_.Flush();
    }

  private:
    std::vector<Knob> instances_;
    std::vector<size_t> instanceIndices_;
    RenderContext context_;
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
