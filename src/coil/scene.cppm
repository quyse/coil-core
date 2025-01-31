module;

#include <tuple>
#include <unordered_set>

export module coil.core.scene;

import coil.core.base.signals;
import coil.core.util;

export namespace Coil
{
  template <typename Object, typename Manager>
  concept IsSceneObjectManagedBy = requires(Object& object, Manager& manager)
  {
    manager.RegisterObject(object);
    manager.UnregisterObject(object);
  };

  template <typename T>
  concept IsSceneSignal = requires(T t)
  {
    t.Get();
  };

  template <IsDecayed Object>
  class Scene
  {
  private:
    Object object_;

  public:
    template <IsSameDecayed<Object> T>
    Scene(T&& object)
    : object_{std::forward<T>(object)} {}

    template <typename... Managers>
    void Register(std::tuple<Managers&...> const& managers)
    {
      std::apply([&](Managers&... managers)
      {
        ([&](Managers& manager)
        {
          if constexpr(IsSceneObjectManagedBy<Object, Managers>)
          {
            manager.RegisterObject(object_);
          }
        }(managers), ...);
      }, managers);
    }

    template <typename... Managers>
    void Unregister(std::tuple<Managers&...> const& managers)
    {
      std::apply([&](Managers&... managers)
      {
        ([&](Managers& manager)
        {
          if constexpr(IsSceneObjectManagedBy<Object, Managers>)
          {
            manager.UnregisterObject(object_);
          }
        }(managers), ...);
      }, managers);
    }

    template <typename... Managers>
    void Update(std::tuple<Managers&...> const& managers)
    {
    }
  };

  template <IsDecayed Object>
  requires IsSceneSignal<Object>
  class Scene<Object>
  {
  private:
    Object object_;

    using ResultObject = std::decay_t<decltype(object_.Get())>;

    SignalBase::WatchCell cell_;
    Scene<ResultObject> resultContainer_;

  public:
    template <IsSameDecayed<Object> T>
    Scene(T&& object)
    : object_{std::forward<T>(object)}
    , resultContainer_{object_.Get()}
    {}

  public:
    template <typename... Managers>
    void Register(std::tuple<Managers&...> const& managers)
    {
      cell_.SubscribeTo(object_);
      resultContainer_.template Register<Managers...>(managers);
    }

    template <typename... Managers>
    void Unregister(std::tuple<Managers&...> const& managers)
    {
      resultContainer_.template Unregister<Managers...>(managers);
      cell_.Remove();
    }

    template <typename... Managers>
    void Update(std::tuple<Managers&...> const& managers)
    {
      if(cell_.CheckDirty())
      {
        resultContainer_.template Unregister<Managers...>(managers);
        resultContainer_ = Scene<ResultObject>{object_.Get()};
        resultContainer_.template Register<Managers...>(managers);
      }
    }
  };

  template <IsDecayed... Objects>
  class Scene<std::tuple<Objects...>>
  {
  private:
    std::tuple<Scene<Objects>...> containers_;

  public:
    template <IsSameDecayed<std::tuple<Objects...>> T>
    Scene(T&& objects)
    : containers_{std::forward<T>(objects)} {}

    template <typename... Managers>
    void Register(std::tuple<Managers&...> const& managers)
    {
      std::apply([&](Scene<Objects>&... containers)
      {
        (containers.template Register<Managers...>(managers), ...);
      }, containers_);
    }

    template <typename... Managers>
    void Unregister(std::tuple<Managers&...> const& managers)
    {
      std::apply([&](Scene<Objects>&... containers)
      {
        (containers.template Unregister<Managers...>(managers), ...);
      }, containers_);
    }

    template <typename... Managers>
    void Update(std::tuple<Managers&...> const& managers)
    {
      std::apply([&](Scene<Objects>&... containers)
      {
        (containers.template Update<Managers...>(managers), ...);
      }, containers_);
    }
  };

  template <IsDecayed Object>
  class Scene<std::optional<Object>>
  {
  private:
    std::optional<Scene<Object>> optContainer_;

  public:
    template <IsSameDecayed<std::optional<Object>> T>
    Scene(T&& object)
    : optContainer_{std::forward<T>(object)} {}

    template <typename... Managers>
    void Register(std::tuple<Managers&...> const& managers)
    {
      if(optContainer_.has_value())
      {
        optContainer_.value().template Register<Managers...>(managers);
      }
    }

    template <typename... Managers>
    void Unregister(std::tuple<Managers&...> const& managers)
    {
      if(optContainer_.has_value())
      {
        optContainer_.value().template Unregister<Managers...>(managers);
      }
    }

    template <typename... Managers>
    void Update(std::tuple<Managers&...> const& managers)
    {
      if(optContainer_.has_value())
      {
        optContainer_.value().template Update<Managers...>(managers);
      }
    }
  };

  template <typename T>
  Scene(T&&) -> Scene<std::decay_t<T>>;
}
