module;

export module coil.core.crypto;

// only one implementation at the moment
import :mbedtls;
export import coil.core.crypto.base;

export namespace Coil
{
  using namespace Coil::Crypto::MbedTLS;
}
