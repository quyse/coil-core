#include "graphics_shaders.hpp"

namespace Coil
{
  ShaderDataTypeInfo const& ShaderDataTypeInfoOf(ShaderDataType dataType)
  {
    static ShaderDataTypeInfo const infos[] =
    {
#define S(bt) { .baseDataType = ShaderDataType::_##bt },

    S(float)
    S(float)
    S(float)
    S(float)
    S(float)
    S(float)
    S(uint)
    S(uint)
    S(uint)
    S(uint)
    S(int)
    S(int)
    S(int)
    S(int)
    S(bool)
    S(bool)
    S(bool)
    S(bool)

#undef S
    };

    return infos[(size_t)dataType];
  }
}
