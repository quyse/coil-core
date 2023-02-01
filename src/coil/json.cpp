#include "json.hpp"

namespace Coil
{
  json JsonFromBuffer(Buffer const& buffer)
  {
    try
    {
      return json::parse((uint8_t const*)buffer.data, (uint8_t const*)buffer.data + buffer.size);
    }
    catch(json::exception const&)
    {
      throw Exception("JSON from buffer failed");
    }
  }

  std::string JsonToString(json const& j)
  {
    try
    {
      return j.dump();
    }
    catch(json::exception const&)
    {
      throw Exception("JSON to string failed");
    }
  }
}
