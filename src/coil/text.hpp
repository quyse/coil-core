#pragma once

#include "base.hpp"
#include <ostream>
#include <string_view>
#include <variant>

namespace Coil
{
  // text, can be outputted into std::ostream
  template <typename Text>
  concept IsText = requires(Text const& text, std::ostream& stream)
  {
    { stream << text } -> std::same_as<std::ostream&>;
  };

  // phrase, which, given args, can be resolved into text
  template <typename Phrase>
  concept IsPhrase = requires(Phrase const& phrase, typename Phrase::Args const& args, std::ostream& stream)
  {
    std::tuple_size_v<typename Phrase::Args> == Phrase::argsIndices.size();
    std::same_as<typename decltype(Phrase::argsIndices)::value_type, size_t>;
    { phrase.Resolve(args) } -> IsText;
  };

  // wraps a value which can be outputted into stream
  template <typename T>
  class ValueText
  {
  public:
    constexpr ValueText(T const& value)
    : _value(value) {}
    constexpr ValueText(T&& value)
    : _value(std::move(value)) {}

    using Args = std::tuple<>;
    static constexpr std::array<size_t, 0> const argsIndices = {};
    constexpr auto Resolve(Args const&) const
    {
      return *this;
    }

    friend std::ostream& operator<<(std::ostream& s, ValueText const& text)
    {
      return s << text._value;
    }

  private:
    T const _value;
  };

  // text literal
  // can be used as non-type template argument
  template <size_t N>
  struct Literal
  {
    template <size_t M>
    constexpr Literal(char const(&s)[M]) requires (N <= M)
    {
      std::copy_n(s, N, this->s);
    }
    constexpr Literal(char const* s)
    {
      std::copy_n(s, N, this->s);
    }

    using Args = std::tuple<>;
    static constexpr std::array<size_t, 0> const argsIndices = {};
    constexpr auto Resolve(Args const&) const
    {
      return *this;
    }

    friend std::ostream& operator<<(std::ostream& s, Literal const& l)
    {
      return s << std::string_view(l.s, l.n);
    }

    char s[N];
    static constexpr size_t n = N;
  };
  template <size_t M>
  Literal(char const(&s)[M]) -> Literal<M - 1>;

  // custom literal
  template <Literal l>
  consteval auto operator""_l()
  {
    return l;
  }

  // compile-time text literal
  template <Literal l>
  struct LiteralText
  {
    using Args = std::tuple<>;
    static constexpr std::array<size_t, 0> const argsIndices = {};
    constexpr auto Resolve(std::tuple<> const&) const
    {
      return *this;
    }

    friend std::ostream& operator<<(std::ostream& s, LiteralText const&)
    {
      return s << l;
    }
  };

  // static text literal
  class StaticText
  {
  public:
    constexpr StaticText(std::string_view sv)
    : _sv(sv) {}
    constexpr StaticText(char const* str, size_t length)
    : _sv(str, length) {}

    using Args = std::tuple<>;
    static constexpr std::array<size_t, 0> const argsIndices = {};
    constexpr auto Resolve(std::tuple<> const&) const
    {
      return *this;
    }

    friend std::ostream& operator<<(std::ostream& s, StaticText const& text)
    {
      return s << text._sv;
    }

  private:
    std::string_view _sv;
  };

  class TextArgHelper
  {
  private:
    template <typename T>
    struct TypeProxy
    {
      using Type = T;
    };

    // combine two types only if they are equal, or missing (std::nullptr_t)
    template <typename A, typename B>
    friend auto operator+(TypeProxy<A> const&, TypeProxy<B> const&)
    {
      static_assert(std::same_as<A, B> || std::is_null_pointer_v<A> || std::is_null_pointer_v<B>, "different type for same argument requested by text chunks");
      if constexpr(std::is_null_pointer_v<A>)
      {
        return TypeProxy<B>();
      }
      else
      {
        return TypeProxy<A>();
      }
    }

    template <typename... Chunks>
    struct ChunksArgsCombiner
    {
      template <typename... CombinedArgs>
      struct Combiner1
      {
        template <size_t... combinedIndices>
        struct Combiner2
        {
          template <size_t... chunksNextIndices>
          struct Combiner3
          {
            static constexpr size_t const minIndex = std::min<size_t>({ []() -> size_t
            {
              if constexpr(chunksNextIndices < Chunks::argsIndices.size())
              {
                return std::get<chunksNextIndices>(Chunks::argsIndices);
              }
              else
              {
                return std::numeric_limits<size_t>::max();
              }
            }()..., std::numeric_limits<size_t>::max() });

            static constexpr bool const notDone = (minIndex < std::numeric_limits<size_t>::max());

            using NextCombiner = decltype([]()
            {
              if constexpr(notDone)
              {
                return std::declval<typename Combiner1<
                  CombinedArgs...,
                  typename decltype((TypeProxy<std::nullptr_t>() + ... +
                    []()
                    {
                      if constexpr(chunksNextIndices < Chunks::argsIndices.size())
                      {
                        if constexpr(std::get<chunksNextIndices>(Chunks::argsIndices) == minIndex)
                        {
                          return TypeProxy<std::tuple_element_t<chunksNextIndices, typename Chunks::Args>>();
                        }
                        else
                        {
                          return TypeProxy<std::nullptr_t>();
                        }
                      }
                      else
                      {
                        return TypeProxy<std::nullptr_t>();
                      }
                    }()
                  ))::Type
                >::template Combiner2<
                  combinedIndices..., minIndex
                >::template Combiner3<
                  []()
                  {
                    if constexpr(chunksNextIndices < Chunks::argsIndices.size())
                    {
                      return std::get<chunksNextIndices>(Chunks::argsIndices) == minIndex ? chunksNextIndices + 1 : chunksNextIndices;
                    }
                    else
                    {
                      return chunksNextIndices;
                    }
                  }()...
                >>();
              }
              else
              {
                return std::declval<void>();
              }
            }());

            using Args = decltype([]()
            {
              if constexpr(notDone)
              {
                return std::declval<typename NextCombiner::Args>();
              }
              else
              {
                return std::declval<std::tuple<CombinedArgs...>>();
              }
            }());

            static constexpr auto const argsIndices = []()
            {
              if constexpr(notDone)
              {
                return NextCombiner::argsIndices;
              }
              else
              {
                return std::array<size_t, sizeof...(combinedIndices)>({ combinedIndices... });
              }
            }();
          };
        };
      };
    };
  public:
    template <typename... Chunks>
    class CombinedChunksArgs
    {
    private:
      using Combiner = typename ChunksArgsCombiner<Chunks...>::template Combiner1<>::template Combiner2<>::template Combiner3<((void)sizeof(Chunks), 0)...>;

    public:
      using Args = typename Combiner::Args;
      static constexpr auto const argsIndices = Combiner::argsIndices;

      static constexpr size_t GetArgRealIndex(size_t index)
      {
        for(size_t i = 0; i < argsIndices.size(); ++i)
          if(argsIndices[i] >= index)
            return i;
        throw "wrong index";
      }

      template <typename Chunk>
      static auto ResolveChunk(Chunk const& chunk, Args const& args)
      {
        return [&]<size_t... I>(std::index_sequence<I...>)
        {
          return chunk.Resolve(typename Chunk::Args(std::get<GetArgRealIndex(std::get<I>(Chunk::argsIndices))>(args)...));
        }(std::make_index_sequence<Chunk::argsIndices.size()>());
      }
    };
  };


  // phrase, combined from several chunks
  template <typename... Chunks>
  class TextSequence
  {
  private:
    using CombinedArgs = TextArgHelper::CombinedChunksArgs<Chunks...>;

  public:
    constexpr TextSequence() = default;
    constexpr TextSequence(Chunks&&... chunks) requires(sizeof...(Chunks) > 0)
    : _chunks(std::forward<Chunks>(chunks)...) {}

    using Args = typename CombinedArgs::Args;
    static constexpr auto const argsIndices = CombinedArgs::argsIndices;
    auto Resolve(Args const& args) const
    {
      return std::apply([&](Chunks const&... chunks)
      {
        return TextSequence<decltype(CombinedArgs::ResolveChunk(chunks, args))...>(CombinedArgs::ResolveChunk(chunks, args)...);
      }, _chunks);
    }

    friend std::ostream& operator<<(std::ostream& s, TextSequence const& text) requires(IsText<Chunks> && ...)
    {
      return std::apply([&](Chunks const&... chunks) -> std::ostream&
      {
        return (s << ... << chunks);
      }, text._chunks);
    }

  private:
    std::tuple<Chunks...> _chunks;
  };

  // phrase chunk resolving to argument
  template <typename Arg, size_t argIndex>
  struct ArgText
  {
    using Args = std::tuple<Arg>;
    static constexpr std::array<size_t, 1> const argsIndices = { argIndex };
    constexpr auto Resolve(Args const& args) const
    {
      return ValueText<Arg>(std::get<0>(args));
    }
    void Resolve(std::ostream& stream, Args const& args) const
    {
      stream << Resolve(args);
    }
  };

  // convenience adapter for text, hiding imlpementation
  template <typename... Args>
  class PhraseInterface
  {
  public:
    virtual void operator()(std::ostream& stream, Args const&... args) const = 0;
  };
  // concrete implementation
  template <IsPhrase T, typename ArgsTuple>
  class PhraseImpl;
  template <IsPhrase T, typename... Args>
  class PhraseImpl<T, std::tuple<Args...>> final : public PhraseInterface<Args...>
  {
  public:
    constexpr PhraseImpl(T const& text)
    : _text(text) {}
    constexpr PhraseImpl(T&& text = {})
    : _text(std::move(text)) {}

    void operator()(std::ostream& stream, Args const&... args) const override
    {
      stream << _text.Resolve(std::tuple<Args const&...>(args...));
    }

  private:
    T _text;
  };
  // type synonym for convenience
  template <IsPhrase T>
  using Phrase = PhraseImpl<T, typename T::Args>;

  // text holding one of multiple text variants
  template <typename... Texts>
  class VariantText
  {
  public:
    VariantText() = delete;
    template <typename... T>
    constexpr VariantText(T&&... init)
    : _variant(std::forward<T>(init)...) {}

    using Args = std::tuple<>;
    static constexpr std::array<size_t, 0> const argsIndices = {};
    constexpr auto Resolve(std::tuple<> const&) const
    {
      return *this;
    }

    friend std::ostream& operator<<(std::ostream& s, VariantText const& text)
    {
      std::visit([&](auto const& text)
      {
        s << text;
      }, text._variant);
      return s;
    }

  private:
    std::variant<Texts...> _variant;
  };

  // phrase substitution method
  template <typename Namespace, Literal l, typename... Args>
  struct PhraseMethod;
}
