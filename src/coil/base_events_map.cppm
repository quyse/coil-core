module;

#include <optional>
#include <unordered_map>

export module coil.core.base.events.map;

import coil.core.base.events;
import coil.core.base.ptr;
import coil.core.base.signals;
import coil.core.base.util;

export namespace Coil
{
  template <IsDecayed Key, IsDecayed Value>
  class SyncMap;

  template <IsDecayed Key, IsDecayed Value>
  using SyncMapPtr = Ptr<SyncMap<Key, Value>>;

  template <IsDecayed Key>
  using SyncSetPtr = SyncMapPtr<Key, std::tuple<>>;

  template <IsDecayed Key, IsDecayed Value>
  class SyncMap : public PtrCountedBase
  {
  public:
    SyncMap() = default;
    SyncMap(SyncMap const&) = delete;
    SyncMap(SyncMap&&) = delete;
    SyncMap& operator=(SyncMap const&) = delete;
    SyncMap& operator=(SyncMap&&) = delete;

    Value* Get(ConstRefExceptScalarOf<Key> key)
    {
      auto i = items_.find(key);
      return i != items_.end() ? &i->second : nullptr;
    }

    template <typename KeyArg>
    Value* Set(KeyArg&& key, std::optional<Value> optValue)
    {
      auto i = items_.find(key);
      if(i != items_.end())
      {
        if(optValue.has_value())
        {
          i->second = std::move(optValue).value();
          // modify event
          GetEvent()->Notify(i->first, &i->second);
          return &i->second;
        }
        else
        {
          items_.erase(i);
          // delete event
          GetEvent()->Notify(key, nullptr);
          return nullptr;
        }
      }
      else
      {
        if(optValue.has_value())
        {
          auto [j, inserted] = items_.insert({std::forward<KeyArg>(key), std::move(optValue).value()});
          // add event
          GetEvent()->Notify(j->first, &j->second);
          return &j->second;
        }
        else
        {
          return nullptr;
        }
      }
    }

    template <typename F>
    bool With(ConstRefExceptScalarOf<Key> key, F const& f) requires requires
    {
      f(std::declval<Value&>());
    }
    {
      auto i = items_.find(key);
      if(i == items_.end()) return false;
      f(i->second);
      return true;
    }

    // range-based for loop support
    auto begin() { return items_.begin(); }
    auto begin() const { return items_.begin(); }
    auto end() { return items_.end(); }
    auto end() const { return items_.end(); }

    template <typename Handler>
    class MappedMap;

    template <typename Handler>
    using MappedValue = decltype(std::declval<Handler>()(std::declval<Key>(), std::declval<Value>()));

    template <typename Handler>
    SyncMapPtr<Key, MappedValue<std::decay_t<Handler>>> Map(Handler&& handler) const
    {
      auto pMap = Ptr<MappedMap<std::decay_t<Handler>>>::Make(std::forward<Handler>(handler), GetEvent());
      for(auto const& [key, value] : items_)
      {
        pMap->Notify(key, &value);
      }
      return SyncMapPtr<Key, MappedValue<std::decay_t<Handler>>>{pMap};
    }

    EventPtr<Key, Value const*> const& GetEvent() const
    {
      if(!pEvent_)
      {
        pEvent_ = MakeEvent<Key, Value const*>();
      }
      return pEvent_;
    }

    SignalPtr<std::unordered_map<Key, Value>> CreateSignal()
    {
      return MakeSignalDependentOnEvent(MakeEventDependentOnEvent([self = PtrSelf()](Key, Value const*)
      {
        return self->items_;
      }, GetEvent()), items_);
    }

  private:
    std::unordered_map<Key, Value> items_;
    mutable EventPtr<Key, Value const*> pEvent_;
  };

  template <IsDecayed Key, IsDecayed Value>
  template <typename Handler>
  class SyncMap<Key, Value>::MappedMap final : public SyncMap<Key, MappedValue<Handler>>
  {
  private:
    class Cell : public Event<Key, Value const*>::Cell
    {
    public:
      Cell(MappedMap& self)
      : self_{self} {}

      void Notify(ConstRefExceptScalarOf<Key> key, Value const* pValue) override
      {
        self_.Notify(key, pValue);
      }

    private:
      MappedMap& self_;
    };

  public:
    template <typename HandlerArg>
    MappedMap(HandlerArg&& handler, EventPtr<Key, Value const*> const& pEvent)
    : cell_{*this}
    , handler_{std::forward<HandlerArg>(handler)}
    {
      cell_.SubscribeTo(pEvent);
    }

    void Notify(ConstRefExceptScalarOf<Key> key, Value const* pValue)
    {
      if(pValue)
      {
        this->Set(key, {handler_(key, *pValue)});
      }
      else
      {
        this->Set(key, {});
      }
    }

  private:
    Cell cell_;
    Handler handler_;
  };

  template <IsDecayed Key, IsDecayed Value>
  SyncMapPtr<Key, Value> MakeSyncMap()
  {
    return Ptr<SyncMap<Key, Value>>::Make();
  }

  template <IsDecayed Key>
  SyncSetPtr<Key> MakeSyncSet()
  {
    return Ptr<SyncMap<Key, std::tuple<>>>::Make();
  }
}
