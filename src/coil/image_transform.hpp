#pragma once

#include "image_format.hpp"
#include "tasks.hpp"

namespace Coil
{
  ImageBuffer GenerateImageMips(Book& book, ImageBuffer const& imageBuffer);

  class ImageMipsAssetLoader
  {
  public:
    template <std::same_as<ImageBuffer> Asset, typename AssetContext>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      co_return GenerateImageMips(book, co_await assetContext.template LoadAssetParam<ImageBuffer>(book, "image"));
    }

    static constexpr std::string_view assetLoaderName = "image_mips";
  };
  static_assert(IsAssetLoader<ImageMipsAssetLoader>);
}
