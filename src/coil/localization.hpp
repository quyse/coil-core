#pragma once

#include "text.hpp"
#include "localization_en.hpp"
#include "localization_ru.hpp"

namespace Coil
{
  // phrase methods

  template <typename Language, size_t argIndex, typename... Args>
  struct PhraseMethod<Language, "plural", ArgText<uint64_t, argIndex>, Args...>
  {
    static consteval auto Call()
    {
      return typename Language::template Plural<argIndex, Args...>();
    }
  };
}
