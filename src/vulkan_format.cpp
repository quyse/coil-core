#include "vulkan.hpp"

namespace Coil
{
  VkFormat VulkanSystem::GetVertexFormat(VertexFormat format)
  {
#define F(f) VertexFormat::Format::f
#define S(s) VertexFormat::Size::_##s
#define R(r) VK_FORMAT_##r
    switch(format.format)
    {
    case F(Float):
      switch(format.components)
      {
      case 1:
        switch(format.size)
        {
        case S(16bit):
          return R(R16_SFLOAT);
        case S(32bit):
          return R(R32_SFLOAT);
        case S(64bit):
          return R(R64_SFLOAT);
        default: break;
        }
        break;
      case 2:
        switch(format.size)
        {
        case S(32bit):
          return R(R16G16_SFLOAT);
        case S(64bit):
          return R(R32G32_SFLOAT);
        case S(128bit):
          return R(R64G64_SFLOAT);
        default: break;
        }
        break;
      case 3:
        switch(format.size)
        {
        case S(48bit):
          return R(R16G16B16_SFLOAT);
        case S(96bit):
          return R(R32G32B32_SFLOAT);
        default: break;
        }
        break;
      case 4:
        switch(format.size)
        {
        case S(64bit):
          return R(R16G16B16A16_SFLOAT);
        case S(128bit):
          return R(R32G32B32A32_SFLOAT);
        default: break;
        }
        break;
      default: break;
      }
      break;
    case F(SInt):
      switch(format.components)
      {
      case 1:
        switch(format.size)
        {
        case S(8bit):
          return R(R8_SINT);
        case S(16bit):
          return R(R16_SINT);
        case S(32bit):
          return R(R32_SINT);
        case S(64bit):
          return R(R64_SINT);
        default: break;
        }
        break;
      case 2:
        switch(format.size)
        {
        case S(16bit):
          return R(R8G8_SINT);
        case S(32bit):
          return R(R16G16_SINT);
        case S(64bit):
          return R(R32G32_SINT);
        case S(128bit):
          return R(R64G64_SINT);
        default: break;
        }
        break;
      case 3:
        switch(format.size)
        {
        case S(24bit):
          return R(R8G8B8_SINT);
        case S(48bit):
          return R(R16G16B16_SINT);
        case S(96bit):
          return R(R32G32B32_SINT);
        default: break;
        }
        break;
      case 4:
        switch(format.size)
        {
        case S(32bit):
          return R(R8G8B8A8_SINT);
        case S(64bit):
          return R(R16G16B16A16_SINT);
        case S(128bit):
          return R(R32G32B32A32_SINT);
        default: break;
        }
        break;
      default: break;
      }
      break;
    case F(UInt):
      switch(format.components)
      {
      case 1:
        switch(format.size)
        {
        case S(8bit):
          return R(R8_UINT);
        case S(16bit):
          return R(R16_UINT);
        case S(32bit):
          return R(R32_UINT);
        case S(64bit):
          return R(R64_UINT);
        default: break;
        }
        break;
      case 2:
        switch(format.size)
        {
        case S(16bit):
          return R(R8G8_UINT);
        case S(32bit):
          return R(R16G16_UINT);
        case S(64bit):
          return R(R32G32_UINT);
        case S(128bit):
          return R(R64G64_UINT);
        default: break;
        }
        break;
      case 3:
        switch(format.size)
        {
        case S(24bit):
          return R(R8G8B8_UINT);
        case S(48bit):
          return R(R16G16B16_UINT);
        case S(96bit):
          return R(R32G32B32_UINT);
        default: break;
        }
        break;
      case 4:
        switch(format.size)
        {
        case S(32bit):
          return R(R8G8B8A8_UINT);
        case S(64bit):
          return R(R16G16B16A16_UINT);
        case S(128bit):
          return R(R32G32B32A32_UINT);
        default: break;
        }
        break;
      default: break;
      }
      break;
    case F(SNorm):
      switch(format.components)
      {
      case 1:
        switch(format.size)
        {
        case S(8bit):
          return R(R8_SNORM);
        case S(16bit):
          return R(R16_SNORM);
        default: break;
        }
        break;
      case 2:
        switch(format.size)
        {
        case S(16bit):
          return R(R8G8_SNORM);
        case S(32bit):
          return R(R16G16_SNORM);
        default: break;
        }
        break;
      case 3:
        switch(format.size)
        {
        case S(24bit):
          return R(R8G8B8_SNORM);
        case S(48bit):
          return R(R16G16B16_SNORM);
        default: break;
        }
        break;
      case 4:
        switch(format.size)
        {
        case S(32bit):
          return R(R8G8B8A8_SNORM);
        case S(64bit):
          return R(R16G16B16A16_SNORM);
        default: break;
        }
        break;
      default: break;
      }
      break;
    case F(UNorm):
      switch(format.components)
      {
      case 1:
        switch(format.size)
        {
        case S(8bit):
          return R(R8_UNORM);
        case S(16bit):
          return R(R16_UNORM);
        default: break;
        }
        break;
      case 2:
        switch(format.size)
        {
        case S(16bit):
          return R(R8G8_UNORM);
        case S(32bit):
          return R(R16G16_UNORM);
        default: break;
        }
        break;
      case 3:
        switch(format.size)
        {
        case S(24bit):
          return R(R8G8B8_UNORM);
        case S(48bit):
          return R(R16G16B16_UNORM);
        default: break;
        }
        break;
      case 4:
        switch(format.size)
        {
        case S(32bit):
          return R(R8G8B8A8_UNORM);
        case S(64bit):
          return R(R16G16B16A16_UNORM);
        default: break;
        }
        break;
      default: break;
      }
      break;
    default: break;
    }
    throw Exception("unsupported Vulkan vertex format");
#undef F
#undef S
#undef R
  }

  VkFormat VulkanSystem::GetPixelFormat(PixelFormat format)
  {
#define T(t) PixelFormat::Type::t
#define C(c) PixelFormat::Components::c
#define F(f) PixelFormat::Format::f
#define S(s) PixelFormat::Size::_##s
#define X(x) PixelFormat::Compression::x
#define R(r) VK_FORMAT_##r
    switch(format.type)
    {
    case T(Unknown):
      return R(UNDEFINED);
    case T(Uncompressed):
      switch(format.components)
      {
      case C(R):
        switch(format.format)
        {
        case F(Untyped):
          break;
        case F(Uint):
          switch(format.size)
          {
          case S(8bit): return R(R8_UNORM);
          case S(16bit): return R(R16_UNORM);
          default: break;
          }
          break;
        case F(Float):
          switch(format.size)
          {
          case S(16bit): return R(R16_SFLOAT);
          case S(32bit): return R(R32_SFLOAT);
          default: break;
          }
          break;
        }
        break;
      case C(RG):
        switch(format.format)
        {
        case F(Untyped):
          break;
        case F(Uint):
          switch(format.size)
          {
          case S(16bit): return R(R8G8_UNORM);
          case S(32bit): return R(R16G16_UNORM);
          default: break;
          }
          break;
        case F(Float):
          switch(format.size)
          {
          case S(32bit): return R(R16G16_SFLOAT);
          case S(64bit): return R(R32G32_SFLOAT);
          default: break;
          }
          break;
        }
        break;
      case C(RGB):
        switch(format.format)
        {
        case F(Untyped):
          break;
        case F(Uint):
          break;
        case F(Float):
          switch(format.size)
          {
          case S(32bit): return R(B10G11R11_UFLOAT_PACK32);
          case S(96bit): return R(R32G32B32_SFLOAT);
          default: break;
          }
          break;
        }
        break;
      case C(RGBA):
        switch(format.format)
        {
        case F(Untyped):
          break;
        case F(Uint):
          switch(format.size)
          {
          case S(32bit): return format.srgb ? R(R8G8B8A8_SRGB) : R(R8G8B8A8_UNORM);
          case S(64bit): return R(R16G16B16A16_UNORM);
          default: break;
          }
          break;
        case F(Float):
          switch(format.size)
          {
          case S(64bit): return R(R16G16B16A16_SFLOAT);
          case S(128bit): return R(R32G32B32A32_SFLOAT);
          default: break;
          }
          break;
        }
        break;
      }
      break;
    case T(Compressed):
      switch(format.compression)
      {
      case X(Bc1): return format.srgb ? R(BC1_RGB_SRGB_BLOCK) : R(BC1_RGB_UNORM_BLOCK);
      case X(Bc1Alpha): return format.srgb ? R(BC1_RGBA_SRGB_BLOCK) : R(BC1_RGBA_UNORM_BLOCK);
      case X(Bc2): return format.srgb ? R(BC2_SRGB_BLOCK) : R(BC2_UNORM_BLOCK);
      case X(Bc3): return format.srgb ? R(BC3_SRGB_BLOCK) : R(BC3_UNORM_BLOCK);
      case X(Bc4): return R(BC4_UNORM_BLOCK);
      case X(Bc4Signed): return R(BC4_SNORM_BLOCK);
      case X(Bc5): return R(BC5_UNORM_BLOCK);
      case X(Bc5Signed): return R(BC5_SNORM_BLOCK);
      default: break;
      }
      break;
    }
    throw Exception("unsupported Vulkan pixel format");
#undef T
#undef C
#undef F
#undef S
#undef X
#undef R
  }
}
