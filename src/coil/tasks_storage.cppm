module;

#include <cstddef>
#include <cstdint>
#include <coroutine>

export module coil.core.tasks.storage;

import coil.core.base;
import coil.core.tasks;

export namespace Coil
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
    virtual Task<void> AsyncEnd()
    {
      co_return;
    }
  };
}
