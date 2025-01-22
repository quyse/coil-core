module;

// crypto implementation based on MbedTLS

#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <array>

export module coil.core.crypto:mbedtls;

import coil.core.base;
import coil.core.crypto.base;

export namespace Coil::Crypto::MbedTLS
{
  class SHA1
  {
  public:
    using Hash = std::array<uint8_t, 20>;

    SHA1()
    {
      mbedtls_sha1_init(&_context);
      mbedtls_sha1_starts(&_context);
    }
    ~SHA1()
    {
      mbedtls_sha1_free(&_context);
    }

    void Feed(Buffer const& buffer)
    {
      mbedtls_sha1_update(&_context, (uint8_t const*)buffer.data, buffer.size);
    }

    Hash Finish()
    {
      Hash hash;
      mbedtls_sha1_finish(&_context, hash.data());
      return hash;
    }

  private:
    mbedtls_sha1_context _context;
  };

  class SHA256
  {
  public:
    using Hash = std::array<uint8_t, 32>;

    SHA256()
    {
      mbedtls_sha256_init(&_context);
      mbedtls_sha256_starts(&_context, 0);
    }
    ~SHA256()
    {
      mbedtls_sha256_free(&_context);
    }

    void Feed(Buffer const& buffer)
    {
      mbedtls_sha256_update(&_context, (uint8_t const*)buffer.data, buffer.size);
    }

    Hash Finish()
    {
      Hash hash;
      mbedtls_sha256_finish(&_context, hash.data());
      return hash;
    }

  private:
    mbedtls_sha256_context _context;
  };

  static_assert(IsHashAlgorithm<SHA256>);
}
