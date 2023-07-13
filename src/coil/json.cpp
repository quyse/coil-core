#include "json.hpp"

namespace Coil
{
  Json JsonFromBuffer(Buffer const& buffer)
  {
    try
    {
      return Json::parse((uint8_t const*)buffer.data, (uint8_t const*)buffer.data + buffer.size);
    }
    catch(Json::exception const&)
    {
      throw Exception("JSON from buffer failed");
    }
  }

  std::string JsonToString(Json const& j)
  {
    try
    {
      return j.dump();
    }
    catch(Json::exception const&)
    {
      throw Exception("JSON to string failed");
    }
  }
}
