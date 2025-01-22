module;

#include <concepts>
#include <cstdint>
#include <tuple>

export module coil.core.localization;

import coil.core.base;
import coil.core.text;

export namespace Coil
{
  // supported languages
  // not very precise list, does not strictly correspond to any standard
  // explicit values are just for stability
  enum class Language : uint32_t
  {
    Arabic = "ar"_c,
    Belarusian = "be"_c,
    Bulgarian = "bg"_c,
    ChineseSimplified = "zhS"_c,
    ChineseTraditional = "zhT"_c,
    Czech = "cs"_c,
    Danish = "da"_c,
    Dutch = "nl"_c,
    English = "en"_c,
    EnglishAmerican = "enUS"_c,
    EnglishBritish = "enGB"_c,
    Finnish = "fi"_c,
    French = "fr"_c,
    German = "de"_c,
    Greek = "el"_c,
    Hebrew = "he"_c,
    Hindi = "hi"_c,
    Hungarian = "hu"_c,
    Italian = "it"_c,
    Japanese = "ja"_c,
    Kazakh = "kk"_c,
    Korean = "ko"_c,
    Persian = "fa"_c,
    Polish = "pl"_c,
    Portuguese = "pt"_c,
    Russian = "ru"_c,
    Serbian = "sr"_c,
    Spanish = "es"_c,
    Swedish = "sv"_c,
    Thai = "th"_c,
    Turkish = "tr"_c,
    Ukrainian = "uk"_c,
    Vietnamese = "vi"_c,
  };

  // language tag
  // strictly corresponds to ISO 639-1 two-letter language tag
  // used as primary language subtag in IETF BCP 47
  // https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
  enum class LanguageTag : uint16_t
  {
    Arabic = "ar"_c,
    Belarusian = "be"_c,
    Bulgarian = "bg"_c,
    Chinese = "zh"_c,
    Czech = "cs"_c,
    Danish = "da"_c,
    Dutch = "nl"_c,
    English = "en"_c,
    Finnish = "fi"_c,
    French = "fr"_c,
    German = "de"_c,
    Greek = "el"_c,
    Hebrew = "he"_c,
    Hindi = "hi"_c,
    Hungarian = "hu"_c,
    Italian = "it"_c,
    Japanese = "ja"_c,
    Kazakh = "kk"_c,
    Korean = "ko"_c,
    Persian = "fa"_c,
    Polish = "pl"_c,
    Portuguese = "pt"_c,
    Russian = "ru"_c,
    Serbian = "sr"_c,
    Spanish = "es"_c,
    Swedish = "sv"_c,
    Thai = "th"_c,
    Turkish = "tr"_c,
    Ukrainian = "uk"_c,
    Vietnamese = "vi"_c,
  };

  // language script
  // strictly corresponds to ISO 15924 four-letter script codes
  // https://unicode.org/iso15924/iso15924-codes.html
  enum class LanguageScript : uint32_t
  {
    Arabic = "Arab"_c,
    Cyrillic = "Cyrl"_c,
    Devanagari = "Deva"_c,
    Greek = "Grek"_c,
    Han = "Hani"_c,
    HanSimplified = "Hans"_c,
    HanTraditional = "Hant"_c,
    Hebrew = "Hebr"_c,
    Korean = "Kore"_c,
    Latin = "Latn"_c,
    Thai = "Thai"_c,
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


  // phrase methods

  template <typename Language, size_t argIndex, typename... Args>
  struct PhraseMethod<Language, "plural", ArgText<uint64_t, argIndex>, Args...>
  {
    static consteval auto Call()
    {
      return typename Language::template Plural<argIndex, Args...>();
    }
  };


  // helpers shared between languages

  struct LocalizationCyrillic
  {
    template <size_t argIndex, typename One, typename Few, typename Many>
    class Plural
    {
    private:
      using CombinedArgs = TextArgHelper::CombinedChunksArgs<ArgText<uint64_t, argIndex>, One, Few, Many>;

      One _one;
      Few _few;
      Many _many;

    public:
      constexpr Plural(One&& one, Few&& few, Many&& many)
      : _one(std::move(one)), _few(std::move(few)), _many(std::move(many)) {}

      using Args = typename CombinedArgs::Args;
      static constexpr auto const argsIndices = CombinedArgs::argsIndices;

      VariantText<
        decltype(CombinedArgs::ResolveChunk(std::declval<One>(), std::declval<Args>())),
        decltype(CombinedArgs::ResolveChunk(std::declval<Few>(), std::declval<Args>())),
        decltype(CombinedArgs::ResolveChunk(std::declval<Many>(), std::declval<Args>()))
      > Resolve(Args const& args) const
      {
        uint64_t n = std::get<CombinedArgs::GetArgRealIndex(argIndex)>(args);

        if((n / 10) % 10 == 1) return
        {
          std::in_place_index<2>,
          CombinedArgs::ResolveChunk(_many, args),
        };

        n %= 10;

        if(n == 0 || n >= 5) return
        {
          std::in_place_index<2>,
          CombinedArgs::ResolveChunk(_many, args),
        };

        if(n == 1) return
        {
          std::in_place_index<0>,
          CombinedArgs::ResolveChunk(_one, args),
        };
        else return
        {
          std::in_place_index<1>,
          CombinedArgs::ResolveChunk(_few, args),
        };
      }
    };
  };


  // specific languages

  template <>
  struct Localization<Language::Arabic>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Arabic,
      .script = LanguageScript::Arabic,
      .direction = LanguageDirection::RightToLeft,
    };
  };
  static_assert(IsLanguage<Language::Arabic>);

  template <>
  struct Localization<Language::Belarusian>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Belarusian,
      .script = LanguageScript::Cyrillic,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Belarusian>);

  template <>
  struct Localization<Language::Bulgarian>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Bulgarian,
      .script = LanguageScript::Cyrillic,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Bulgarian>);

  template <>
  struct Localization<Language::ChineseSimplified>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Chinese,
      .script = LanguageScript::HanSimplified,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::ChineseSimplified>);

  template <>
  struct Localization<Language::ChineseTraditional>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Chinese,
      .script = LanguageScript::HanTraditional,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::ChineseTraditional>);

  template <>
  struct Localization<Language::Czech>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Czech,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Czech>);

  template <>
  struct Localization<Language::Danish>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Danish,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Danish>);

  template <>
  struct Localization<Language::Dutch>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Dutch,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Dutch>);

  template <>
  struct Localization<Language::English>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::English,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };

    template <size_t argIndex, typename One, typename Many>
    class Plural
    {
    private:
      using CombinedArgs = TextArgHelper::CombinedChunksArgs<ArgText<uint64_t, argIndex>, One, Many>;

      One _one;
      Many _many;

    public:
      constexpr Plural(One&& one, Many&& many)
      : _one(std::move(one)), _many(std::move(many)) {}

      using Args = typename CombinedArgs::Args;
      static constexpr auto const argsIndices = CombinedArgs::argsIndices;

      VariantText<
        decltype(CombinedArgs::ResolveChunk(std::declval<One>(), std::declval<Args>())),
        decltype(CombinedArgs::ResolveChunk(std::declval<Many>(), std::declval<Args>()))
      > Resolve(Args const& args) const
      {
        uint64_t n = std::get<CombinedArgs::GetArgRealIndex(argIndex)>(args);

        if(n == 1) return
        {
          std::in_place_index<0>,
          CombinedArgs::ResolveChunk(_one, args),
        };
        else return
        {
          std::in_place_index<1>,
          CombinedArgs::ResolveChunk(_many, args),
        };
      }
    };
  };
  template <>
  struct Localization<Language::EnglishAmerican> : public Localization<Language::English>
  {
  };
  template <>
  struct Localization<Language::EnglishBritish> : public Localization<Language::English>
  {
  };
  static_assert(IsLanguage<Language::English>);
  static_assert(IsLanguage<Language::EnglishAmerican>);
  static_assert(IsLanguage<Language::EnglishBritish>);

  template <>
  struct Localization<Language::Finnish>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Finnish,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Finnish>);

  template <>
  struct Localization<Language::French>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::French,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::French>);

  template <>
  struct Localization<Language::German>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::German,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::German>);

  template <>
  struct Localization<Language::Greek>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Greek,
      .script = LanguageScript::Greek,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Greek>);

  template <>
  struct Localization<Language::Hebrew>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Hebrew,
      .script = LanguageScript::Hebrew,
      .direction = LanguageDirection::RightToLeft,
    };
  };
  static_assert(IsLanguage<Language::Hebrew>);

  template <>
  struct Localization<Language::Hindi>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Hindi,
      .script = LanguageScript::Devanagari,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Hindi>);

  template <>
  struct Localization<Language::Hungarian>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Hungarian,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Hebrew>);

  template <>
  struct Localization<Language::Italian>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Italian,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Italian>);

  template <>
  struct Localization<Language::Japanese>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Japanese,
      .script = LanguageScript::Han,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Japanese>);

  template <>
  struct Localization<Language::Kazakh>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Kazakh,
      .script = LanguageScript::Cyrillic,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Kazakh>);

  template <>
  struct Localization<Language::Korean>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Korean,
      .script = LanguageScript::Korean,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Korean>);

  template <>
  struct Localization<Language::Persian>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Persian,
      .script = LanguageScript::Arabic,
      .direction = LanguageDirection::RightToLeft,
    };
  };
  static_assert(IsLanguage<Language::Persian>);

  template <>
  struct Localization<Language::Polish>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Polish,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Polish>);

  template <>
  struct Localization<Language::Portuguese>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Portuguese,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Portuguese>);

  template <>
  struct Localization<Language::Russian> : public LocalizationCyrillic
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Russian,
      .script = LanguageScript::Cyrillic,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Russian>);

  template <>
  struct Localization<Language::Serbian>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Serbian,
      .script = LanguageScript::Cyrillic,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Serbian>);

  template <>
  struct Localization<Language::Spanish>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Spanish,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Spanish>);

  template <>
  struct Localization<Language::Swedish>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Swedish,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Swedish>);

  template <>
  struct Localization<Language::Thai>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Thai,
      .script = LanguageScript::Thai,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Thai>);

  template <>
  struct Localization<Language::Turkish>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Turkish,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Turkish>);

  template <>
  struct Localization<Language::Ukrainian>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Ukrainian,
      .script = LanguageScript::Cyrillic,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Ukrainian>);

  template <>
  struct Localization<Language::Vietnamese>
  {
    static constexpr LanguageInfo const info =
    {
      .tag = LanguageTag::Vietnamese,
      .script = LanguageScript::Latin,
      .direction = LanguageDirection::LeftToRight,
    };
  };
  static_assert(IsLanguage<Language::Vietnamese>);
}
