#pragma once

namespace Coil
{
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

  static_assert(IsLanguage<Language::English>);
}
