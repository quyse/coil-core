#pragma once

#include "base.hpp"
#include <zstd.h>

namespace Coil
{
  class ZstdCompressStream : public OutputStream
  {
  public:
    ZstdCompressStream(OutputStream& outputStream);
    ~ZstdCompressStream();

    void Write(Buffer const& buffer) override;
    void End() override;

  private:
    void _Write(Buffer const& buffer, ZSTD_EndDirective op);

    OutputStream& _outputStream;
    ZSTD_CStream* _stream = nullptr;
  };

  class ZstdDecompressStream : public InputStream
  {
  public:
    ZstdDecompressStream(InputStream& inputStream);
    ~ZstdDecompressStream();

    size_t Read(Buffer const& buffer) override;

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
}
