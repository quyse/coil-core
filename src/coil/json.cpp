#include "json.hpp"

namespace Coil
{
  json ParseJsonBuffer(Buffer const& buffer)
  {
    try
    {
      return json::parse((uint8_t const*)buffer.data, (uint8_t const*)buffer.data + buffer.size);
    }
    catch(json::exception const&)
    {
      throw Exception("invalid JSON");
    }
  }
}
