#include "entrypoint.hpp"
#include "json.hpp"
#include "text.hpp"
#include "unicode.hpp"
#include "fs.hpp"
#include <map>
#include <iostream>
#include <fstream>

using namespace Coil;

struct LocalizationLanguageConfig
{
  // namespace with methods
  std::string lang;
  // string file
  std::optional<std::string> file;
  // fallback language
  std::optional<std::string> fallback;
};
template <>
struct JsonDecoder<LocalizationLanguageConfig> : public JsonDecoderBase<LocalizationLanguageConfig>
{
  static LocalizationLanguageConfig Decode(json const& j)
  {
    return
    {
      .lang = JsonDecoder<std::string>::DecodeField(j, "lang"),
      .file = JsonDecoder<std::optional<std::string>>::DecodeField(j, "file", {}),
      .fallback = JsonDecoder<std::optional<std::string>>::DecodeField(j, "fallback", {}),
    };
  }
};

struct LocalizationConfig
{
  std::map<std::string, LocalizationLanguageConfig> langs;
};
template <>
struct JsonDecoder<LocalizationConfig> : public JsonDecoderBase<LocalizationConfig>
{
  static LocalizationConfig Decode(json const& j)
  {
    return
    {
      .langs = JsonDecoder<std::map<std::string, LocalizationLanguageConfig>>::DecodeField(j, "langs"),
    };
  }
};

struct LocalizedString
{
  size_t index;
  // strings by lang
  std::vector<std::optional<std::string>> strings;
  std::optional<uint64_t> argMask;
};

void OutputCppString(std::ostream& stream, std::string const& str, size_t indent = 0)
{
  size_t const lineLimit = 120;
  size_t cnt = 0;
  for(Unicode::Iterator<char, char32_t, char const*> i = str.c_str(); *i; ++i)
  {
    if(indent)
    {
      if(cnt >= lineLimit)
      {
        stream << "\"\n";
        cnt = 0;
      }
    }
    if(cnt == 0)
    {
      for(size_t t = 0; t < indent; ++t)
        stream << ' ';
      stream << '\"';
    }
    ++cnt;

    char32_t c = *i;
    char const* s;
    switch(c)
    {
    case 0: s = R"(\0)"; break;
    case 1: s = R"(\x01)"; break;
    case 2: s = R"(\x02)"; break;
    case 3: s = R"(\x03)"; break;
    case 4: s = R"(\x04)"; break;
    case 5: s = R"(\x05)"; break;
    case 6: s = R"(\x06)"; break;
    case 7: s = R"(\a)"; break;
    case 8: s = R"(\b)"; break;
    case 9: s = R"(\t)"; break;
    case 10: s = R"(\n)"; break;
    case 11: s = R"(\v)"; break;
    case 12: s = R"(\f)"; break;
    case 13: s = R"(\r)"; break;
    case 14: s = R"(\x0e)"; break;
    case 15: s = R"(\x0f)"; break;
    case 16: s = R"(\x10)"; break;
    case 17: s = R"(\x11)"; break;
    case 18: s = R"(\x12)"; break;
    case 19: s = R"(\x13)"; break;
    case 20: s = R"(\x14)"; break;
    case 21: s = R"(\x15)"; break;
    case 22: s = R"(\x16)"; break;
    case 23: s = R"(\x17)"; break;
    case 24: s = R"(\x18)"; break;
    case 25: s = R"(\x19)"; break;
    case 26: s = R"(\x1a)"; break;
    case 27: s = R"(\x1b)"; break;
    case 28: s = R"(\x1c)"; break;
    case 29: s = R"(\x1d)"; break;
    case 30: s = R"(\x1e)"; break;
    case 31: s = R"(\x1f)"; break;
    case '\\': s = R"(\\)"; break;
    case '"': s = R"(\")"; break;
    default:
      {
        char32_t cc[] = { c, 0 };
        for(Unicode::Iterator<char32_t, char, char32_t const*> j = cc; *j; ++j)
          stream << *j;
      }
      continue;
    }
    stream << s;
  }
  if(indent) stream << "\"\n";
  else stream << "\"";
}

class PhraseMaker
{
public:
  enum class TokenType
  {
    String,
    Arg,
    BraceOpen,
    Separator,
    BraceClose,
  };

  struct Token
  {
    TokenType type;
    std::string_view string;
  };

  PhraseMaker()
  {
    Reset();
  }

  void Reset()
  {
    _tokens.clear();
    _tokenIndex = 0;
    _argMask = 0;
    _typeStream.str({});
    _initStream.str({});
  }

  std::pair<size_t, size_t> AddStringToCombined(std::string_view sv)
  {
    std::pair<size_t, size_t> r;
    r.first = _combinedStringsSize;
    _combinedStringsStream << sv;
    _combinedStringsSize += sv.length();
    r.second = _combinedStringsSize;
    return r;
  }

  void SetLang(std::string const& lang)
  {
    _pLang = &lang;
  }

  void ParseTokens(std::string_view s)
  {
    for(auto i = s.begin(); i != s.end(); )
    {
      char c = *i;
      if(c == '$')
      {
        auto j = i;
        ++j;
        if(j == s.end()) throw "$ at the end of string";
        c = *j;
        if(c == '{')
        {
          _tokens.push_back(
          {
            .type = TokenType::BraceOpen,
            .string = { i, i + 2 },
          });
        }
        else if(c >= '0' && c <= '9')
        {
          _tokens.push_back(
          {
            .type = TokenType::Arg,
            .string = { i, i + 2 },
          });
        }
        else
        {
          _tokens.push_back(
          {
            .type = TokenType::String,
            .string = { j, j + 1 },
          });
        }
        i = j;
        ++i;
      }
      else if(c == '|')
      {
        _tokens.push_back(
        {
          .type = TokenType::Separator,
          .string = { i, i + 1 },
        });
        ++i;
      }
      else if(c == '}')
      {
        _tokens.push_back(
        {
          .type = TokenType::BraceClose,
          .string = { i, i + 1 },
        });
        ++i;
      }
      else
      {
        auto j = i;
        for(++j; j != s.end(); ++j)
        {
          c = *j;
          if(c == '$' || c == '|' || c == '}') break;
        }
        _tokens.push_back(
        {
          .type = TokenType::String,
          .string = { i, j },
        });
        i = j;
      }
    }
  }

  void ParsePhrase()
  {
    _typeStream << "TextSequence<";
    _initStream << "{ ";
    bool first = true;
    auto comma = [&]()
    {
      if(first) first = false;
      else
      {
        _typeStream << ", ";
        _initStream << ", ";
      }
    };
    bool ok = true;
    while(ok && _tokenIndex < _tokens.size())
    {
      switch(_tokens[_tokenIndex].type)
      {
      case TokenType::String:
        {
          comma();
          auto p = AddStringToCombined(_tokens[_tokenIndex].string);
          _typeStream << "StaticText";
          _initStream << "{ G + " << p.first << ", " << (p.second - p.first) << " }";
          ++_tokenIndex;
        }
        break;
      case TokenType::Arg:
        {
          comma();
          uint64_t argIndex = _tokens[_tokenIndex].string[1] - '0';
          _typeStream << "ArgText<uint64_t, " << argIndex << ">";
          _initStream << "{}";
          _argMask |= 1 << argIndex;
          ++_tokenIndex;
        }
        break;
      case TokenType::BraceOpen:
        {
          comma();
          ++_tokenIndex;

          if(_tokenIndex >= _tokens.size() || _tokens[_tokenIndex].type != TokenType::String) throw Exception("${ must be followed by phrase method name");

          bool firstInTypeStream = true;
          bool firstInInitStream = true;
          auto comma = [&]()
          {
            if(firstInTypeStream) firstInTypeStream = false;
            else _typeStream << ", ";
            if(firstInInitStream) firstInInitStream = false;
            else _initStream << ", ";
          };

          auto methodName = _tokens[_tokenIndex].string;
          if(methodName.substr(0, 6) == "plural")
          {
            if(methodName.length() != 7) throw Exception("wrong format for pluralN method");
            _typeStream << "Localization::" << *_pLang << "::Plural<" << (methodName[6] - '0');
            _initStream << "{ ";
            firstInTypeStream = false;
          }
          else throw Exception() << "unsupported phrase method: " << methodName;

          ++_tokenIndex;
          bool ok = true;
          while(ok)
          {
            if(_tokenIndex >= _tokens.size()) throw Exception("phrase method not finished");
            switch(_tokens[_tokenIndex].type)
            {
            case TokenType::Separator:
              comma();
              ++_tokenIndex;
              ParsePhrase();
              break;
            case TokenType::BraceClose:
              ++_tokenIndex;
              ok = false;
              break;
            default:
              throw Exception("unexpected token in phrase method");
            }
          }
          _typeStream << ">";
          _initStream << " }";
        }
        break;
      default:
        ok = false;
        break;
      }
    }
    _typeStream << ">";
    _initStream << " }";
  }

  uint64_t GetArgMask() const
  {
    return _argMask;
  }

  void OutputDef(std::ostream& stream, size_t index)
  {
    stream << "  constinit Phrase<" << std::move(_typeStream).str() << "> const s" << index << " = { " << std::move(_initStream).str() << " };\n";
  }

  void OutputCombinedStrings(std::ostream& stream)
  {
    stream << R"(  constinit char const G[] =
)";
    OutputCppString(stream, std::move(_combinedStringsStream).str(), 4);
    stream << R"(  ;
)";
  }

private:
  std::string const* _pLang = nullptr;
  std::vector<Token> _tokens;
  size_t _tokenIndex;
  uint64_t _argMask;
  std::ostringstream _typeStream;
  std::ostringstream _initStream;

  std::ostringstream _combinedStringsStream;
  size_t _combinedStringsSize = 0;
};

___COIL_ENTRY_POINT = [](std::vector<std::string>&& args) -> int
{
  std::string workingDir = ".";
  std::string localizationConfigFileName;
  std::string outputDir = ".";

  if(args.size() <= 1)
  {
    std::cerr << args[0] << " usage:\n"
      << args[0] << " [-v] [-w <working dir>] <localization config> [<output dir>]\n"
    ;
    return 1;
  };

  for(size_t i = 1; i < args.size(); ++i)
  {
    auto const& arg = args[i];
    if(arg.length() == 2 && arg[0] == '-')
    {
      switch(arg[1])
      {
      case 'w':
        if(++i >= args.size())
        {
          std::cerr << "-w requires argument\n";
          return 1;
        }
        workingDir = args[i];
        break;
      default:
        std::cerr << "unknown option: " << arg << "\n";
        return 1;
      }
    }
    else
    {
      localizationConfigFileName = arg;

      if(++i >= args.size()) break;
      outputDir = args[i];

      break;
    }
  }

  if(localizationConfigFileName.empty())
  {
    std::cerr << "localization config must be specified\n";
    return 1;
  }

  Book book;

  // setup for errors
  std::vector<std::string> errors;
  auto addError = [&](std::string const& error)
  {
    errors.push_back(error);
  };
  auto checkErrors = [&]() -> bool
  {
    if(errors.empty()) return true;
    for(size_t i = 0; i < errors.size(); ++i)
      std::cerr << errors[i] << '\n';
    return false;
  };

  // read global config
  auto config = JsonDecoder<LocalizationConfig>::Decode(ParseJsonBuffer(File::MapRead(book, localizationConfigFileName)));

  size_t langsCount = config.langs.size();
  std::vector<std::string> langsIds;
  std::map<std::string, size_t> langsIndices;

  // global list of strings
  std::map<std::string, LocalizedString> strings;

  // language configs
  std::vector<LocalizationLanguageConfig> langsConfigs(langsCount);

  // loop languages
  for(auto const& [langId, langConfig] : config.langs)
  {
    size_t langIndex = langsIds.size();
    langsIds.push_back(langId);
    langsIndices[langId] = langIndex;

    langsConfigs[langIndex] = langConfig;

    // read language strings
    if(langConfig.file.has_value())
    {
      for(auto j = ParseJsonBuffer(File::MapRead(book, workingDir + "/" + langConfig.file.value())); auto const& [key, value] : j.items())
      {
        auto& string = strings.insert({ key, {} }).first->second;
        if(string.strings.size() != langsCount) string.strings.resize(langsCount);
        string.strings[langIndex] = value;
      }
    }
  }

  // assign string indices
  {
    size_t i = 0;
    for(auto& [key, value] : strings)
      value.index = i++;
  }

  // resolve fallbacks
  std::vector<std::optional<size_t>> langsFallbacks(langsCount);
  for(size_t langIndex = 0; langIndex < langsCount; ++langIndex)
  {
    if(langsConfigs[langIndex].fallback.has_value())
    {
      auto i = config.langs.find(langsConfigs[langIndex].fallback.value());
      if(i == config.langs.end())
      {
        std::ostringstream ss;
        ss << "language \"" << langsIds[langIndex] << "\" refers to missing fallback language \"" << langsConfigs[langIndex].fallback.value() << "\"";
        addError(ss.str());
      }
      else
      {
        langsFallbacks[langIndex] = langsIndices[i->first];
      }
    }
  }
  if(!checkErrors()) return 1;

  // generate <lang>.cpp files
  for(size_t langIndex = 0; langIndex < langsCount; ++langIndex)
  {
    std::ofstream outStream(outputDir + "/" + langsIds[langIndex] + ".cpp", std::ios::out | std::ios::trunc);

    outStream << R"(#include "localized.hpp"

using namespace Coil;

namespace
{
)";

    PhraseMaker phraseMaker;
    std::ostringstream defsStream;
    std::ostringstream tableStream;

    size_t index = 0;
    for(auto i = strings.begin(); i != strings.end(); ++i)
    {
      phraseMaker.Reset();

      bool ok = false;
      for(size_t stringLangIndex = langIndex;;)
      {
        auto const& string = i->second.strings[stringLangIndex];
        if(string.has_value())
        {
          phraseMaker.SetLang(langsConfigs[stringLangIndex].lang);
          phraseMaker.ParseTokens(string.value());
          ok = true;
          break;
        }

        if(!langsFallbacks[stringLangIndex].has_value())
        {
          std::ostringstream ss;
          ss << "string \"" << i->first << "\" in language \"" << langsIds[langIndex] << "\" is not specified and there is no fallback language";
          addError(ss.str());
          break;
        }

        stringLangIndex = langsFallbacks[stringLangIndex].value();
      }
      if(!ok) continue;

      phraseMaker.ParsePhrase();
      phraseMaker.OutputDef(defsStream, index);

      uint64_t argMask = phraseMaker.GetArgMask();
      if(i->second.argMask.has_value() && i->second.argMask.value() != argMask)
      {
        std::ostringstream ss;
        ss << "string " << i->first << " requires different sets of args in different languages";
        addError(ss.str());
      }
      i->second.argMask = argMask;

      tableStream << "s" << index << ",\n";

      ++index;
    }

    phraseMaker.OutputCombinedStrings(outStream);
    outStream << "\n";
    outStream << std::move(defsStream).str();

    outStream << R"(
  constinit void const* const T[] =
  {
)";
    for(size_t i = 0; i < strings.size(); ++i)
      outStream << "    &s" << i << ",\n";
    outStream << R"(  };
}

namespace Localized
{
  template <> constinit Set const g_set<)" << langIndex << R"(> =
  {
    .name = )";
    OutputCppString(outStream, langsIds[langIndex]);
    outStream << R"(,
    .phrases = T,
  };
}
)";
  }
  if(!checkErrors()) return 1;

  // global header
  {
    std::ofstream outStream(outputDir + "/localized.hpp", std::ios::out | std::ios::trunc);
    outStream << R"(#pragma once

#include <coil/localization.hpp>

namespace Localized
{
  using namespace Coil;

  template <Literal l>
  struct PhraseInfo;

  struct Set
  {
    char const* const name;
    void const* const* const phrases;
    static Set const* Current;
  };

  template <size_t langIndex> extern Set const g_set;
)";
    // specialization declarations for g_set
    for(size_t langIndex = 0; langIndex < langsCount; ++langIndex)
      outStream << R"(  template <> extern Set const g_set<)" << langIndex << R"(>;
)";
    // explicit instantiation declarations for g_set
    for(size_t langIndex = 0; langIndex < langsCount; ++langIndex)
      outStream << R"(  extern template Set const g_set<)" << langIndex << R"(>;
)";
    // array of sets
    outStream << "  extern Set const* const Sets[" << langsCount << R"(];
)";

    for(auto const& [key, value] : strings)
    {
      outStream << R"(
  template <> struct PhraseInfo<)";
      OutputCppString(outStream, key);
      outStream << R"(>
  {
    using Type = PhraseInterface<)";
      bool first = true;
      for(size_t i = 0; i < 64; ++i)
        if(value.argMask.value() & (uint64_t(1) << i))
        {
          if(first) first = false;
          else outStream << ", ";
          outStream << "uint64_t";
        }
      outStream << R"(>;
    static constexpr size_t const index = )" << value.index << R"(;
  };
)";
    }
    outStream << R"(
  template <Literal l>
  auto const& operator""_loc()
  {
    using Info = PhraseInfo<l>;
    return *(typename Info::Type const*)Set::Current->phrases[Info::index];
  }
}
)";
  }

  // global implementation
  {
    std::ofstream outStream(outputDir + "/localized.cpp", std::ios::out | std::ios::trunc);
    outStream << R"(#include "localized.hpp"

namespace Localized
{
  constinit Set const* Set::Current = nullptr;
  constinit Set const* const Sets[] =
  {
)";
    for(size_t langIndex = 0; langIndex < langsCount; ++langIndex)
      outStream << "    &g_set<" << langIndex << R"(>,
)";
    outStream << R"(  };
};
)";
  }

  return 0;
};
