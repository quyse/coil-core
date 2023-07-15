#pragma once

#include "image_format.hpp"
#include "tasks.hpp"

namespace Coil
{
  ImageBuffer CompressImage(Book& book, ImageBuffer const& imageBuffer, PixelFormat::Compression compression);

  class ImageCompressAssetLoader
  {
  public:
    template <std::same_as<ImageBuffer> Asset, typename AssetContext>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      auto image = co_await assetContext.template LoadAssetParam<ImageBuffer>(book, "image");
      auto compressionParam = assetContext.template GetOptionalFromStringParam<PixelFormat::Compression>("compression");
      PixelFormat::Compression compression;
      if(compressionParam.has_value())
      {
        compression = compressionParam.value();
      }
      else
      {
        switch(image.format.format.components)
        {
        case PixelFormat::Components::R:
          compression = PixelFormat::Compression::Bc4;
          break;
        case PixelFormat::Components::RG:
          compression = PixelFormat::Compression::Bc5;
          break;
        case PixelFormat::Components::RGB:
          compression = PixelFormat::Compression::Bc1;
          break;
        case PixelFormat::Components::RGBA:
          compression = PixelFormat::Compression::Bc3;
          break;
        }
      }
      co_return CompressImage(book, image, compression);
    }

    static constexpr std::string_view assetLoaderName = "image_compress";
  };
  static_assert(IsAssetLoader<ImageCompressAssetLoader>);
}
