#include "graphics_shaders.hpp"

namespace Coil
{
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
        return as.t < bs.t;
      }
    case ShaderDataKind::Vector:
      {
        auto const& av = static_cast<ShaderDataVectorType const&>(a);
        auto const& bv = static_cast<ShaderDataVectorType const&>(b);
        if(av.t < bv.t) return true;
        if(bv.t < av.t) return false;
        return av.n < bv.n;
      }
    case ShaderDataKind::Matrix:
      {
        auto const& am = static_cast<ShaderDataMatrixType const&>(a);
        auto const& bm = static_cast<ShaderDataMatrixType const&>(b);
        if(am.t < bm.t) return true;
        if(bm.t < am.t) return false;
        if(am.n < bm.n) return true;
        if(bm.n < am.n) return false;
        return am.m < bm.m;
      }
    case ShaderDataKind::Array:
      {
        auto const& aa = static_cast<ShaderDataArrayType const&>(a);
        auto const& ba = static_cast<ShaderDataArrayType const&>(b);
        if(aa.t < ba.t) return true;
        if(ba.t < aa.t) return false;
        return aa.n < ba.n;
      }
    }
  }
}
