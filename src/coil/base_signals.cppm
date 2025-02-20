module;

#include <memory>
#include <optional>
#include <ranges>
#include <vector>

export module coil.core.base.signals;

import coil.core.base.events;
import coil.core.base.util;

export namespace Coil
{
  template <typename T>
  class SignalPtr;

  // base signal class, doesn't know what type it returns,
  // but knows what signals depend on it
  class SignalBase : public Event<>
  {
  public:
    SignalBase() = default;

  protected:
    void Notify() override
    {
      if(dirty_) return;
      dirty_ = true;
      Event<>::Notify();
    }

  public:
    // cell which passes notify to its signal
    class SignalCell final : public Cell
    {
    public:
      SignalCell(SignalBase* pSignal)
      : pSignal_{pSignal} {}

      void Notify() override
      {
        pSignal_->Notify();
      }

    private:
      SignalBase* pSignal_ = nullptr;
    };

    // cell which tracks if notify was called
    class WatchCell final : public Cell
    {
    public:
      bool CheckDirty()
      {
        if(dirty_)
        {
          dirty_ = false;
          return true;
        }
        return false;
      }

      void Notify() override
      {
        dirty_ = true;
      }

    private:
      bool dirty_ = false;
    };

  protected:
    template <typename T>
    struct EventDependency;

    template <typename T>
    struct SignalDependency;

    mutable bool dirty_ = true;
  };

  // base typed signal class, knows what type it returns,
  // but doesn't know how or what it depends on
  template <typename T>
  class Signal : public SignalBase
  {
  public:
    virtual ConstRefExceptScalarOf<T> Get() const = 0;
  };

  template <typename T>
  class SignalPtr : public std::shared_ptr<Signal<T>>
  {
  public:
    SignalPtr() = default;
    SignalPtr(SignalPtr const&) = default;
    SignalPtr(SignalPtr&&) = default;
    SignalPtr& operator=(SignalPtr const&) = default;
    SignalPtr& operator=(SignalPtr&&) = default;

    SignalPtr(std::shared_ptr<Signal<T>> pSignal)
    : SignalPtr::shared_ptr{std::move(pSignal)}
    {}

    template <typename S>
    SignalPtr(std::shared_ptr<S> pSignal)
    : SignalPtr::shared_ptr{std::move(pSignal)}
    {}

    ConstRefExceptScalarOf<T> Get() const
    {
      return this->get()->Get();
    }

    template <typename Self>
    operator EventPtr<>(this Self&& self)
    {
      return std::static_pointer_cast<Event<>>(std::forward<Self>(self));
    }
  };

  // dependency for watching another signal
  template <typename T>
  struct SignalBase::SignalDependency
  {
    SignalDependency(SignalPtr<T> pSignal, SignalBase* pSelfSignal)
    : pSignal_{std::move(pSignal)}, cell_{pSelfSignal}
    {}

    // to be called when `this` address became stable
    // not in constructor, because can be moved afterwards
    void Init()
    {
      cell_.SubscribeTo(pSignal_);
    }

    SignalPtr<T> pSignal_;
    SignalCell cell_;
  };

  template <typename T>
  class ConstSignal final : public Signal<T>
  {
  public:
    ConstSignal(T&& value)
    : value_{std::forward<T>(value)} {}

    ConstRefExceptScalarOf<T> Get() const override
    {
      return value_;
    }

  private:
    T value_;
  };

  template <typename T>
  class VariableSignal : public Signal<T>
  {
  public:
    VariableSignal(T value = {})
    : value_{std::move(value)} {}

    template <typename TT>
    VariableSignal(TT&& value)
    : value_(std::forward<TT>(value)) {}

    ConstRefExceptScalarOf<T> Get() const override
    {
      return value_;
    }

    template <typename TT>
    void Set(TT&& arg)
    {
      value_ = std::forward<TT>(arg);
      this->dirty_ = false;
      this->NotifyDependants();
    }

    template <typename TT>
    bool SetIfDiffers(TT&& arg) requires requires
    {
      { this->value_ != arg } -> std::same_as<bool>;
    }
    {
      if(value_ != arg)
      {
        Set<TT>(std::forward<TT>(arg));
        return true;
      }
      else
      {
        return false;
      }
    }

  private:
    T value_;
  };

  template <typename T, typename F, typename... Args>
  class DependentSignal final : public Signal<T>
  {
  public:
    template <typename FF, typename... AArgs>
    DependentSignal(FF&& f, SignalPtr<AArgs>... pSignals)
    : f_{std::forward<FF>(f)}
    , dependencies_(SignalBase::SignalDependency{std::move(pSignals), this}...)
    {
      std::apply([](SignalBase::SignalDependency<Args>&... dependencies)
      {
        (dependencies.Init(), ...);
      }, dependencies_);
    }

    ConstRefExceptScalarOf<T> Get() const override
    {
      if(!optValue_.has_value() || this->dirty_)
      {
        optValue_ = {std::apply([&](SignalBase::SignalDependency<Args> const&... dependencies) -> T
        {
          return f_(dependencies.pSignal_->Get()...);
        }, dependencies_)};
        this->dirty_ = false;
      }
      return optValue_.value();
    }

  private:
    F f_;
    std::tuple<SignalBase::SignalDependency<Args>...> dependencies_;
    mutable std::optional<T> optValue_;
  };

  template <typename T>
  class SignalUnpackingSignal final : public Signal<T>
  {
  public:
    SignalUnpackingSignal(SignalPtr<SignalPtr<T>> pSignalSignal)
    : pSignalSignal_{std::move(pSignalSignal)}
    , cellSigSig_{this}
    , cellSig_{this}
    {
      cellSigSig_.SubscribeTo(pSignalSignal_);
      cellSig_.SubscribeTo(pSignalSignal_->Get());
    }

    ConstRefExceptScalarOf<T> Get() const override
    {
      if(!optSignal_.has_value() || !optValue_.has_value() || this->dirty_)
      {
        optSignal_ = {pSignalSignal_.Get()};
        optValue_ = {optSignal_.value().Get()};
        cellSig_.SubscribeTo(optSignal_.value());
        this->dirty_ = false;
      }
      return optValue_.value();
    }

  private:
    SignalPtr<SignalPtr<T>> pSignalSignal_;
    mutable SignalBase::SignalCell cellSigSig_;
    mutable SignalBase::SignalCell cellSig_;
    mutable std::optional<SignalPtr<T>> optSignal_;
    mutable std::optional<T> optValue_;
  };

  template <typename F, typename A, typename R = decltype(std::declval<F>()(std::declval<std::views::empty<A>>()))>
  class SignalDependentOnRange final : public Signal<R>
  {
  public:
    template <typename FF, std::ranges::range Args>
    SignalDependentOnRange(FF&& f, Args&& pSignals)
    : f_{std::forward<FF>(f)}
    {
      if constexpr(std::ranges::sized_range<Args>)
      {
        dependencies_.reserve(std::ranges::size(pSignals));
      }
      for(auto const& pSignal : pSignals)
      {
        dependencies_.push_back({pSignal, this});
      }
      for(size_t i = 0; i < dependencies_.size(); ++i)
      {
        dependencies_[i].Init();
      }
    }

    ConstRefExceptScalarOf<R> Get() const override
    {
      if(!optValue_.has_value() || this->dirty_)
      {
        optValue_ = {f_(dependencies_ | std::views::transform([](SignalBase::SignalDependency<A> const& dependency) -> auto
        {
          return dependency.pSignal_->Get();
        }))};
        this->dirty_ = false;
      }
      return optValue_.value();
    }

  private:
    F f_;
    std::vector<SignalBase::SignalDependency<A>> dependencies_;
    mutable std::optional<R> optValue_;
  };

  template <typename T>
  class SignalVarPtr : public SignalPtr<T>
  {
  public:
    SignalVarPtr(std::shared_ptr<VariableSignal<T>> pSignal)
    : SignalVarPtr::SignalPtr{std::move(pSignal)}
    {}

    template <typename TT>
    void Set(TT&& arg) const
    {
      static_cast<VariableSignal<T>*>(this->get())->Set(std::forward<TT>(arg));
    }

    template <typename TT>
    bool SetIfDiffers(TT&& arg) const requires requires
    {
      { static_cast<VariableSignal<T>*>(this->get())->SetIfDiffers(std::forward<TT>(arg)) } -> std::same_as<bool>;
    }
    {
      return static_cast<VariableSignal<T>*>(this->get())->SetIfDiffers(std::forward<TT>(arg));
    }
  };

  // dependency for watching single-arg event
  template <typename T>
  class SignalDependentOnEvent : public VariableSignal<T>
  {
  private:
    class SignalEventCell final : public Event<T>::Cell
    {
    public:
      SignalEventCell(SignalDependentOnEvent* pSignal)
      : pSignal_{pSignal} {}

      void Notify(ConstRefExceptScalarOf<T> arg) override
      {
        pSignal_->Set(arg);
      }

    private:
      SignalDependentOnEvent* pSignal_ = nullptr;
    };

  public:
    SignalDependentOnEvent(EventPtr<T> pEvent, T initialValue = {})
    : SignalDependentOnEvent::VariableSignal{std::move(initialValue)}, pEvent_{std::move(pEvent)}, cell_{this}
    {
      cell_.SubscribeTo(pEvent_);
    }

    template <typename TT>
    SignalDependentOnEvent(EventPtr<T> pEvent, TT&& initialValue)
    : SignalDependentOnEvent::VariableSignal{std::forward<TT>(initialValue)}, pEvent_{std::move(pEvent)}, cell_{this}
    {
      cell_.SubscribeTo(pEvent_);
    }

    EventPtr<T> pEvent_;
    SignalEventCell cell_;
  };

  template <typename T>
  SignalPtr<std::decay_t<T>> MakeConstSignal(T&& value)
  {
    return std::make_shared<ConstSignal<std::decay_t<T>>>(std::move(value));
  }

  template <typename T>
  SignalVarPtr<std::decay_t<T>> MakeVariableSignal(T&& value = {})
  {
    return std::make_shared<VariableSignal<std::decay_t<T>>>(std::move(value));
  }

  template <typename T, typename TT = T>
  SignalPtr<T> MakeSignalDependentOnEvent(EventPtr<T> pEvent, TT&& initialValue = {})
  {
    return std::make_shared<SignalDependentOnEvent<T>>(std::move(pEvent), std::forward<TT>(initialValue));
  }

  template <typename F, typename... Args>
  auto MakeSignalDependentOnSignals(F&& f, SignalPtr<Args>... args) -> SignalPtr<decltype(f(args->Get()...))>
  {
    return std::make_shared<
      DependentSignal<
        std::decay_t<decltype(f(args->Get()...))>,
        std::decay_t<F>,
        std::decay_t<Args>...
      >
    >(std::forward<F>(f), std::move(args)...);
  }

  template <typename T>
  SignalPtr<T> MakeSignalUnpackingSignal(SignalPtr<SignalPtr<T>> pSignal)
  {
    return std::make_shared<SignalUnpackingSignal<T>>(pSignal);
  }

  template <
    typename F,
    std::ranges::range Args,
    typename A = std::decay_t<decltype(std::declval<std::ranges::range_value_t<Args>>()->Get())>,
    typename R = std::decay_t<decltype(std::declval<F>()(std::views::empty<A>))>
  >
  SignalPtr<R> MakeSignalDependentOnRange(F&& f, Args&& args)
  {
    return std::make_shared<SignalDependentOnRange<std::decay_t<F>, A, R>>(std::forward<F>(f), std::forward<Args>(args));
  }
}
