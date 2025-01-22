module;

#include <any>
#include <concepts>
#include <mutex>
#include <string_view>
#include <unordered_map>

export module coil.core.assets;

import coil.core.base;
import coil.core.json;
import coil.core.tasks;

export namespace Coil
{
  // whether asset can be loaded by asset loader
  template <typename Asset, typename AssetLoader, typename AssetContext>
  concept IsAssetLoadable = requires(AssetLoader const& assetLoader, Book& book, AssetContext& assetContext)
  {
    { assetLoader.template LoadAsset<Asset>(book, assetContext) } -> std::same_as<Task<Asset>>;
  };

  // class managing loaded assets
  template <IsAssetLoader... AssetLoaders>
  class AssetManager
  {
  public:
    AssetManager() = default;
    AssetManager(AssetLoaders&&... assetLoaders)
    : _assetLoaders(std::move(assetLoaders)...) {}

    void SetJsonContext(Json&& jsonContext)
    {
      _jsonContext = std::move(jsonContext);
      _assetContext.emplace(*this, _jsonContext);
    }

    // context for asset
    // uses json dictionary for parameters
    class AssetContext
    {
    public:
      AssetContext(AssetManager& assetManager, Json const& context)
      : _assetManager(assetManager), _context(context) {}

      // load asset represented by context
      template <IsAsset Asset>
      Task<Asset> LoadAsset(Book& book)
      requires (IsAssetLoadable<Asset, AssetLoaders, AssetContext> || ...)
      {
        return TryLoadAsset<Asset, 0>(book, JsonDecodeField<std::string>(_context, "loader"));
      }

      // get required parameter
      std::string GetParam(std::string_view paramName)
      {
        return JsonDecodeField<std::string>(_context, paramName);
      }
      // get optional parameter
      std::optional<std::string> GetOptionalParam(std::string_view paramName)
      {
        return JsonDecodeField<std::optional<std::string>>(_context, paramName);
      }
      // get parameter from string
      template <typename T, typename... DefaultArgs>
      T GetFromStringParam(std::string_view paramName, DefaultArgs&&... defaultArgs)
      {
        auto value = JsonDecodeField<std::optional<std::string>>(_context, paramName, {});
        return value.has_value() ? FromString<T>(value.value()) : T(std::forward<DefaultArgs>(defaultArgs)...);
      }
      // get optional parameter from string
      template <typename T>
      std::optional<T> GetOptionalFromStringParam(std::string_view paramName)
      {
        auto value = JsonDecodeField<std::optional<std::string>>(_context, paramName, {});
        if(!value.has_value()) return {};
        return FromString<T>(value.value());
      }

      // load asset represented by parameter
      template <IsAsset Asset>
      Task<Asset> LoadAssetParam(Book& book, std::string_view paramName)
      {
        auto i = _context.find(paramName);
        if(i == _context.end())
          throw Exception() << "no asset param " << paramName;

        auto const& subContext = i.value();
        if(subContext.is_object())
        {
          co_return co_await AssetContext(_assetManager, subContext).LoadAsset<Asset>(book);
        }
        else
        {
          co_return co_await _assetManager.LoadAsset<Asset>(book, JsonDecode<std::string>(subContext));
        }
      }

    private:
      template <typename Asset, size_t i>
      Task<Asset> TryLoadAsset(Book& book, std::string_view assetLoaderName)
      {
        if constexpr(i < sizeof...(AssetLoaders))
        {
          auto const& assetLoader = std::get<i>(_assetManager._assetLoaders);
          using AssetLoader = std::decay_t<decltype(assetLoader)>;
          if constexpr(IsAssetLoadable<Asset, AssetLoader, AssetManager>)
          {
            if(AssetLoader::assetLoaderName == assetLoaderName)
            {
              return assetLoader.template LoadAsset<Asset>(book, *this);
            }
          }

          return TryLoadAsset<Asset, i + 1>(book, assetLoaderName);
        }
        else
        {
          throw Exception() << "no asset loader " << assetLoaderName << " for " << typeid(Asset).name();
        }
      }

      AssetManager& _assetManager;
      Json const& _context;
    };

    template <IsAsset Asset>
    Task<Asset> LoadAsset(Book& book, std::string const& assetName)
    requires (IsAssetLoadable<Asset, AssetLoaders, AssetManager> || ...)
    {
      std::unique_lock lock(_mutex);

      // if task already started
      auto i = _assets.find(assetName);
      if(i != _assets.end())
      {
        // wait for task to finish
        auto task = i->second;
        lock.unlock();

        auto asset = co_await task;

        // safely cast to required asset type
        try
        {
          co_return std::any_cast<Asset>(asset);
        }
        catch(std::bad_any_cast const&)
        {
          throw Exception() << "mistyped cached asset: expected " << typeid(Asset).name() << " but got " << asset.type().name();
        }
      }
      else
      {
        // otherwise start task

        if(!_assetContext.has_value())
          throw Exception("no asset context set");

        auto assetTask = _assetContext.value().template LoadAssetParam<Asset>(book, assetName);
        _assets.insert(
        {
          assetName,
          [](auto assetTask) -> Task<std::any>
          {
            co_return std::any(co_await assetTask);
          }(assetTask),
        });
        lock.unlock();

        co_return co_await assetTask;
      }
    }

  private:
    std::tuple<AssetLoaders...> const _assetLoaders;

    Json _jsonContext;
    std::optional<AssetContext> _assetContext;

    std::mutex _mutex;
    std::unordered_map<std::string, Task<std::any>> _assets;
  };
}
