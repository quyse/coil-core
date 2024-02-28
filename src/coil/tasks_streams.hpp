#pragma once

#include "tasks_sync.hpp"
#include "data.hpp"

namespace Coil
{
  class SuspendableInputStream
  {
  public:
    // read some data
    // returns empty optional on stream end
    // returns zero if there's no data yet, but it's not end yet
    virtual std::optional<size_t> TryRead(Buffer const& buffer) = 0;

    // wait until some data appears
    virtual Task<void> WaitForRead() = 0;

    // read some non-zero amount of data, suspend if needed
    // returns zero on stream end
    Task<size_t> Read(Buffer const& buffer);
  };

  class SuspendableOutputStream
  {
  public:
    // write some data
    // writes whole buffer or nothing
    // empty buffer indicates end
    virtual bool TryWrite(Buffer const& buffer) = 0;

    // wait until it's possible to write specific amount of data
    virtual Task<void> WaitForWrite(size_t size) = 0;

    // write some data, suspend if needed
    Task<void> Write(Buffer const& buffer);
  };

  // buffered pipe which can be written to one end and read from the other
  class SuspendablePipe final : public SuspendableInputStream, public SuspendableOutputStream
  {
  public:
    // allow buffer expansion means allow writes bigger than buffer size
    // normally if there's not enough space for write pipe suspends
    // but if the write is bigger than the buffer, there's no point in suspending
    // if expansion is allowed, big writes succeed immediately, and buffer is expanded
    // if expansion is not allowed, an exception is thrown
    SuspendablePipe(size_t bufferSize, bool allowBufferExpansion);

    std::optional<size_t> TryRead(Buffer const& buffer) override;
    Task<void> WaitForRead() override;
    bool TryWrite(Buffer const& buffer) override;
    Task<void> WaitForWrite(size_t size) override;

  private:
    size_t const _bufferSize;
    bool const _allowBufferExpansion;
    std::mutex _mutex;
    CircularMemoryBuffer _buffer;
    ConditionVariable _readerVar;
    ConditionVariable _writerVar;
    bool _ended = false;
  };
}
