#pragma once

#include "tasks.hpp"

namespace Coil
{
  class AsyncReadableStorage
  {
  public:
    virtual Task<size_t> AsyncRead(uint64_t offset, Buffer const& buffer) const = 0;
  };

  class AsyncWritableStorage
  {
  public:
    virtual Task<void> AsyncWrite(uint64_t offset, Buffer const& buffer) = 0;
    virtual Task<void> AsyncEnd();
  };
}
