#include "entrypoint.hpp"

#if defined(COIL_PLATFORM_WINDOWS)

#include "windows.hpp"

int wmain(int argc, wchar_t** argv)
{
  std::vector<std::string> args(argc);
  for(int i = 0; i < argc; ++i)
  {
    auto& arg = args[i];
    int argSize = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL) - 1;
    arg.assign(argSize, '\0');
    WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, arg.data(), arg.size(), NULL, NULL);
  }

  return COIL_ENTRY_POINT(std::move(args));
}

#else

int main(int argc, char** argv)
{
  return COIL_ENTRY_POINT(std::vector<std::string>(argv, argv + argc));
}

#endif
