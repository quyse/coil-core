module;

#include <any>
#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

export module coil.core.assets;

import coil.core.base;
import coil.core.json;

export namespace Coil
{
  // whether asset can be loaded by asset loader
  template <typename Asset, typename AssetLoader, typename AssetContext>
  concept IsAssetLoadable = requires(AssetLoader const& assetLoader, Book& book, AssetContext& assetContext)
  {
    { assetLoader.template LoadAsset<Asset>(book, assetContext) } -> std::same_as<Asset>;
  };

  // class managing loaded assets
  template <IsAssetLoader... AssetLoaders>
  class AssetManager
  {
  public:
    AssetManager() = default;
    AssetManager(AssetLoaders&&... assetLoaders)
    : assetLoaders_{std::move(assetLoaders)...} {}
    AssetManager(std::tuple<AssetLoaders...>&& assetLoaders)
    : assetLoaders_{std::move(assetLoaders)} {}

    void SetJsonContext(Json&& jsonContext)
    {
      jsonContext_ = std::move(jsonContext);
      assetContext_.emplace(*this, jsonContext_);
    }

    // context for asset
    // uses json dictionary for parameters
    class AssetContext
    {
    public:
      AssetContext(AssetManager& assetManager, Json const& context)
      : assetManager_{assetManager}, context_{context} {}

      // load asset represented by context
      template <IsAsset Asset>
      requires (IsAssetLoadable<Asset, AssetLoaders, AssetContext> || ...)
      Asset LoadAsset(Book& book)
      {
        auto assetLoaderName = JsonDecodeField<std::string>(context_, "loader");
        return [&]<size_t i>(this auto const& search) -> Asset
        {
          if constexpr(i < sizeof...(AssetLoaders))
          {
            auto const& assetLoader = std::get<i>(assetManager_.assetLoaders_);
            using AssetLoader = std::decay_t<decltype(assetLoader)>;
            if constexpr(IsAssetLoadable<Asset, AssetLoader, AssetManager>)
            {
              if(AssetLoader::assetLoaderName == assetLoaderName)
              {
                return assetLoader.template LoadAsset<Asset>(book, *this);
              }
            }

            return search.template operator()<i + 1>();
          }
          else
          {
            throw Exception{} << "no asset loader " << assetLoaderName << " for " << typeid(Asset).name();
          }
        }.template operator()<0>();
      }

      // get required parameter
      std::string GetParam(std::string_view paramName)
      {
        return JsonDecodeField<std::string>(context_, paramName);
      }
      // get optional parameter
      std::optional<std::string> GetOptionalParam(std::string_view paramName)
      {
        return JsonDecodeField<std::optional<std::string>>(context_, paramName);
      }
      // get parameter from string
      template <typename T, typename... DefaultArgs>
      T GetFromStringParam(std::string_view paramName, DefaultArgs&&... defaultArgs)
      {
        auto value = JsonDecodeField<std::optional<std::string>>(context_, paramName, {});
        return value.has_value() ? FromString<T>(value.value()) : T(std::forward<DefaultArgs>(defaultArgs)...);
      }
      // get optional parameter from string
      template <typename T>
      std::optional<T> GetOptionalFromStringParam(std::string_view paramName)
      {
        auto value = JsonDecodeField<std::optional<std::string>>(context_, paramName, {});
        if(!value.has_value()) return {};
        return FromString<T>(value.value());
      }

      // load asset represented by parameter
      template <IsAsset Asset>
      Asset LoadAssetParam(Book& book, std::string_view paramName)
      {
        auto i = context_.find(paramName);
        if(i == context_.end())
          throw Exception{} << "no asset param " << paramName;

        auto const& subContext = i.value();
        if(subContext.is_object())
        {
          return AssetContext(assetManager_, subContext).LoadAsset<Asset>(book);
        }
        else
        {
          return assetManager_.LoadAsset<Asset>(book, JsonDecode<std::string>(subContext));
        }
      }

    private:
      AssetManager& assetManager_;
      Json const& context_;
    };

    template <IsAsset Asset>
    Asset LoadAsset(Book& book, std::string const& assetName)
    requires (IsAssetLoadable<Asset, AssetLoaders, AssetManager> || ...)
    {
      // mark the asset as loading or check that it's already started loading or loaded
      auto [i, inserted] = assets_.insert({assetName, std::optional<std::any>{}});

      // if asset does not exist yet
      if(inserted)
      {
        // load asset

        if(!assetContext_.has_value())
          throw Exception{"no asset context set"};

        Asset asset = assetContext_.value().template LoadAssetParam<Asset>(book, assetName);
        i->second = std::any{asset};
        return asset;
      }
      // otherwise get loaded asset
      else
      {
        auto optAsset = i->second;
        // if it's not actually loaded, it's a dependency loop
        if(!optAsset.has_value())
        {
          throw Exception{} << "asset dependency loop detected on " << typeid(Asset).name();
        }

        // safely cast to required asset type
        try
        {
          return std::any_cast<Asset>(optAsset.value());
        }
        catch(std::bad_any_cast const&)
        {
          throw Exception{} << "mistyped cached asset: expected " << typeid(Asset).name() << " but got " << optAsset.value().type().name();
        }
      }
    }

  private:
    std::tuple<AssetLoaders...> const assetLoaders_;

    Json jsonContext_;
    std::optional<AssetContext> assetContext_;

    std::unordered_map<std::string, std::optional<std::any>> assets_;
  };
}
