module;

#include <coroutine>
#include <mutex>
#include <optional>

export module coil.core.tasks.streams;

import coil.core.base;
import coil.core.data;
import coil.core.tasks;
import coil.core.tasks.sync;

export namespace Coil
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
    Task<size_t> Read(Buffer const& buffer)
    {
      for(;;)
      {
        std::optional<size_t> maybeRead = TryRead(buffer);
        if(!maybeRead.has_value())
        {
          co_return 0;
        }
        if(maybeRead.value())
        {
          co_return maybeRead.value();
        }

        co_await WaitForRead();
      }
    }

    // read all up to the end
    Task<std::vector<uint8_t>> ReadAll()
    {
      std::vector<uint8_t> buffer;
      for(;;)
      {
        // extend buffer a bit
        size_t const chunkSize = 4096;
        size_t size = buffer.size();
        buffer.resize(size + chunkSize);

        std::optional<size_t> maybeRead = TryRead(Buffer(buffer.data() + size, chunkSize));
        // if stream ended, return
        if(!maybeRead.has_value())
        {
          buffer.resize(size);
          co_return std::move(buffer);
        }
        // if some data is read
        if(maybeRead.value())
        {
          // resize buffer back to correct size
          buffer.resize(size + maybeRead.value());
        }
        // otherwise wait
        else
        {
          buffer.resize(size);
          co_await WaitForRead();
        }
      }
    }
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
    Task<void> Write(Buffer const& buffer)
    {
      while(!TryWrite(buffer))
      {
        co_await WaitForWrite(buffer.size);
      }
    }
  };

  // buffered pipe which can be written to one end and read from the other
  class SuspendablePipe final : public SuspendableInputStream, public SuspendableOutputStream
  {
  public:
    // allow buffer expansion means allow writes bigger than buffer size
    // normally if there's not enough space for write pipe suspends
    // but if the write is bigger than the buffer, assuming the writer cannot
    // do lesser write, there's no point in suspending
    // if expansion is allowed, big writes succeed immediately, and buffer is expanded
    // if expansion is not allowed, an exception is thrown
    SuspendablePipe(size_t bufferSize, bool allowBufferExpansion)
    : _bufferSize(bufferSize), _allowBufferExpansion(allowBufferExpansion)
    {
    }

    std::optional<size_t> TryRead(Buffer const& buffer) override
    {
      std::unique_lock lock{_mutex};

      size_t read = _buffer.Read(buffer);
      if(read)
      {
        lock.unlock();
        // notify writer
        _writerVar.NotifyOne();
      }
      else
      {
        // indicate end if needed
        if(_ended) return {};
      }

      return read;
    }

    Task<void> WaitForRead() override
    {
      std::unique_lock lock{_mutex};
      while(!_ended && _buffer.GetDataSize() <= 0)
      {
        co_await _readerVar.Wait(lock);
      }
    }

    bool TryWrite(Buffer const& buffer) override
    {
      std::unique_lock lock{_mutex};

      // if it's a write
      if(buffer.size)
      {
        // error if input buffer if bigger than requested buffer size, and expansion is not allowed
        if(buffer.size > _bufferSize && !_allowBufferExpansion)
          throw Exception("write to suspendable pipe is too big");

        // if there's no place for the data
        if(buffer.size <= _bufferSize && _buffer.GetDataSize() + buffer.size > _bufferSize)
          return false;

        // otherwise there's space, write data
        _buffer.Write(buffer);
      }
      // otherwise it's indication of end
      else
      {
        // record end
        _ended = true;
      }

      lock.unlock();
      // notify reader
      _readerVar.NotifyOne();

      return true;
    }

    Task<void> WaitForWrite(size_t size) override
    {
      std::unique_lock lock{_mutex};
      while(size <= _bufferSize && _buffer.GetDataSize() + size > _bufferSize)
      {
        co_await _writerVar.Wait(lock);
      }
    }

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
