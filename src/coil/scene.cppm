module;

#include <tuple>
#include <unordered_set>

export module coil.core.scene;

export namespace Coil
{
  template <typename Object, typename Manager>
  concept IsSimpleSceneObjectManagedBy = requires(Object& object, Manager& manager)
  {
    manager.RegisterObject(object);
    manager.UnregisterObject(object);
  };

  // object can decide to control registration themselves
  template <typename Object>
  concept IsCompoundSceneObject = requires(Object& object)
  {
    // just checking that methods are in place
    object.RegisterCompoundSceneObject(std::tuple{});
    object.UnregisterCompoundSceneObject(std::tuple{});
  };

  template <typename Object, typename Manager>
  concept IsCompoundSceneObjectManagedBy = Object::template ManagedBy<Manager>;

  template <typename Object, typename Manager>
  concept IsSceneObjectManagedBy = IsSimpleSceneObjectManagedBy<Object, Manager> || IsCompoundSceneObjectManagedBy<Object, Manager>;

  template <typename Object, typename... Managers>
  struct CompoundSceneObjectHelper
  {
    template <size_t I, typename F, typename... FilteredManagers>
    static auto ApplyFilteredManagers(F&& f, std::tuple<Managers&...> const& managers, FilteredManagers&... filteredManagers)
    {
      if constexpr(I < sizeof...(Managers))
      {
        using Manager = std::tuple_element_t<I, std::tuple<Managers...>>;
        if constexpr(IsCompoundSceneObjectManagedBy<Object, Manager>)
        {
          return ApplyFilteredManagers<I + 1, F, FilteredManagers..., Manager>(std::forward<F>(f), managers, filteredManagers..., std::get<I>(managers));
        }
        else
        {
          return ApplyFilteredManagers<I + 1, F, FilteredManagers...>(std::forward<F>(f), managers, filteredManagers...);
        }
      }
      else
      {
        return std::forward<F>(f)(filteredManagers...);
      }
    }
  };

  class SceneObjectBase
  {
  public:
    virtual void Unregister() = 0;
    virtual ~SceneObjectBase() {}
  };

  using SceneObjectPtr = std::shared_ptr<SceneObjectBase>;

  template <typename Object, typename... Managers>
  void RegisterSceneObject(Object& object, std::tuple<Managers&...> const& managers)
  {
    if constexpr(IsCompoundSceneObject<Object>)
    {
      // skip registration altogether if the object is not interested in any managers
      if constexpr((IsCompoundSceneObjectManagedBy<Object, Managers> || ...))
      {
        // register with only interesting managers
        CompoundSceneObjectHelper<Object, Managers...>::template ApplyFilteredManagers<0>([&]<typename... FilteredManagers>(FilteredManagers&... filteredManagers)
        {
          object.RegisterCompoundSceneObject(std::tuple<FilteredManagers&...>{filteredManagers...});
        }, managers);
      }
    }
    else
    {
      std::apply([&](Managers&... managers)
      {
        ([&](Managers& manager)
        {
          if constexpr(IsSimpleSceneObjectManagedBy<Object, Managers>)
          {
            manager.RegisterObject(object);
          }
        }(managers), ...);
      }, managers);
    }
  }

  template <typename Object, typename... Managers>
  void UnregisterSceneObject(Object& object, std::tuple<Managers&...> const& managers)
  {
    if constexpr(IsCompoundSceneObject<Object>)
    {
      // skip unregistration altogether if the object was not interested in any managers
      if constexpr((IsCompoundSceneObjectManagedBy<Object, Managers> || ...))
      {
        // unregister with only interesting managers
        CompoundSceneObjectHelper<Object, Managers...>::template ApplyFilteredManagers<0>([&]<typename... FilteredManagers>(FilteredManagers&... filteredManagers)
        {
          object.UnregisterCompoundSceneObject(std::tuple<FilteredManagers&...>{filteredManagers...});
        }, managers);
      }
    }
    else
    {
      std::apply([&](Managers&... managers)
      {
        ([&](Managers& manager)
        {
          if constexpr(IsSimpleSceneObjectManagedBy<Object, Managers>)
          {
            manager.UnregisterObject(object);
          }
        }(managers), ...);
      }, managers);
    }
  }

  template <typename... Managers>
  class Scene
  {
  private:
    template <typename Object>
    class SceneObject final : public SceneObjectBase
    {
    public:
      SceneObject(SceneObject const&) = delete;
      SceneObject(SceneObject&&) = delete;

      template <typename... Args>
      SceneObject(Args&&... args)
      : object_{std::forward<Args>(args)...} {}

      ~SceneObject()
      {
        Unregister();
      }

      void Register(Scene const* pScene)
      {
        Unregister();

        pScene_ = pScene;

        RegisterSceneObject<Object, Managers...>(object_, pScene_->managers_);
      }

      void Unregister() override
      {
        if(!pScene_) return;

        UnregisterSceneObject<Object, Managers...>(object_, pScene_->managers_);

        pScene_ = nullptr;
      }

    private:
      Scene const* pScene_ = nullptr;
      Object object_;
    };

  public:
    Scene(Managers&... managers)
    : managers_{managers...} {}

    template <typename Object>
    SceneObjectPtr AddObject(Object&& object)
    {
      auto pObject = std::make_shared<SceneObject<Object>>(std::forward<Object>(object));
      pObjects_.insert(pObject);
      pObject->Register(this);
      return pObject;
    }

    template <typename Object, typename... Args>
    SceneObjectPtr AddObject(Args&&... args)
    {
      auto pObject = std::make_shared<SceneObject<Object>>(std::forward<Args>(args)...);
      pObjects_.insert(pObject);
      pObject->Register(this);
      return pObject;
    }

    template <typename Object>
    void RemoveObject(SceneObjectPtr const& pObject)
    {
      pObject->Unregister();
      pObjects_.erase(pObject);
    }

    ~Scene()
    {
      // unregister objects manually, in case shared pointers are held by someone else
      for(auto const& pObject : pObjects_)
      {
        pObject->Unregister();
      }
      pObjects_.clear();
    }

  private:
    std::tuple<Managers&...> managers_;
    std::unordered_set<SceneObjectPtr> pObjects_;
  };

  template <typename... Objects>
  class CompoundSceneObject
  {
  public:
    CompoundSceneObject(Objects&&... objects)
    : objects_{std::forward<Objects>(objects)...} {}

    template <typename Manager>
    static constexpr bool ManagedBy = (IsSceneObjectManagedBy<Objects, Manager> || ...);

    template <typename... Managers>
    void RegisterCompoundSceneObject(std::tuple<Managers&...> const& managers)
    {
      std::apply([&](Objects&... objects)
      {
        (RegisterSceneObject<Objects, Managers...>(objects, managers), ...);
      }, objects_);
    }

    template <typename... Managers>
    void UnregisterCompoundSceneObject(std::tuple<Managers&...> const& managers)
    {
      std::apply([&](Objects&... objects)
      {
        (UnregisterSceneObject<Objects, Managers...>(objects, managers), ...);
      }, objects_);
    }

  private:
    std::tuple<Objects...> objects_;
  };
}
