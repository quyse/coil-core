#include "tasks_streams.hpp"

namespace Coil
{
  Task<size_t> SuspendableInputStream::Read(Buffer const& buffer)
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

  Task<std::vector<uint8_t>> SuspendableInputStream::ReadAll()
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

  Task<void> SuspendableOutputStream::Write(Buffer const& buffer)
  {
    while(!TryWrite(buffer))
    {
      co_await WaitForWrite(buffer.size);
    }
  }

  SuspendablePipe::SuspendablePipe(size_t bufferSize, bool allowBufferExpansion)
  : _bufferSize(bufferSize), _allowBufferExpansion(allowBufferExpansion)
  {
  }

  std::optional<size_t> SuspendablePipe::TryRead(Buffer const& buffer)
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

  Task<void> SuspendablePipe::WaitForRead()
  {
    std::unique_lock lock{_mutex};
    while(!_ended && _buffer.GetDataSize() <= 0)
    {
      co_await _readerVar.Wait(lock);
    }
  }

  bool SuspendablePipe::TryWrite(Buffer const& buffer)
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

  Task<void> SuspendablePipe::WaitForWrite(size_t size)
  {
    std::unique_lock lock{_mutex};
    while(size <= _bufferSize && _buffer.GetDataSize() + size > _bufferSize)
    {
      co_await _writerVar.Wait(lock);
    }
  }
}
