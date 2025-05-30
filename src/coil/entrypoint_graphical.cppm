module;

#include "entrypoint.hpp"
#include "base.hpp"

#if defined(COIL_PLATFORM_WINDOWS)

#include "windows.hpp"

#endif

export module coil.core.entrypoint_graphical;

#if defined(COIL_PLATFORM_WINDOWS)

extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
  int argsCount;
  LPWSTR* argsStrs = CommandLineToArgvW(pCmdLine, &argsCount);
  std::vector<std::string> args(argsCount);
  {
    std::vector<char> argBuf;
    for(int i = 0; i < argsCount; ++i)
    {
      int argSize = WideCharToMultiByte(CP_UTF8, 0, argsStrs[i], -1, NULL, 0, NULL, NULL);
      argBuf.assign(argSize, '\0');
      WideCharToMultiByte(CP_UTF8, 0, argsStrs[i], -1, argBuf.data(), argBuf.size(), NULL, NULL);
      args[i] = argBuf.data();
    }
  }
  LocalFree(argsStrs);

  return COIL_ENTRY_POINT(std::move(args));
}

#else

int main(int argc, char** argv)
{
  return COIL_ENTRY_POINT(std::vector<std::string>(argv, argv + argc));
}

#endif
