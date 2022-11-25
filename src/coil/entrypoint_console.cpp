#include "entrypoint.hpp"

#if defined(___COIL_PLATFORM_WINDOWS)

#include "windows.hpp"

int wmain(int argc, wchar_t** argv)
{
  std::vector<std::string> args(argc);
  for(int i = 0; i < argc; ++i)
  {
    int argSize = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
    args[i].assign(argSize, '\0');
    WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, args[i].data(), args[i].size(), NULL, NULL);
  }

  return Coil::g_entryPoint(std::move(args));
}

#else

int main(int argc, char** argv)
{
  return Coil::g_entryPoint(std::vector<std::string>(argv, argv + argc));
}

#endif
