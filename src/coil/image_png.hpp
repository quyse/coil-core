#pragma once

#include "graphics.hpp"
#include "tasks.hpp"

namespace Coil
{
  ImageBuffer LoadPngImage(Book& book, InputStream& stream);
  void SavePngImage(OutputStream& stream, ImageBuffer const& imageBuffer);

  class PngAssetLoader
  {
  public:
    template <std::same_as<ImageBuffer> Asset, typename AssetContext>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      BufferInputStream inputStream = co_await assetContext.template LoadAssetParam<Buffer>(book, "buffer");
      co_return LoadPngImage(book, inputStream);
    }

    static constexpr std::string_view assetLoaderName = "image_png";
  };
  static_assert(IsAssetLoader<PngAssetLoader>);
}
