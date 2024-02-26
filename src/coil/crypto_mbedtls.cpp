#include "crypto_base.hpp"
#include "crypto_mbedtls.hpp"

namespace Coil::Crypto::MbedTLS
{
  SHA256::SHA256()
  {
    mbedtls_sha256_init(&_context);
    mbedtls_sha256_starts(&_context, 0);
  }

  SHA256::~SHA256()
  {
    mbedtls_sha256_free(&_context);
  }

  void SHA256::Feed(Buffer const& buffer)
  {
    mbedtls_sha256_update(&_context, (uint8_t const*)buffer.data, buffer.size);
  }

  SHA256::Hash SHA256::Finish()
  {
    Hash hash;
    mbedtls_sha256_finish(&_context, hash.data());
    return hash;
  }
}
