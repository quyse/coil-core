#pragma once

#include "text.hpp"

namespace Coil
{
  // supported languages
  // to keep things simple, we only recognise major languages
  // e.g. no distinction for American/British English
  enum class Language
  {
    English,
    Russian,
  };

  // language tag
  // corresponds to ISO 639-1 two-letter language tag
  // used as primary language subtag in IETF BCP 47
  // https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
  enum class LanguageTag : uint16_t
  {
    English = "en"_c,
    Russian = "ru"_c,
  };

  // language script
  // corresponds to ISO 15924 four-letter script codes
  // https://unicode.org/iso15924/iso15924-codes.html
  enum class LanguageScript : uint32_t
  {
    Latin = "Latn"_c,
    Cyrillic = "Cyrl"_c,
  };

  // horizontal writing direction for language
  enum class LanguageDirection
  {
    LeftToRight,
    RightToLeft,
  };

  struct LanguageInfo
  {
    LanguageTag tag;
    LanguageScript script;
    LanguageDirection direction;
  };

  template <Language language>
  struct Localization;

  template <Language language>
  concept IsLanguage = requires
  {
    std::same_as<decltype(Localization<language>::info), LanguageInfo>;
  };
}

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
