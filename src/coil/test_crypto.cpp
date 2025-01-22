#include "entrypoint.hpp"

import coil.core.base;
import coil.core.crypto;

using namespace Coil;

template <IsHashAlgorithm HashAlgorithm>
bool CheckHash(std::string_view str, typename HashAlgorithm::Hash const& hash)
{
  auto gotHash = CalculateHash<HashAlgorithm>(Buffer(str.data(), str.length()));
  return gotHash == hash;
}

int COIL_ENTRY_POINT(std::vector<std::string> args)
{
  // SHA256
  if(!CheckHash<SHA256>("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"_hex)) return 1;
  if(!CheckHash<SHA256>("The quick brown fox jumps over the lazy dog", "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"_hex)) return 1;
  {
    HashStream<SHA256> s;
    for(size_t i = 0; i < 1000000; ++i)
      s.Write(Buffer("a", 1));
    if(s.Finish() != "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"_hex) return 1;
  }

  return 0;
}
