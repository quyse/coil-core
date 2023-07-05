#include "image_compress.hpp"
#include "image_typed.hpp"
#include <concepts>
#include <squish.h>

namespace Coil
{
  ImageBuffer CompressImage(Book& book, ImageBuffer const& imageBuffer, PixelFormat::Compression compression)
  {
    if(imageBuffer.format.format.type != PixelFormat::Type::Uncompressed)
      throw Exception("Image to compress must be uncompressed");
    if(!(imageBuffer.format.width > 0 && imageBuffer.format.height > 0 && imageBuffer.format.depth == 0))
      throw Exception("Image to compress must be 2D");

    // convert to RGBA 8 bit per channel
    auto withRGBA8 = [&]<typename T, size_t n>(RawImageSlice<T, n> const& imageSlice, auto const& f)
    {
      if constexpr(std::same_as<T, xvec_ua<uint8_t, 4>>)
      {
        f(imageSlice);
      }
      else
      {
        RawImage<xvec_ua<uint8_t, 4>, n> rgbaImage(imageSlice.size);
        rgbaImage.Blend(imageSlice, {}, {}, imageSlice.size, [](xvec_ua<uint8_t, 4>& dst, T const& src)
        {
          auto srcFun = [&](size_t i) -> typename VectorTraits<T>::Scalar
          {
            if constexpr(VectorTraits<T>::N == 1)
            {
              return src;
            }
            else
            {
              return src(i);
            }
          };

          static_assert(VectorTraits<T>::N <= 4);
          for(size_t i = 0; i < VectorTraits<T>::N; ++i)
          {
            if constexpr(std::same_as<typename VectorTraits<T>::Scalar, uint8_t>)
            {
              dst(i) = srcFun(i);
            }
            else if constexpr(std::integral<typename VectorTraits<T>::Scalar>)
            {
              dst(i) = (uint8_t)(srcFun(i) / (std::numeric_limits<typename VectorTraits<T>::Scalar>::max() / 255));
            }
            else // presume floating point
            {
              dst(i) = (uint8_t)(srcFun(i) * 255);
            }
          }
          for(size_t i = VectorTraits<T>::N; i < 4; ++i)
            dst(i) = i < 3 ? 0 : 255;
        });
        f(rgbaImage);
      }
    };

    std::vector<uint8_t> resultPixels;
    auto process = [&]<PixelFormat::Compression compression, size_t flags>()
    {
      constexpr auto const compressionMetrics = PixelFormat::GetCompressionMetrics(compression);

      auto metrics = imageBuffer.format.GetMetrics();
      std::vector<size_t> compressedMipsSize(metrics.mips.size());
      for(size_t i = 0; i < metrics.mips.size(); ++i)
      {
        auto const& mip = metrics.mips[i];
        compressedMipsSize[i] =
          ((mip.width + compressionMetrics.blockWidth - 1) / compressionMetrics.blockWidth) *
          ((mip.height + compressionMetrics.blockHeight - 1) / compressionMetrics.blockHeight) *
          compressionMetrics.blockSize;
      }

      ProcessImageBufferMips(imageBuffer, [&]<typename T, size_t n>(RawImageSlice<T, n> const& imageSlice, size_t imageIndex, size_t mipIndex)
      {
        if constexpr(n == 2)
        {
          withRGBA8(imageSlice, [&](RawImageSlice<xvec_ua<uint8_t, 4>, n> const& imageSlice)
          {
            size_t offset = resultPixels.size();
            resultPixels.resize(offset + compressedMipsSize[mipIndex]);
            squish::CompressImage(
              (uint8_t const*)imageSlice.pixels,
              imageSlice.size.x(), imageSlice.size.y(),
              imageSlice.pitch.y() * sizeof(xvec_ua<uint8_t, 4>),
              (uint8_t*)resultPixels.data() + offset,
              flags
            );
          });
        }
      });
    };

    switch(compression)
    {
    case PixelFormat::Compression::Bc1:
      process.operator()<PixelFormat::Compression::Bc1, squish::kDxt1 | squish::kColourIterativeClusterFit>();
      break;
    // case PixelFormat::Compression::Bc1Alpha: // not supported by libsquish
    case PixelFormat::Compression::Bc2:
      process.operator()<PixelFormat::Compression::Bc2, squish::kDxt3 | squish::kColourIterativeClusterFit | squish::kWeightColourByAlpha>();
      break;
    case PixelFormat::Compression::Bc3:
      process.operator()<PixelFormat::Compression::Bc3, squish::kDxt5 | squish::kColourIterativeClusterFit | squish::kWeightColourByAlpha>();
      break;
    // case PixelFormat::Compression::Bc4: // TODO
    // case PixelFormat::Compression::Bc4Signed: // TODO
    // case PixelFormat::Compression::Bc5: // TODO
    // case PixelFormat::Compression::Bc5Signed: // TODO
    default:
      throw Exception("Unsupported target image compression");
    }

    ImageFormat format = imageBuffer.format;
    format.format.type = PixelFormat::Type::Compressed;
    format.format.compression = compression;

    return
    {
      .format = format,
      .buffer = book.Allocate<std::vector<uint8_t>>(std::move(resultPixels)),
    };
  }
}
