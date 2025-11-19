module;

#include "base.hpp"
#include <zstd.h>
#if !defined(COIL_PLATFORM_WINDOWS)
#include <alloca.h>
#endif
#include <malloc.h>
#include <concepts>
#include <cstdint>
#include <string_view>

export module coil.core.compress.zstd;

import coil.core.base;

namespace
{
  size_t const compressOutBufferSize = ZSTD_CStreamOutSize();
  size_t const decompressOutBufferSize = ZSTD_DStreamOutSize();
}

export namespace Coil
{
  class ZstdCompressStream final : public OutputStream
  {
  public:
    ZstdCompressStream(OutputStream& outputStream)
    : _outputStream(outputStream), _stream(ZSTD_createCStream())
    {
    }
    ~ZstdCompressStream()
    {
      ZSTD_freeCStream(_stream);
    }

    void Write(Buffer const& buffer) override
    {
      _Write(buffer, ZSTD_e_continue);
    }

    void End() override
    {
      _Write({}, ZSTD_e_end);
    }

  private:
    void _Write(Buffer const& buffer, ZSTD_EndDirective op)
    {
      ZSTD_inBuffer inBuffer =
      {
        .src = buffer.data,
        .size = buffer.size,
        .pos = 0,
      };
      uint8_t* outBufferData = (uint8_t*)alloca(compressOutBufferSize);
      bool moreOutput;
      do
      {
        ZSTD_outBuffer outBuffer =
        {
          .dst = outBufferData,
          .size = compressOutBufferSize,
          .pos = 0,
        };
        size_t result = ZSTD_compressStream2(_stream, &outBuffer, &inBuffer, op);
        if(ZSTD_isError(result))
        {
          throw Exception("Zstd compression failed");
        }
        moreOutput = result > 0;
        if(outBuffer.pos)
        {
          _outputStream.Write(Buffer(outBuffer.dst, outBuffer.pos));
        }
      }
      while(moreOutput || inBuffer.pos < inBuffer.size);
    }

    OutputStream& _outputStream;
    ZSTD_CStream* _stream = nullptr;
  };

  class ZstdDecompressStream final : public InputStream
  {
  public:
    ZstdDecompressStream(InputStream& inputStream)
    : _inputStream(inputStream), _stream(ZSTD_createDStream())
    {
    }
    ~ZstdDecompressStream()
    {
      ZSTD_freeDStream(_stream);
    }

    size_t Read(Buffer const& buffer) override
    {
      ZSTD_outBuffer outBuffer =
      {
        .dst = buffer.data,
        .size = buffer.size,
        .pos = 0,
      };
      while(outBuffer.pos < outBuffer.size)
      {
        if(_inBuffer.pos >= _inBuffer.size)
        {
          _inBuffer.size = _inputStream.Read(Buffer(_inBuffer.src, sizeof(_inBufferData)));
          _inBuffer.pos = 0;
        }
        size_t lastOutPos = outBuffer.pos;
        size_t result = ZSTD_decompressStream(_stream, &outBuffer, &_inBuffer);
        if(ZSTD_isError(result))
        {
          throw Exception("Zstd decompression failed");
        }
        if(outBuffer.pos == lastOutPos && _inBuffer.size == 0)
        {
          break;
        }
      }
      return outBuffer.pos;
    }

  private:
    InputStream& _inputStream;
    ZSTD_DStream* _stream = nullptr;
    uint8_t _inBufferData[0x100];
    ZSTD_inBuffer _inBuffer =
    {
      .src = _inBufferData,
      .size = 0,
      .pos = 0,
    };
  };

  class ZstdDecompressStreamSource final : public InputStreamSource
  {
  public:
    ZstdDecompressStreamSource(InputStreamSource& source)
    : _source(source) {}

    InputStream& CreateStream(Book& book) override
    {
      return book.Allocate<ZstdDecompressStream>(_source.CreateStream(book));
    }

  private:
    InputStreamSource& _source;
  };

  class ZstdAssetLoader
  {
  public:
    template <typename Asset, typename AssetContext>
    requires std::convertible_to<ZstdDecompressStreamSource*, Asset>
    Asset LoadAsset(Book& book, AssetContext& assetContext) const
    {
      return &book.Allocate<ZstdDecompressStreamSource>(
        *assetContext.template LoadAssetParam<InputStreamSource*>(book, "source")
      );
    }

    static constexpr std::string_view assetLoaderName = "zstd";
  };
  static_assert(IsAssetLoader<ZstdAssetLoader>);
}
