module;

#include <utility>

export module coil.core.base.ptr;

export namespace Coil
{
  // traits for objects with reference counter
  template <typename T>
  struct PtrTraits;

  template <typename T>
  concept HasPtrTraits = requires(T& object)
  {
    PtrTraits<T>::Reference(object);
    PtrTraits<T>::Dereference(object);
  };

  // thread-unsafe reference counting smart pointer
  template <HasPtrTraits T>
  class Ptr
  {
  public:
    Ptr() = default;
    explicit Ptr(T* ptr)
    : ptr_(ptr)
    {
      PtrTraits<T>::Reference(*ptr_);
    }
    Ptr(Ptr const& other)
    {
      *this = other;
    }
    Ptr(Ptr&& other)
    {
      *this = std::move(other);
    }
    ~Ptr()
    {
      if(ptr_) PtrTraits<T>::Dereference(*ptr_);
    }

    Ptr& operator=(Ptr const& other)
    {
      if(ptr_) PtrTraits<T>::Dereference(*ptr_);
      ptr_ = other.ptr_;
      if(ptr_) PtrTraits<T>::Reference(*ptr_);
      return *this;
    }
    Ptr& operator=(Ptr&& other)
    {
      std::swap(ptr_, other.ptr_);
      return *this;
    }

    T& operator*() const
    {
      return *ptr_;
    }
    T* operator->() const
    {
      return ptr_;
    }
    operator T*() const
    {
      return ptr_;
    }
    operator bool() const
    {
      return !!ptr_;
    }

    template <typename TT>
    requires std::convertible_to<T*, TT*>
    Ptr<TT> StaticCast() const
    {
      return Ptr<TT>(static_cast<TT*>(ptr_));
    }

    template <typename... Args>
    static Ptr<T> Make(Args&&... args)
    {
      return Ptr<T>{new T(std::forward<Args>(args)...)};
    }

    friend auto operator<=>(Ptr const&, Ptr const&) = default;

  protected:
    T* ptr_ = nullptr;
  };

  // base class for objects with embedded reference counter
  class PtrCountedBase
  {
  public:
    // force virtual destructor
    virtual ~PtrCountedBase() = default;

    void PtrReference()
    {
      ++_ptrRefCount;
    }
    void PtrDereference()
    {
      if(!--_ptrRefCount) delete this;
    }

    // pointer to itself
    template <typename Self>
    Ptr<std::remove_reference_t<Self>> PtrSelf(this Self&& self)
    {
      return Ptr<std::remove_reference_t<Self>>{&self};
    }

    size_t _ptrRefCount = 0;
  };

  // concept for object having embedded methods for referencing/dereferencing
  template <typename T>
  concept HasPtrCounting = requires(T& object)
  {
    object.PtrReference();
    object.PtrDereference();
  };

  template <HasPtrCounting T>
  struct PtrTraits<T>
  {
    static void Reference(T& object)
    {
      object.PtrReference();
    }
    static void Dereference(T& object)
    {
      object.PtrDereference();
    }
  };
}
