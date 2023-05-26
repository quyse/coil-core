#pragma once

#include "math.hpp"
#include <tuple>

namespace Coil
{
  // view of typed uncompressed image of given dimensionality
  template <typename T, size_t n>
  struct RawImageSlice
  {
    T* pixels = nullptr;
    ivec<n> pitch;
    ivec<n> size;

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
      std::swap(a.pitch, b.pitch);
      std::swap(a.size, b.size);
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
    void Blit(RawImageSlice const& image, ivec<n> dst, ivec<n> src, ivec<n> size)
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
        pixels[dstOffset] = image.pixels[srcOffset];
      }, dot(dst, pitch), dot(src, image.pitch));
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
    RawImage(RawImage&& image)
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

    template <typename S>
    RawImage DownSample(ivec<n> factor) const
    {
      ivec<n> newSize = this->size / factor;
      RawImage newImage(newSize);

      S factorVolume = 1;
      for(size_t i = 0; i < n; ++i)
        factorVolume *= factor(i);

      typename RawImage::template ProcessHelper<ivec<n>> sumHelper(factor, this->pitch);
      typename RawImage::template ProcessHelper<ivec<n>, ivec<n>>(newSize, newImage.pitch, this->pitch * factor).template Process<n - 1>([&](int32_t dstOffset, int32_t srcOffset)
      {
        S s = {};
        sumHelper.template Process<n - 1>([&](int32_t offset)
        {
          s += this->pixels[offset];
        }, srcOffset);
        newImage.pixels[dstOffset] = (T)(s / factorVolume);
      }, int32_t{}, int32_t{});

      return std::move(newImage);
    }

  private:
    std::vector<T> _pixels;
  };

  template <typename T>
  using RawImage2D = RawImage<T, 2>;

  std::pair<std::vector<ivec2>, ivec2> Image2DShelfUnion(std::vector<ivec2> const& imageSizes, int32_t maxResultWidth, int32_t border = 0);
}
