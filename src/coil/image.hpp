#pragma once

#include "math.hpp"

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

    friend void swap(RawImageSlice& a, RawImageSlice& b)
    {
      std::swap(a.pixels, b.pixels);
      std::swap(a.pitch, b.pitch);
      std::swap(a.size, b.size);
    }

  private:
    class BlitHelper
    {
    public:
      BlitHelper(RawImageSlice& dstImage, RawImageSlice const& srcImage, ivec<n> const& size)
      : _dstImage(dstImage), _srcImage(srcImage), _size(size) {}

      template <size_t i>
      void Blit(int32_t dstOffset, int32_t srcOffset)
      {
        for(int32_t c = 0; c < _size(i); ++c)
        {
          if constexpr(i > 0)
          {
            Blit<i - 1>(dstOffset, srcOffset);
          }
          else
          {
            _dstImage.pixels[dstOffset] = _srcImage.pixels[srcOffset];
          }
          dstOffset += _dstImage.pitch(i);
          srcOffset += _srcImage.pitch(i);
        }
      }

    private:
      RawImageSlice& _dstImage;
      RawImageSlice const& _srcImage;
      ivec<n> const _size;
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
      BlitHelper(*this, image, size).template Blit<n - 1>(dot(dst, pitch), dot(src, image.pitch));
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

    RawImage(RawImageSlice<T, n> const& slice)
    : RawImage(slice.size)
    {
      this->Blit(slice, {}, {}, this->size);
    }

    RawImage(RawImage&& image)
    {
      swap(*this, image);
    }

    RawImage& operator=(RawImage&& image)
    {
      swap(*this, image);
      return *this;
    }

    friend void swap(RawImage& a, RawImage& b)
    {
      std::swap(a._pixels, b._pixels);
      std::swap(static_cast<RawImageSlice<T, n>&>(a), static_cast<RawImageSlice<T, n>&>(b));
    }

  private:
    std::vector<T> _pixels;
  };

  template <typename T>
  using RawImage2D = RawImage<T, 2>;

  std::pair<std::vector<ivec2>, ivec2> Image2DShelfUnion(std::vector<ivec2> const& imageSizes, int32_t maxResultWidth, int32_t border = 0);
}
