#include "compress_zstd.hpp"

namespace
{
  size_t const compressOutBufferSize = ZSTD_CStreamOutSize();
  size_t const decompressOutBufferSize = ZSTD_DStreamOutSize();
}

namespace Coil
{
  ZstdCompressStream::ZstdCompressStream(OutputStream& outputStream)
  : _outputStream(outputStream), _stream(ZSTD_createCStream())
  {
  }

  ZstdCompressStream::~ZstdCompressStream()
  {
    ZSTD_freeCStream(_stream);
  }

  void ZstdCompressStream::Write(Buffer const& buffer)
  {
    _Write(buffer, ZSTD_e_continue);
  }

  void ZstdCompressStream::End()
  {
    _Write({}, ZSTD_e_end);
  }

  void ZstdCompressStream::_Write(Buffer const& buffer, ZSTD_EndDirective op)
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

  ZstdDecompressStream::ZstdDecompressStream(InputStream& inputStream)
  : _inputStream(inputStream), _stream(ZSTD_createDStream())
  {
  }

  ZstdDecompressStream::~ZstdDecompressStream()
  {
    ZSTD_freeDStream(_stream);
  }

  size_t ZstdDecompressStream::Read(Buffer const& buffer)
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

  ZstdDecompressStreamSource::ZstdDecompressStreamSource(InputStreamSource& source)
  : _source(source) {}

  InputStream& ZstdDecompressStreamSource::CreateStream(Book& book)
  {
    return book.Allocate<ZstdDecompressStream>(_source.CreateStream(book));
  }
}
