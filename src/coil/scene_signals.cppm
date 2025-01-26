module;

#include <optional>

export module coil.core.scene.signals;

export namespace Coil
{
  // base signal class, doesn't know what type it returns,
  // but knows what signals depend on it
  class SignalBase
  {
  public:
    SignalBase() = default;
    SignalBase(SignalBase const&) = delete;
    SignalBase(SignalBase&&) = delete;
    SignalBase& operator=(SignalBase const&) = delete;
    SignalBase& operator=(SignalBase&&) = delete;

    virtual ~SignalBase() {}

  protected:
    void Recalculate()
    {
      if(dirty_) return;
      dirty_ = true;
      RecalculateDependants();
    }

    void RecalculateDependants() const
    {
      dependants_.Recalculate();
    }

    struct Cell;

    struct CellHead
    {
      void Recalculate()
      {
        for(Cell* pCell = pNext_; pCell; pCell = pCell->pNext_)
        {
          pCell->pSignal_->Recalculate();
        }
      }

      Cell* pNext_ = nullptr;
    };

    struct Cell : public CellHead
    {
      Cell(SignalBase* pSignal)
      : pSignal_{pSignal} {}

      ~Cell()
      {
        Remove();
      }

      void AddTo(CellHead& head)
      {
        Remove();
        pNext_ = head.pNext_;
        head.pNext_ = this;
      }

      void Remove()
      {
        if(!pNext_ && !pPrev_) return;
        if(pNext_) pNext_->pPrev_ = pPrev_;
        if(pPrev_) pPrev_->pNext_ = pNext_;
        pNext_ = nullptr;
        pPrev_ = nullptr;
      }

      CellHead* pPrev_ = nullptr;
      SignalBase* pSignal_ = nullptr;
    };

    template <typename T>
    struct Dependency;

    mutable CellHead dependants_;
    mutable bool dirty_ = true;
  };

  // base typed signal class, knows what type it returns,
  // but doesn't know how or what it depends on
  template <typename T>
  class Signal : public SignalBase
  {
  public:
    virtual T const& Get() const = 0;
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

    T Get() const
    {
      return this->get()->Get();
    }
  };

  template <typename T>
  struct SignalBase::Dependency
  {
    Dependency(SignalPtr<T> pSignal, SignalBase* pSelfSignal)
    : pSignal_{std::move(pSignal)}, cell_{pSelfSignal}
    {}

    // to be called when `this` address became stable
    // not in constructor, because can be moved afterwards
    void Init()
    {
      cell_.AddTo(pSignal_->dependants_);
    }

    SignalPtr<T> pSignal_;
    Cell cell_;
  };

  template <typename T>
  class ConstSignal final : public Signal<T>
  {
  public:
    ConstSignal(T&& value)
    : value_{std::forward<T>(value)} {}

    T const& Get() const override
    {
      return value_;
    }

  private:
    T value_;
  };

  template <typename T>
  class VariableSignal final : public Signal<T>
  {
  public:
    VariableSignal(T value = {})
    : value_{std::move(value)} {}

    template <typename TT>
    VariableSignal(TT&& value)
    : value_(std::forward<TT>(value)) {}

    T const& Get() const override
    {
      return value_;
    }

    template <typename TT>
    void Set(TT&& arg)
    {
      T newValue = std::forward<TT>(arg);
      if(value_ != newValue)
      {
        value_ = std::move(newValue);
        this->dirty_ = false;
        this->RecalculateDependants();
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
    , dependencies_{{std::move(pSignals), this}...}
    {
      std::apply([](SignalBase::Dependency<Args>&... dependencies)
      {
        (dependencies.Init(), ...);
      }, dependencies_);
    }

    T const& Get() const override
    {
      if(!optValue_.has_value() || this->dirty_)
      {
        T newValue = std::apply([&](SignalBase::Dependency<Args> const&... dependencies) -> T
        {
          return f_(dependencies.pSignal_->Get()...);
        }, dependencies_);
        this->dirty_ = false;
        if(!optValue_.has_value() || optValue_.value() != newValue)
        {
          optValue_ = {std::move(newValue)};
          this->RecalculateDependants();
        }
      }
      return optValue_.value();
    }

  private:
    F f_;
    std::tuple<SignalBase::Dependency<Args>...> dependencies_;
    mutable std::optional<T> optValue_;
  };

  template <typename T>
  class SignalVarPtr : public SignalPtr<T>
  {
  public:
    SignalVarPtr(std::shared_ptr<VariableSignal<T>> pSignal)
    : SignalVarPtr::SignalPtr{std::move(pSignal)}
    {}

    void Set(T const& value)
    {
      static_cast<VariableSignal<T>*>(this->get())->Set(value);
    }
  };

  template <typename T>
  SignalPtr<std::decay_t<T>> MakeConstSignal(T&& value)
  {
    return {std::make_shared<ConstSignal<std::decay_t<T>>>(std::move(value))};
  }

  template <typename T>
  SignalVarPtr<std::decay_t<T>> MakeVariableSignal(T&& value = {})
  {
    return {std::make_shared<VariableSignal<std::decay_t<T>>>(std::move(value))};
  }

  template <typename F, typename... Args>
  auto MakeDependentSignal(F&& f, SignalPtr<Args>... args) -> SignalPtr<decltype(f(args->Get()...))>
  {
    return
    {
      std::make_shared<
        DependentSignal<
          std::decay_t<decltype(f(args->Get()...))>,
          std::decay_t<F>,
          std::decay_t<Args>...
        >
      >(std::forward<F>(f), std::move(args)...)
    };
  }

  // convenience operators
#define OP(op) \
  template <typename A, typename B> \
  auto operator op(SignalPtr<A> const& a, SignalPtr<B> const& b) -> SignalPtr<decltype(std::declval<A>() op std::declval<B>())> \
  { \
    return MakeDependentSignal([](auto const& a, auto const& b) { return a op b; }, a, b); \
  }

  OP(+)
  OP(-)
  OP(*)
  OP(/)

#undef OP
}
