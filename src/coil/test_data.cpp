#include "entrypoint.hpp"
#include "data.hpp"
#include <random>
#include <iostream>
using namespace Coil;

std::mt19937 rnd;

bool Test(size_t opCount, size_t maxLen)
{
  CircularMemoryBuffer buffer;

  size_t size = 0, writtenSize = 0, readSize = 0;
  std::vector<uint8_t> tmpbuf;
  for(size_t i = 0; i < opCount; ++i)
  {
    size_t len = rnd() % maxLen;
    tmpbuf.resize(len);
    if(rnd() % 2)
    {
      for(size_t j = 0; j < len; ++j)
        tmpbuf[j] = (uint8_t)((writtenSize + j) % 251);
      buffer.Write(Buffer(tmpbuf));
      size += len;
      writtenSize += len;
    }
    else
    {
      size_t read = buffer.Read(Buffer(tmpbuf));
      if(read < len)
      {
        if(readSize + read != writtenSize)
        {
          std::cerr << "read less than requested, but size does not add up\n";
          return false;
        }
      }
      else if(read != len) return false;

      for(size_t j = 0; j < read; ++j)
        if(tmpbuf[j] != (uint8_t)((readSize + j) % 251))
        {
          std::cerr << "wrong data read\n";
          return false;
        }

      readSize += read;
      size -= read;
    }

    if(size != buffer.GetDataSize())
    {
      std::cerr << "wrong data size " << i << "\n";
      return false;
    }
  }

  return true;
}

bool TestSeries(size_t count, size_t opCount, size_t maxLen)
{
  for(size_t i = 0; i < count; ++i)
    if(!Test(opCount, maxLen))
      return false;
  return true;
}

int COIL_ENTRY_POINT(std::vector<std::string> args)
{
  if(!TestSeries(100, 1000, 10)) return 1;
  if(!TestSeries(100, 1000, 100)) return 1;
  if(!TestSeries(100, 1000, 1000)) return 1;
  if(!TestSeries(100, 1000, 10000)) return 1;

  return 0;
}
