module;

#include <algorithm>
#include <tuple>

export module coil.core.image;

import coil.core.base;
import coil.core.image.format;
import coil.core.math;

export namespace Coil
{
  template <typename T, size_t n>
  class RawImage;

  // view of typed uncompressed image of given dimensionality
  template <typename T, size_t n>
  struct RawImageSlice
  {
    T* pixels = nullptr;
    // size of image in pixels
    ivec<n> size;
    // pitch (1, width, width * height, ...) in pixels
    ivec<n> pitch;

    operator Buffer() const
    {
      return Buffer(pixels, pitch(n - 1) * size(n - 1) * sizeof(T));
    }

    T& operator()(ivec<n> const& c)
    {
      return pixels[dot(c, pitch)];
    }
    T const& operator()(ivec<n> const& c) const
    {
      return pixels[dot(c, pitch)];
    }

    friend void swap(RawImageSlice& a, RawImageSlice& b) noexcept
    {
      std::swap(a.pixels, b.pixels);
      std::swap(a.size, b.size);
      std::swap(a.pitch, b.pitch);
    }

  protected:
    template <typename... Pitches>
    class ProcessHelper
    {
    public:
      ProcessHelper(ivec<n> const& size, Pitches const&... pitches)
      : _size(size), _pitches(pitches...) {}

      template <size_t i, typename F, typename... Offsets>
      void Process(F const& f, Offsets... offsets)
      {
        for(int32_t c = 0; c < _size(i); ++c)
        {
          if constexpr(i > 0)
          {
            Process<i - 1, F, Offsets...>(f, offsets...);
          }
          else
          {
            f(offsets...);
          }
          std::apply([&](Pitches const&... pitches)
          {
            ((offsets += pitches(i)), ...);
          }, _pitches);
        }
      }

    private:
      ivec<n> const _size;
      std::tuple<Pitches...> _pitches;
    };

  public:
    void Blit(RawImageSlice const& image, ivec<n> const& dst, ivec<n> const& src, ivec<n> const& size)
    {
      Blend(image, dst, src, size, [](T& dst, T const& src)
      {
        dst = src;
      });
    }

    template <typename S>
    void Blend(RawImageSlice<S, n> const& image, ivec<n> dst, ivec<n> src, ivec<n> size, auto const& blend)
    {
      // crop image if it goes out of bounds
      for(size_t i = 0; i < n; ++i)
      {
        if(dst(i) < 0)
        {
          src(i) -= dst(i);
          size(i) += dst(i);
          dst(i) = 0;
        }
      }
      for(size_t i = 0; i < n; ++i)
      {
        size(i) = std::min(src(i) + size(i), image.size(i)) - src(i);
        size(i) = std::min(dst(i) + size(i), this->size(i)) - dst(i);
        if(size(i) <= 0) return;
      }

      // perform blit
      ProcessHelper<ivec<n>, ivec<n>>(size, pitch, image.pitch).template Process<n - 1>([&](int32_t dstOffset, int32_t srcOffset)
      {
        blend(pixels[dstOffset], image.pixels[srcOffset]);
      }, dot(dst, pitch), dot(src, image.pitch));
    }

    template <typename S>
    RawImage<T, n> DownSample(ivec<n> factor) const
    {
      ivec<n> newSize = this->size / factor;
      RawImage<T, n> newImage(newSize);

      typename VectorTraits<S>::Scalar factorVolume = 1;
      for(size_t i = 0; i < n; ++i)
        factorVolume *= factor(i);

      ProcessHelper<ivec<n>> sumHelper(factor, this->pitch);
      ProcessHelper<ivec<n>, ivec<n>>(newSize, newImage.pitch, this->pitch * factor).template Process<n - 1>([&](int32_t dstOffset, int32_t srcOffset)
      {
        S s = {};
        sumHelper.template Process<n - 1>([&](int32_t offset)
        {
          s += (S)this->pixels[offset];
        }, srcOffset);
        newImage.pixels[dstOffset] = (T)(s / factorVolume);
      }, int32_t{}, int32_t{});

      return std::move(newImage);
    }
  };

  template <typename T>
  using RawImageSlice2D = RawImageSlice<T, 2>;

  // typed uncompressed image of given dimensionality
  template <typename T, size_t n>
  class RawImage : public RawImageSlice<T, n>
  {
  public:
    RawImage(ivec<n> const& size = {})
    {
      this->size = size;
      int32_t p = 1;
      for(size_t i = 0; i < n; ++i)
      {
        this->pitch(i) = p;
        p *= this->size(i);
      }
      _pixels.assign(p, {});
      this->pixels = _pixels.data();
    }

    explicit RawImage(RawImageSlice<T, n> const& slice)
    : RawImage(slice.size)
    {
      this->Blit(slice, {}, {}, this->size);
    }

    RawImage(RawImage const&) = delete;
    RawImage(RawImage&& image) noexcept
    {
      *this = std::move(image);
    }

    RawImage& operator=(RawImage const&) = delete;
    RawImage& operator=(RawImage&& image) noexcept
    {
      swap(*this, image);
      return *this;
    }

    friend void swap(RawImage& a, RawImage& b) noexcept
    {
      swap(static_cast<RawImageSlice<T, n>&>(a), static_cast<RawImageSlice<T, n>&>(b));
      swap(a._pixels, b._pixels);
    }

  private:
    std::vector<T> _pixels;
  };

  template <typename T>
  using RawImage2D = RawImage<T, 2>;

  std::pair<std::vector<ivec2>, ivec2> Image2DShelfUnion(std::vector<ivec2> const& imageSizes, int32_t maxResultWidth, int32_t border = 0)
  {
    size_t imagesCount = imageSizes.size();

    // sort images by height
    std::vector<size_t> sortedImages(imagesCount);
    for(size_t i = 0; i < imagesCount; ++i)
      sortedImages[i] = i;
    std::sort(sortedImages.begin(), sortedImages.end(), [&](size_t a, size_t b)
    {
      return imageSizes[a].y() < imageSizes[b].y();
    });

    // place images
    int32_t resultWidth = border, resultHeight = border;
    std::vector<ivec2> positions(imagesCount);
    {
      int32_t currentX = border, currentRowHeight = 0;
      for(size_t i = 0; i < imagesCount; ++i)
      {
        auto const& imageSize = imageSizes[sortedImages[i]];
        int32_t width = imageSize.x();
        if(currentX + width + border > maxResultWidth)
        {
          // move to the next row
          resultHeight += currentRowHeight;
          currentRowHeight = 0;
          currentX = border;
        }

        positions[sortedImages[i]] = { currentX, resultHeight };

        currentX += width + border;
        resultWidth = std::max(resultWidth, currentX);
        currentRowHeight = std::max(currentRowHeight, imageSize.y() + border);
      }
      resultHeight += currentRowHeight;
    }

    return
    {
      std::move(positions),
      { resultWidth, resultHeight },
    };
  }

  // call templated function with every mip of uncompressed image buffer
  // the function is called with typed RawImageSlice, image index, and mip index
  template <typename F>
  void ProcessImageBufferMips(ImageBuffer const& imageBuffer, F const& f)
  {
    auto metrics = imageBuffer.format.GetMetrics();
    size_t imagesCount = imageBuffer.format.count ? imageBuffer.format.count : 1;

    auto process = [&]<size_t n>()
    {
      if(!imageBuffer.format.format.ApplyTyped([&]<typename T>()
      {
        for(size_t imageIndex = 0; imageIndex < imagesCount; ++imageIndex)
        {
          for(size_t mipIndex = 0; mipIndex < metrics.mips.size(); ++mipIndex)
          {
            auto const& mip = metrics.mips[mipIndex];

            RawImageSlice<T, n> imageSlice =
            {
              .pixels = (T*)((uint8_t*)imageBuffer.buffer.data + imageIndex * metrics.imageSize + mip.offset),
            };

            if constexpr(0 < n)
            {
              imageSlice.size.x() = mip.width;
              imageSlice.pitch.x() = 1;
            }
            if constexpr(1 < n)
            {
              imageSlice.size.y() = mip.height;
              imageSlice.pitch.y() = imageSlice.pitch.x() * imageSlice.size.x();
            }
            if constexpr(2 < n)
            {
              imageSlice.size.z() = mip.depth;
              imageSlice.pitch.z() = imageSlice.pitch.y() * imageSlice.size.y();
            }

            f(imageSlice, imageIndex, mipIndex);
          }
        }
      }))
        throw Exception("Unsupported format to process mips");
    };

    if(imageBuffer.format.height > 0)
    {
      if(imageBuffer.format.depth > 0)
      {
        process.template operator()<3>();
      }
      else
      {
        process.template operator()<2>();
      }
    }
    else
    {
      process.template operator()<1>();
    }
  }
}
