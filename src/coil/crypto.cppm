module;

export module coil.core.crypto;

// only one implementation at the moment
import :mbedtls;
export import coil.core.crypto.base;

export namespace Coil
{
  using SHA1 = Crypto::MbedTLS::SHA1;
  using SHA256 = Crypto::MbedTLS::SHA256;
}
