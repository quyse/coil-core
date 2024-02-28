#include "fs.hpp"
#include "compress_zstd.hpp"
#include "entrypoint.hpp"
#include <cstring>
#include <iostream>

using namespace Coil;

int COIL_ENTRY_POINT(std::vector<std::string> args)
{
  Book book;

  std::cout << "file: " << args[0] << "\n";
  auto& inputStream = FileInputStream::Open(book, std::move(args[0]));
  MemoryStream sourceStream;
  sourceStream.WriteAllFrom(inputStream);
  std::cout << "source size: " << sourceStream.ToBuffer().size << "\n";
  MemoryStream compressedStream;
  {
    BufferInputStream s1(sourceStream.ToBuffer());
    ZstdCompressStream s2(compressedStream);
    s2.WriteAllFrom(s1);
    s2.End();
  }
  std::cout << "compressed size: " << compressedStream.ToBuffer().size << "\n";
  MemoryStream decompressedStream;
  {
    BufferInputStream s1(compressedStream.ToBuffer());
    ZstdDecompressStream s2(s1);
    decompressedStream.WriteAllFrom(s2);
  }
  std::cout << "decompressed size: " << decompressedStream.ToBuffer().size << "\n";

  if(sourceStream.ToBuffer().size != decompressedStream.ToBuffer().size) return 1;

  {
    int c = memcmp(sourceStream.ToBuffer().data, decompressedStream.ToBuffer().data, sourceStream.ToBuffer().size);
    std::cout << "comparing: " << (c == 0 ? "OK" : "FAIL") << "\n";
    return c == 0 ? 0 : 1;
  }
}
