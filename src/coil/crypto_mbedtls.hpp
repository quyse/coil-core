#pragma once

// crypto implementation based on MbedTLS

#include <mbedtls/sha256.h>

namespace Coil::Crypto::MbedTLS
{
  class SHA256
  {
  public:
    using Hash = std::array<uint8_t, 32>;

    SHA256();
    ~SHA256();

    void Feed(Buffer const& buffer);
    Hash Finish();

  private:
    mbedtls_sha256_context _context;
  };

  static_assert(IsHashAlgorithm<SHA256>);
}
