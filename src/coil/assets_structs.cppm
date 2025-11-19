module;

#include <coroutine>
#include <memory>
#include <string>
#include <vector>

export module coil.core.assets.structs;

import coil.core.assets;
import coil.core.base;

export namespace Coil
{
  // asset struct adapter, allowing to load all asset fields using asset loader
  // all field types must have default constructor
  class AssetStructAdapter
  {
  private:
    // helper adapter which registers fields of a struct
    template <typename AssetManager>
    struct RegistrationAdapter
    {
      template <typename FieldType>
      using Field = std::tuple<>;

      template <template <typename> typename StructTemplate>
      class Base;
    };

  public:
    template <typename FieldType>
    using Field = FieldType;

    template <template <typename> typename StructTemplate>
    class Base
    {
    public:
      template <typename AssetManager>
      void SelfLoad(Book& book, AssetManager& assetManager, std::string const& namePrefix = {})
      {
        static StructTemplate<RegistrationAdapter<AssetManager>> const registration;
        for(size_t i = 0; i < registration._fields.size(); ++i)
        {
          registration._fields[i]->SelfLoad(static_cast<StructTemplate<AssetStructAdapter>&>(*this), book, assetManager, namePrefix);
        }
      }

    protected:
      template <typename FieldType, auto fieldPtr, Literal name>
      Field<FieldType> RegisterField()
      {
        return {};
      }
    };
  };

  template <typename AssetManager>
  template <template <typename> typename StructTemplate>
  class AssetStructAdapter::RegistrationAdapter<AssetManager>::Base
  {
  private:
    struct FieldInfoBase
    {
      virtual ~FieldInfoBase() = default;
      virtual void SelfLoad(StructTemplate<AssetStructAdapter>& s, Book& book, AssetManager& assetManager, std::string const& namePrefix) = 0;
    };

    template <typename FieldType>
    struct FieldInfo : public FieldInfoBase
    {
      FieldInfo(std::string name, typename AssetStructAdapter::template Field<FieldType> StructTemplate<AssetStructAdapter>::* ptr)
      : name(std::move(name)), ptr(ptr) {}

      void SelfLoad(StructTemplate<AssetStructAdapter>& s, Book& book, AssetManager& assetManager, std::string const& namePrefix) override
      {
        // handle sub-structs
        if constexpr(requires
        {
          { (s.*ptr).SelfLoad(book, assetManager, namePrefix + name) } -> std::same_as<void>;
        })
        {
          (s.*ptr).SelfLoad(book, assetManager, namePrefix + name);
        }
        // else it's asset field
        else
        {
          s.*ptr = assetManager.template LoadAsset<FieldType>(book, namePrefix + name);
        }
      }

      std::string const name;
      typename AssetStructAdapter::template Field<FieldType> StructTemplate<AssetStructAdapter>::* ptr;
    };

  protected:
    template <typename FieldType, auto fieldPtr, Literal name>
    Field<FieldType> RegisterField()
    {
      _fields.push_back(std::make_unique<FieldInfo<FieldType>>(std::string{name}, fieldPtr.template operator()<StructTemplate<AssetStructAdapter>>()));
      return {};
    };

  private:
    std::vector<std::unique_ptr<FieldInfoBase>> _fields;

    friend AssetStructAdapter;
  };
}
