module;

#include <utility>

export module coil.core.base.events;

import coil.core.base.ptr;
import coil.core.base.util;

namespace Coil
{
  // helper for skipping argument if it's void
  template <template <typename...> typename T, typename... Args>
  struct EventArgHelper
  {
    using Result = T<Args...>;
  };
  template <template <typename...> typename T, typename... Args>
  struct EventArgHelper<T, void, Args...>
  {
    using Result = typename EventArgHelper<T, Args...>::Result;
  };

  template <typename Handler, typename... Args>
  using EventHandlerResult = decltype(std::declval<Handler>()(std::declval<Args>()...));
}

export namespace Coil
{
  template <typename... Args>
  class EventPtr;

  template <typename... Args>
  class Event : public PtrCountedBase
  {
  public:
    using ArgsTuple = std::tuple<Args...>;

    Event() = default;
    // events are non-copyable and non-movable
    Event(Event const&) = delete;
    Event(Event&&) = delete;
    Event& operator=(Event const&) = delete;
    Event& operator=(Event&&) = delete;

    virtual ~Event() = default;

    virtual void Notify(ConstRefExceptScalarOf<Args>... args)
    {
      NotifyDependants(args...);
    }

  protected:
    void NotifyDependants(ConstRefExceptScalarOf<Args>... args) const
    {
      dependants_.NotifyAllNext(args...);
    }

  public:
    class Cell;

    // just a head of a linked list of event cells
    class CellHead
    {
    public:
      ~CellHead()
      {
        if(pNext_) pNext_->pPrev_ = nullptr;
      }

      void NotifyAllNext(ConstRefExceptScalarOf<Args>... args) const
      {
        for(Cell* pCell = pNext_; pCell; pCell = pCell->pNext_)
        {
          pCell->Notify(args...);
        }
      }

    private:
      Cell* pNext_ = nullptr;

      friend Cell;
    };

    // an event cell, capable of being in a linked list of cells
    class Cell : public CellHead
    {
    public:
      ~Cell()
      {
        Remove();
      }

      // convenience method
      void SubscribeTo(Ptr<Event> const& pEvent)
      {
        AddTo(pEvent->dependants_);
      }

      void AddTo(CellHead& head)
      {
        Remove();
        this->pNext_ = head.pNext_;
        if(this->pNext_) this->pNext_->pPrev_ = this;
        head.pNext_ = this;
        pPrev_ = &head;
      }

      void Remove()
      {
        if(!this->pNext_ && !pPrev_) return;
        if(this->pNext_) this->pNext_->pPrev_ = pPrev_;
        if(pPrev_) pPrev_->pNext_ = this->pNext_;
        this->pNext_ = nullptr;
        pPrev_ = nullptr;
      }

      virtual void Notify(ConstRefExceptScalarOf<Args>... args) = 0;

    private:
      CellHead* pPrev_ = nullptr;

      friend CellHead;
    };

    // cell which passes events to a handler
    template <typename Handler>
    requires requires(Handler handler, ConstRefExceptScalarOf<Args>... args)
    {
      handler(args...);
    }
    class HandlerCell final : public Cell
    {
    public:
      HandlerCell(Handler&& handler)
      : handler_{std::forward<Handler>(handler)} {}

      void Notify(ConstRefExceptScalarOf<Args>... args) override
      {
        handler_(args...);
      }

    private:
      Handler handler_;
    };

    template <typename Handler>
    class DependentEvent;

    template <typename HandlerArg>
    DependentEvent(EventPtr<Args...> const& pEvent, HandlerArg&& handler) -> DependentEvent<std::decay_t<HandlerArg>>;

  protected:
    CellHead dependants_;
  };

  template <typename... Args>
  class EventPtr : public Ptr<Event<Args...>>
  {
  public:
    EventPtr() = default;
    EventPtr(EventPtr const&) = default;
    EventPtr(EventPtr&&) = default;
    EventPtr& operator=(EventPtr const&) = default;
    EventPtr& operator=(EventPtr&&) = default;

    EventPtr(Ptr<Event<Args...>> pEvent)
    : EventPtr::Ptr{std::move(pEvent)}
    {}

    void Notify(ConstRefExceptScalarOf<Args>... args) const
    {
      this->ptr_->Notify(args...);
    }
  };

  template <typename... Args>
  template <typename Handler>
  class Event<Args...>::DependentEvent final : public EventArgHelper<Event, EventHandlerResult<Handler, Args...>>::Result
  {
  public:
    template <typename HandlerArg>
    DependentEvent(EventPtr<Args...> const& pEvent, HandlerArg&& handler)
    : handler_{std::forward<HandlerArg>(handler)}
    , cell_{this}
    {
      cell_.SubscribeTo(pEvent);
    }

  private:
    void NotifyFromDependency(ConstRefExceptScalarOf<Args>... args)
    {
      if constexpr(std::same_as<EventHandlerResult<Handler, Args...>, void>)
      {
        handler_(args...);
        this->Notify();
      }
      else
      {
        this->Notify(handler_(args...));
      }
    }

    class DependencyCell final : public Cell
    {
    public:
      DependencyCell(DependentEvent* pEvent)
      : pEvent_{pEvent} {}

      void Notify(ConstRefExceptScalarOf<Args>... args) override
      {
        pEvent_->NotifyFromDependency(args...);
      }

    private:
      DependentEvent* pEvent_ = nullptr;
    };

    Handler handler_;
    DependencyCell cell_;
  };

  template <typename... Args>
  EventPtr<Args...> MakeEvent()
  {
    return EventPtr<Args...>::Make();
  }

  template <typename Handler, typename... Args>
  EventArgHelper<EventPtr, EventHandlerResult<Handler, Args...>>::Result MakeEventDependentOnEvent(Handler&& handler, EventPtr<Args...> const& pEvent)
  {
    return
      Ptr<typename Event<Args...>::template DependentEvent<std::decay_t<Handler>>>
      ::Make(pEvent, std::forward<Handler>(handler))
      .template StaticCast<typename EventArgHelper<Event, EventHandlerResult<Handler, Args...>>::Result>();
  }
}
