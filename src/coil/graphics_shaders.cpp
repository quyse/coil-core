#include "graphics_shaders.hpp"

namespace Coil
{
  ShaderDataScalarType const& ShaderDataTypeGetScalarType(ShaderDataType const& dataType)
  {
    switch(dataType.GetKind())
    {
    case ShaderDataKind::Scalar:
      return static_cast<ShaderDataScalarType const&>(dataType);
    case ShaderDataKind::Vector:
      return static_cast<ShaderDataVectorType const&>(dataType).baseType;
    case ShaderDataKind::Matrix:
      return static_cast<ShaderDataMatrixType const&>(dataType).baseType;
    default:
      throw Exception("unsupported SPIR-V shader data kind for getting scalar data type");
    }
  }

  bool operator<(ShaderDataType const& a, ShaderDataType const& b)
  {
    auto ak = a.GetKind();
    auto bk = b.GetKind();
    if(ak < bk) return true;
    if(bk < ak) return false;
    switch(ak)
    {
    case ShaderDataKind::Scalar:
      {
        auto const& as = static_cast<ShaderDataScalarType const&>(a);
        auto const& bs = static_cast<ShaderDataScalarType const&>(b);
        return as.type < bs.type;
      }
    case ShaderDataKind::Vector:
      {
        auto const& av = static_cast<ShaderDataVectorType const&>(a);
        auto const& bv = static_cast<ShaderDataVectorType const&>(b);
        if(av.baseType < bv.baseType) return true;
        if(bv.baseType < av.baseType) return false;
        return av.n < bv.n;
      }
    case ShaderDataKind::Matrix:
      {
        auto const& am = static_cast<ShaderDataMatrixType const&>(a);
        auto const& bm = static_cast<ShaderDataMatrixType const&>(b);
        if(am.baseType < bm.baseType) return true;
        if(bm.baseType < am.baseType) return false;
        if(am.n < bm.n) return true;
        if(bm.n < am.n) return false;
        return am.m < bm.m;
      }
    case ShaderDataKind::Array:
      {
        auto const& aa = static_cast<ShaderDataArrayType const&>(a);
        auto const& ba = static_cast<ShaderDataArrayType const&>(b);
        if(aa.baseType < ba.baseType) return true;
        if(ba.baseType < aa.baseType) return false;
        return aa.n < ba.n;
      }
    case ShaderDataKind::Struct:
      {
        auto const& as = static_cast<ShaderDataStructType const&>(a);
        auto const& bs = static_cast<ShaderDataStructType const&>(b);
        return as.members < bs.members;
      }
    }
  }
}
