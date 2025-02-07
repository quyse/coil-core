module;

#include <cstdint>

export module coil.core.render.realistic.formats;

import coil.core.math;

export namespace Coil
{
  // vertex with position+normal
  template <template <typename> typename T>
  struct VertexPN
  {
    T<vec3_ua> position;
    T<vec3_ua> normal;
  };
  // vertex with position+normal+texcoord
  template <template <typename> typename T>
  struct VertexPNT
  {
    T<vec3_ua> position;
    T<vec3_ua> normal;
    T<vec2_ua> texcoord;
  };
  // vertex with position+normal+texcoord+bones
  template <template <typename> typename T>
  struct VertexPNTB
  {
    T<vec3_ua> position;
    T<vec3_ua> normal;
    T<vec2_ua> texcoord;
    T<vec4_ua> bonesWeights;
    T<xvec_ua<uint8_t, 4>> bonesIndexes;
  };

  // vertex with rotation+offset
  template <template <typename> typename T>
  struct VertexRO
  {
    T<quat_ua> rotation;
    T<vec3_ua> offset;
  };

  template <template <typename> typename T>
  struct ScreenVertex
  {
    T<vec2_ua> position;
  };
}
