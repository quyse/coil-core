module;

#include <string_view>
#include <vector>

export module coil.core.image.transform;

import coil.core.base;
import coil.core.image.format;
import coil.core.image;
import coil.core.math;

export namespace Coil
{
  ImageBuffer GenerateImageMips(Book& book, ImageBuffer const& imageBuffer)
  {
    if(imageBuffer.format.format.type != PixelFormat::Type::Uncompressed)
      throw Exception("Image to generate mips must be uncompressed");
    if(!(imageBuffer.format.width > 0 && imageBuffer.format.height > 0 && imageBuffer.format.depth == 0))
      throw Exception("Image to generate mips must be 2D");
    if(!(
      ((imageBuffer.format.width - 1) & imageBuffer.format.width) == 0 ||
      ((imageBuffer.format.height - 1) & imageBuffer.format.height) == 0))
      throw Exception("Image to generate mips must be power-of-two");

    std::vector<uint8_t> resultPixels;

    ImageFormat format = imageBuffer.format;

    ProcessImageBufferMips(imageBuffer, [&]<typename T, size_t n>(RawImageSlice<T, n> const& imageSlice, size_t imageIndex, size_t mipIndex)
    {
      // process only top mip
      if(mipIndex > 0) return;

      RawImageSlice<T, n> currentImageSlice;
      RawImage<T, n> currentImage;

      size_t i;
      for(i = 0; (imageSlice.size(0) >> i) > 0 || (imageSlice.size(1) >> i) > 0; ++i)
      {
        if(i <= 0)
        {
          currentImageSlice = imageSlice;
        }
        else
        {
          currentImage = currentImageSlice.template DownSample<ScalarOrVector<float, VectorTraits<T>::N>>({ 2, 2 });
          currentImageSlice = currentImage;
        }
        size_t offset = resultPixels.size();
        size_t size = currentImageSlice.size(1) * currentImageSlice.pitch(1) * sizeof(T);
        resultPixels.resize(offset + size);
        RawImageSlice<T, n>
        {
          .pixels = (T*)(resultPixels.data() + offset),
          .size = currentImageSlice.size,
          .pitch = { 1, currentImageSlice.size(0) },
        }.Blit(currentImageSlice, {}, {}, currentImageSlice.size);
      }
      format.mips = i;
    });

    return
    {
      .format = format,
      .buffer = book.Allocate<std::vector<uint8_t>>(std::move(resultPixels)),
    };
  }

  class ImageMipsAssetLoader
  {
  public:
    template <std::same_as<ImageBuffer> Asset, typename AssetContext>
    Asset LoadAsset(Book& book, AssetContext& assetContext) const
    {
      return GenerateImageMips(book, assetContext.template LoadAssetParam<ImageBuffer>(book, "image"));
    }

    static constexpr std::string_view assetLoaderName = "image_mips";
  };
  static_assert(IsAssetLoader<ImageMipsAssetLoader>);
}
