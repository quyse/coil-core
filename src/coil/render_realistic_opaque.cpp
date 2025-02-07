module;

#include <numbers>
#include <tuple>

module coil.core.render.realistic;

namespace Coil
{
  void RealisticRenderer::InitOpaqueShaders(Book& book)
  {
    auto const& uniformScene = GetShaderDataStructBuffer<SceneUniform, 0, 0, ShaderBufferType::Uniform>();
    auto const& uniformMaterial = GetShaderDataStructBuffer<OpaqueMaterialUniform, 1, 0, ShaderBufferType::Uniform>();

    pOpaqueShader_ = &graphicsDevice_.CreateShader(book, [&]() -> GraphicsShaderRoots
    {
      auto const& [vertex, instance] = GetOpaqueVertexLayout().slots;

      using namespace Shaders;

      // vertex inputs
      auto vInPosition = *vertex.position;
      auto vInNormal = *vertex.normal;
      auto vInRotation = *instance.rotation;
      auto vInOffset = *instance.offset;

      // apply object transform
      auto vPosition = vInRotation * vInPosition + vInOffset;
      auto vNormal = vInRotation * vInNormal;

      // interpolants
      auto iOutPosition = ShaderInterpolantBuiltinPosition();
      auto iPosition = ShaderInterpolant<vec3>(0);
      auto iNormal = ShaderInterpolant<vec3>(1);

      // vertex program
      auto vertexProgram =
      (
        iOutPosition.Write(*uniformScene.viewProj * cvec(vPosition, float_(1))),
        iPosition.Write(vPosition),
        iNormal.Write(vNormal)
      );

      // fragment inputs
      auto fInPosition = *iPosition;
      auto fInNormal = normalize(*iNormal);

      // directions to eye, light, and half vector
      auto fToEye = normalize(*uniformScene.eyePosition - fInPosition);
      auto fToLight = normalize(*uniformScene.lightPosition - fInPosition);
      auto fHalf = normalize(fToEye + fToLight);

      // uniform light inputs
      auto uLightColor = *uniformScene.lightColor;
      auto uAmbientLightColor = *uniformScene.ambientLightColor;

      // uniform material inputs
      auto uColor = *uniformMaterial.color;
      auto uRoughness = *uniformMaterial.roughness;
      auto uMetalness = *uniformMaterial.metalness;

      // specular color (characteristic specular reflectance), constant for non-metals
      auto fSpecularColor = mix(vec3_(0.04f, 0.04f, 0.04f), uColor, cvec(uMetalness, uMetalness, uMetalness));

      // various dot products with clamping
      auto fNormalDotToEye = max(float_(0), dot(fInNormal, fToEye));
      auto fNormalDotToLight = max(float_(0), dot(fInNormal, fToLight));
      auto fNormalDotHalf = max(float_(0), dot(fInNormal, fHalf));
      auto fToEyeDotHalf = max(float_(0), dot(fToEye, fHalf));

      // GGX normal distribution term
      auto alpha2 = sqr(sqr(uRoughness));
      auto fNormalDistributionTerm = alpha2 / (sqr(sqr(fNormalDotHalf) * (alpha2 - float_(1)) + float_(1)) * float_(std::numbers::pi_v<float>));

      // Fresnel term
      auto fFresnelTerm = fSpecularColor + (vec3_(1, 1, 1) - fSpecularColor) * exp2((fToEyeDotHalf * float_(-5.55473) + float_(-6.98316)) * fToEyeDotHalf);

      // visibility term (= geometry term with some dot products canceled out by Cook Torrance denominator)
      auto fVisibilityTerm = [&]()
      {
        auto k = sqr(uRoughness + float_(1)) * float_(0.125f);
        auto g1 = [&](ShaderExpression<float> p)
        {
          return float_(1) / (p * (float_(1) - k) + k);
        };
        return g1(fNormalDotToLight) * g1(fNormalDotToEye);
      }();

      // Cook Torrance specular
      auto fSpecular = fFresnelTerm * fNormalDistributionTerm * fVisibilityTerm * float_(0.25f);

      auto fDiffuse = (vec3_(1, 1, 1) - fFresnelTerm) * uColor * (float_(1) - uMetalness) * float_(1 / std::numbers::pi_v<float>);

      auto fReflectance = (fSpecular + fDiffuse) * uLightColor * fNormalDotToLight;

      auto fOutColor = ShaderFragment<vec4>(0);

      auto fragmentProgram =
      (
        fOutColor.Write(cvec(fReflectance, float_(1)))
      );

      return
      {
        .vertex = vertexProgram,
        .fragment = fragmentProgram,
      };
    }());

    {
      GraphicsShader* shaders[] = { pOpaqueShader_ };
      pOpaquePipelineLayout_ = &graphicsDevice_.CreatePipelineLayout(book, shaders);
    }
  }
}
