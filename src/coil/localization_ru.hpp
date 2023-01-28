#pragma once

namespace Coil::Localization::Ru
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
}
