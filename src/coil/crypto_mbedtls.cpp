#include "crypto_base.hpp"
#include "crypto_mbedtls.hpp"

namespace Coil::Crypto::MbedTLS
{
  SHA1::SHA1()
  {
    mbedtls_sha1_init(&_context);
    mbedtls_sha1_starts(&_context);
  }

  SHA1::~SHA1()
  {
    mbedtls_sha1_free(&_context);
  }

  void SHA1::Feed(Buffer const& buffer)
  {
    mbedtls_sha1_update(&_context, (uint8_t const*)buffer.data, buffer.size);
  }

  SHA1::Hash SHA1::Finish()
  {
    Hash hash;
    mbedtls_sha1_finish(&_context, hash.data());
    return hash;
  }


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
