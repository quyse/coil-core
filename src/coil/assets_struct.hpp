#pragma once

#include "assets.hpp"
#include "util.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Coil
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
      Task<void> SelfLoad(Book& book, AssetManager& assetManager, std::string const& namePrefix = {})
      {
        static StructTemplate<RegistrationAdapter<AssetManager>> const registration;
        std::vector<Task<void>> tasks;
        tasks.reserve(registration._fields.size());
        for(size_t i = 0; i < registration._fields.size(); ++i)
        {
          tasks.push_back(registration._fields[i]->SelfLoad(static_cast<StructTemplate<AssetStructAdapter>&>(*this), book, assetManager, namePrefix));
        }
        for(size_t i = 0; i < tasks.size(); ++i)
        {
          co_await tasks[i];
        }
      }

    protected:
      template <typename FieldType, auto fieldPtr>
      Field<FieldType> RegisterField(std::string_view name)
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
      virtual ~FieldInfoBase() {}
      virtual Task<void> SelfLoad(StructTemplate<AssetStructAdapter>& s, Book& book, AssetManager& assetManager, std::string const& namePrefix) = 0;
    };

    template <typename FieldType>
    struct FieldInfo : public FieldInfoBase
    {
      FieldInfo(std::string&& name, typename AssetStructAdapter::template Field<FieldType> StructTemplate<AssetStructAdapter>::* ptr)
      : name(std::move(name)), ptr(ptr) {}

      Task<void> SelfLoad(StructTemplate<AssetStructAdapter>& s, Book& book, AssetManager& assetManager, std::string const& namePrefix) override
      {
        // handle sub-structs
        if constexpr(requires
        {
          { (s.*ptr).SelfLoad(book, assetManager, namePrefix + name) } -> std::same_as<Task<void>>;
        })
        {
          co_await (s.*ptr).SelfLoad(book, assetManager, namePrefix + name);
        }
        // else it's asset field
        else
        {
          s.*ptr = co_await assetManager.template LoadAsset<FieldType>(book, namePrefix + name);
        }
      }

      std::string const name;
      typename AssetStructAdapter::template Field<FieldType> StructTemplate<AssetStructAdapter>::* ptr;
    };

  protected:
    template <typename FieldType, auto fieldPtr>
    Field<FieldType> RegisterField(std::string&& name)
    {
      _fields.push_back(std::make_unique<FieldInfo<FieldType>>(std::move(name), fieldPtr.template operator()<StructTemplate<AssetStructAdapter>>()));
      return {};
    };

  private:
    std::vector<std::unique_ptr<FieldInfoBase>> _fields;

    friend AssetStructAdapter;
  };
}
