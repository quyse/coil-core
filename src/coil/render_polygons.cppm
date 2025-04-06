module;

// alloca
#include "base.hpp"
#if defined(COIL_PLATFORM_WINDOWS)
#include <malloc.h>
#else
#include <alloca.h>
#endif

#include <span>

export module coil.core.render.polygons;

import coil.core.base;
import coil.core.graphics.shaders;
import coil.core.graphics;
import coil.core.math;
import coil.core.render;

export namespace Coil
{
  class PolygonRenderer
  {
  public:
    template <template <typename> typename T>
    struct Vertex
    {
      T<vec2_ua> position;
      T<vec3_ua> plane1;
      T<vec3_ua> plane2;
      T<vec3_ua> plane3;
      T<float> thickness;
      T<vec4_ua> fillColor;
      T<vec4_ua> borderColor;
    };

  private:
    static auto const& GetVertexLayout()
    {
      static auto const layout = GetGraphicsVertexStructLayout<Vertex>();
      return layout;
    }

  public:
    PolygonRenderer(GraphicsDevice& device)
    : device_{device} {}

    void Init(Book& book, GraphicsPool& pool)
    {
      pShader_ = &device_.CreateShader(book, [&]() -> GraphicsShaderRoots
      {
        using namespace Shaders;

        auto const& [vertex] = GetVertexLayout().slots;

        auto vInPosition = *vertex.position;
        auto vInPlane1 = *vertex.plane1;
        auto vInPlane2 = *vertex.plane2;
        auto vInPlane3 = *vertex.plane3;
        auto vInThickness = *vertex.thickness;
        auto vInFillColor = *vertex.fillColor;
        auto vInBorderColor = *vertex.borderColor;

        auto vAffinePosition = cvec(vInPosition, float_(1));

        auto iOutPosition = ShaderInterpolantBuiltinPosition();
        auto iDistances = ShaderInterpolant<vec4>(0);
        auto iFillColor = ShaderInterpolant<vec4>(1);
        auto iBorderColor = ShaderInterpolant<vec4>(2);

        auto vertexProgram = (
          iOutPosition.Write(cvec(
            vInPosition,
            float_(0),
            float_(1))
          ),
          iDistances.Write(cvec(
            dot(vAffinePosition, vInPlane1),
            dot(vAffinePosition, vInPlane2),
            dot(vAffinePosition, vInPlane3),
            vInThickness
          )),
          iFillColor.Write(vInFillColor),
          iBorderColor.Write(vInBorderColor)
        );

        auto fOutColor = ShaderFragment<vec4>(0);

        auto fDistances = *iDistances;
        auto fBorderCoef = clamp(
          (swizzle(fDistances, "w") - min(min(swizzle(fDistances, "x"), swizzle(fDistances, "y")), swizzle(fDistances, "z"))) * float_(1000.0f),
          float_(0), float_(1)
        );
        auto fColor = mix(*iFillColor, *iBorderColor, cvec(fBorderCoef, fBorderCoef, fBorderCoef, fBorderCoef));

        auto fragmentProgram = (
          fOutColor.Write(cvec(swizzle(fColor, "xyz"), float_(1)) * swizzle(fColor, "w"))
        );

        return
        {
          .vertex = vertexProgram,
          .fragment = fragmentProgram,
        };
      }());

      {
        GraphicsShader* shaders[] = { pShader_ };
        pPipelineLayout_ = &device_.CreatePipelineLayout(book, shaders);
      }
    }

    void InitPipeline(Book& book, ivec2 canvasSize, GraphicsPass& pass, GraphicsSubPassId subPassId)
    {
      canvasSize_ = canvasSize;
      canvasSizeCoef_ = vec2(canvasSize_) / 2.0f;
      invCanvasSizeCoef_ = 2.0f / vec2(canvasSize_);

      GraphicsPipelineConfig config =
      {
        .viewport = canvasSize,
        .depthTest = false,
        .depthWrite = false,
        .vertexLayout = GetVertexLayout().layout,
        .attachments =
        {
          {
            .blending = GraphicsPipelineConfig::Blending {},
          },
        },
      };
      pPipeline_ = &device_.CreatePipeline(book, config, *pPipelineLayout_, pass, subPassId, *pShader_);
    }

    using Cache = RenderCache<
      RenderPipelineKnob,
      RenderTriangleKnob<ShaderDataIdentityStruct<Vertex>>
    >;

    void RenderConvexPolygon(Cache& cache, std::span<vec2> const& positions, vec4 const& fillColor, vec4 const& borderColor, float thickness)
    {
      size_t n = positions.size();

      // calculate screen space positions
      vec2* p = (vec2*)alloca(n * sizeof(vec2));
      for(size_t i = 0; i < n; ++i)
      {
        p[i] = vec2(positions[i]) * invCanvasSizeCoef_ + vec2(-1, -1);
      }

      // calculate center of mass
      vec2 center;
      float area = 0;
      for(size_t i = n - 1, j = 0; j < n; i = j++)
      {
        float s = p[i].x() * p[j].y() - p[i].y() * p[j].x();
        center += (p[i] + p[j]) * s;
        area += s;
      }
      center /= area * 3;

      // calculate planes
      vec3* planes = (vec3*)alloca(n * sizeof(vec3));
      for(size_t i = n - 1, j = 0; j < n; i = j++)
      {
        vec2 edge = positions[j] - positions[i];
        vec2 normal = normalize(vec2(edge.y(), -edge.x()));
        planes[j] = vec3(
          normal.x() * canvasSizeCoef_.x(),
          normal.y() * canvasSizeCoef_.y(),
          dot(normal, canvasSizeCoef_ - positions[j]));
      }

      // prepare triangles
      ShaderDataIdentityStruct<Vertex> triangle[3] =
      {
        // first vertex is always center
        {
          .position = center,
        },
      };
      for(size_t t = 0; t < 3; ++t)
      {
        triangle[t].thickness = thickness;
        triangle[t].fillColor = fillColor;
        triangle[t].borderColor = borderColor;
      }

      for(size_t i = n - 2, j = n - 1, k = 0; k < n; i = j, j = k++)
      {
        triangle[1].position = p[i];
        triangle[2].position = p[j];
        for(size_t t = 0; t < 3; ++t)
        {
          triangle[t].plane1 = planes[i];
          triangle[t].plane2 = planes[j];
          triangle[t].plane3 = planes[k];
        }

        cache.Render(*pPipeline_, triangle);
      }
    }

  private:
    GraphicsDevice& device_;
    GraphicsShader* pShader_ = nullptr;
    GraphicsPipelineLayout* pPipelineLayout_ = nullptr;
    GraphicsPipeline* pPipeline_ = nullptr;
    ivec2 canvasSize_;
    vec2 canvasSizeCoef_;
    vec2 invCanvasSizeCoef_;
  };
}
