#pragma once

#include "crypto_base.hpp"

// only one implementation at the moment
#include "crypto_mbedtls.hpp"
namespace Coil
{
  using namespace Coil::Crypto::MbedTLS;
}
