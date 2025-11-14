module;

#include <tuple>
#include <variant>

export module coil.core.component;

export namespace Coil
{
  // dummy types to use in concepts
  struct DummyComponent
  {
    using State = std::tuple<>;
  };

  template <typename Context>
  struct ComponentContextTraits;

  struct DummyContext {};

  template <>
  struct ComponentContextTraits<DummyContext>
  {
    template <typename Context>
    static constexpr bool HasContext = std::same_as<Context, DummyContext>;
  };

  template <typename Context>
  concept IsComponentContext = requires
  {
    { ComponentContextTraits<Context>::template HasContext<DummyContext> } -> std::convertible_to<bool>;
  };

  // component traits
  template <typename Component>
  struct ComponentTraits;

  template <typename Component>
  concept IsComponent = requires(Component&& component, typename ComponentTraits<Component>::State&& state, DummyContext& context)
  {
    { ComponentTraits<Component>::template OnCreate<DummyContext>(std::move(component), context) } -> std::same_as<typename ComponentTraits<Component>::State>;
    ComponentTraits<Component>::template OnDestroy<DummyContext>(std::move(state), context);
  };

  // component traits, default implementation
  template <typename Component>
  requires requires(Component&& component, typename Component::State&& state, DummyContext& context)
  {
    { std::move(component).template OnCreate<DummyContext>(context) } -> std::same_as<typename Component::State>;
    Component::template OnDestroy<DummyContext>(std::move(state), context);
  }
  struct ComponentTraits<Component>
  {
    using State = typename Component::State;

    template <IsComponentContext Context>
    static State OnCreate(Component&& component, Context& context)
    {
      return std::move(component).template OnCreate<Context>(context);
    }

    template <IsComponentContext Context>
    static void OnDestroy(State&& state, Context& context)
    {
      Component::template OnDestroy<Context>(std::move(state), context);
    }
  };

  template <typename Context>
  struct ComponentContextTraits;

  // wrapper for component which is also a context
  // stores component's state, but remembers what component it was
  template <IsComponent Component>
  struct ComponentContext
  {
    ComponentContext(typename ComponentTraits<Component>::State* pState)
    : pState{pState} {}

    typename ComponentTraits<Component>::State* pState;
  };

  template <IsComponent Component>
  struct ComponentContextTraits<ComponentContext<Component>>
  {
    template <typename Context>
    static constexpr bool HasContext = std::same_as<Component, Context>;

    using State = typename ComponentTraits<Component>::State;

    template <typename Context>
    requires HasContext<Context>
    static State& GetState(ComponentContext<Component>& context)
    {
      return *context.pState;
    }
  };

  // std::tuple as a component context
  template <IsComponentContext... Contexts>
  struct ComponentContextTraits<std::tuple<Contexts...>>
  {
    template <typename Context>
    static constexpr bool HasContext = (ComponentContextTraits<Contexts>::template HasContext<Context> || ...);

    template <typename Context>
    requires HasContext<Context>
    static auto& GetState(std::tuple<Contexts...>& context)
    {
      return [&]<size_t i>(this auto const& search) -> auto&
      {
        using CurrentContext = Contexts...[i];
        if constexpr(ComponentContextTraits<CurrentContext>::template HasContext<Context>)
        {
          return ComponentContextTraits<CurrentContext>::template GetState<Context>(std::get<i>(context));
        }
        else
        {
          return search.template operator()<i + 1>();
        }
      }.template operator()<0>();
    }
  };

  // std::tuple as a component
  template <IsComponent... Components>
  struct ComponentTraits<std::tuple<Components...>>
  {
    using State = std::tuple<typename ComponentTraits<Components>::State...>;

    template <IsComponentContext Context>
    static State OnCreate(std::tuple<Components...>&& components, Context& context)
    {
      return [&]<size_t i, typename AccumulatedContext, typename... States>(this auto const& create, AccumulatedContext& context, States&&... states) -> State
      {
        if constexpr(i < sizeof...(Components))
        {
          using Component = Components...[i];
          using State = typename ComponentTraits<Component>::State;
          State state = ComponentTraits<Component>::template OnCreate<AccumulatedContext>(std::move(std::get<i>(components)), context);

          using NewContext = std::tuple<ComponentContext<Component>, AccumulatedContext>;
          NewContext newContext{{&state}, context};
          return create.template operator()<i + 1, NewContext, States..., State>(newContext, std::move(states)..., std::move(state));
        }
        else
        {
          return {std::move(states)...};
        }
      }.template operator ()<0, Context>(context);
    }

    template <IsComponentContext Context>
    static void OnDestroy(State&& states, Context& context)
    {
      [&]<size_t i, typename AccumulatedContext, typename... States>(this auto const& destroy, AccumulatedContext& context)
      {
        if constexpr(i < sizeof...(Components))
        {
          using Component = Components...[i];
          using State = typename ComponentTraits<Component>::State;
          State state = std::move(std::get<i>(states));

          {
            using NewContext = std::tuple<ComponentContext<Component>, AccumulatedContext>;
            NewContext newContext{{&state}, context};
            destroy.template operator()<i + 1, NewContext>(newContext);
          }

          ComponentTraits<Component>::template OnDestroy<AccumulatedContext>(std::move(state), context);
        }
      }.template operator ()<0, Context>(context);
    }
  };

  // std::variant as a component
  template <IsComponent... Components>
  struct ComponentTraits<std::variant<Components...>>
  {
    using State = std::variant<typename ComponentTraits<Components>::State...>;

    template <IsComponentContext Context>
    static State OnCreate(std::variant<Components...>&& component, Context& context)
    {
      return [&]<size_t... I>(std::index_sequence<I...>) -> State
      {
        return std::array{[&]() -> State
        {
          return {std::in_place_index<I>, ComponentTraits<Components>::template OnCreate<Context>(std::get<I>(std::move(component)), context)};
        }...}[component.index()]();
      }(std::make_index_sequence<sizeof...(Components)>());
    }

    template <IsComponentContext Context>
    static void OnDestroy(State&& state, Context& context)
    {
      [&]<size_t... I>(std::index_sequence<I...>)
      {
        std::array{[&]()
        {
          ComponentTraits<Components>::template OnDestroy<Context>(std::get<I>(std::move(state)), context);
        }...}[state.index()]();
      }(std::make_index_sequence<sizeof...(Components)>());
    }
  };

  // convenience function
  template <typename SpecificContext, IsComponentContext Context>
  auto& GetComponentContext(Context& context)
  {
    static_assert(ComponentContextTraits<Context>::template HasContext<SpecificContext>, "No specified context");
    return ComponentContextTraits<Context>::template GetState<SpecificContext>(context);
  }

  template <IsComponent Component>
  class ComponentRoot
  {
  public:
    ComponentRoot(Component&& component)
    : state_{ComponentTraits<Component>::template OnCreate<std::tuple<>>(std::move(component), rootContext_)}
    {
    }

    ~ComponentRoot()
    {
      ComponentTraits<Component>::template OnDestroy<std::tuple<>>(std::move(state_), rootContext_);
    }

  private:
    std::tuple<> rootContext_;
    typename ComponentTraits<Component>::State state_;
  };
}
