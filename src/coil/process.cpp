#include "process.hpp"
#include "appidentity.hpp"
#include "fs.hpp"
#if defined(COIL_PLATFORM_WINDOWS)
#include "unicode.hpp"
#include "windows.hpp"
#include <shlobj.h>
#include <knownfolders.h>
#elif defined(COIL_PLATFORM_POSIX)
#include <unistd.h>
#endif
#include <filesystem>

#if defined(COIL_PLATFORM_WINDOWS)
namespace
{
  std::string GetKnownFolder(REFKNOWNFOLDERID pId)
  {
    std::string location;
    Coil::CoTaskMemPtr<wchar_t> pLocation;
    Coil::CheckHResult(SHGetKnownFolderPath(pId, 0, NULL, &pLocation), "failed to get known folder path");
    Coil::Unicode::Convert<char16_t, char>((wchar_t const*)pLocation, location);
    return location;
  }
}
#endif

namespace
{
  std::string const& GetHomeLocation()
  {
    static std::string const location =
#if defined(COIL_PLATFORM_WINDOWS)
    GetKnownFolder(FOLDERID_Profile);
#elif defined(COIL_PLATFORM_POSIX)
    []()
    {
      char const* str = std::getenv("HOME");
      return str ? str : ".";
    }();
#endif
    return location;
  }

  std::string const& GetConfigLocation()
  {
    static std::string const location =
#if defined(COIL_PLATFORM_WINDOWS)
    GetKnownFolder(FOLDERID_RoamingAppData);
#elif defined(COIL_PLATFORM_POSIX)
    []() -> std::string
    {
      char const* str = std::getenv("XDG_CONFIG_HOME");
      if(str) return str;
      return GetHomeLocation() + "/.config";
    }();
#endif
    return location;
  }

  std::string const& GetDataLocation()
  {
    static std::string const location =
#if defined(COIL_PLATFORM_WINDOWS)
    GetKnownFolder(FOLDERID_RoamingAppData);
#elif defined(COIL_PLATFORM_POSIX)
    []() -> std::string
    {
      char const* str = std::getenv("XDG_DATA_HOME");
      if(str) return str;
      return GetHomeLocation() + "/.local/share";
    }();
#endif
    return location;
  }

  std::string const& GetStateLocation()
  {
    static std::string const location =
#if defined(COIL_PLATFORM_WINDOWS)
    GetKnownFolder(FOLDERID_LocalAppData);
#elif defined(COIL_PLATFORM_POSIX)
    []() -> std::string
    {
      char const* str = std::getenv("XDG_STATE_HOME");
      if(str) return str;
      return GetHomeLocation() + "/.local/state";
    }();
#endif
    return location;
  }
}

namespace Coil
{
  std::string GetAppKnownLocation(AppKnownLocation location)
  {
    switch(location)
    {
    case AppKnownLocation::Config:
      return GetConfigLocation() + FsPathSeparator + AppIdentity::GetInstance().PackageName();
    case AppKnownLocation::Data:
      return GetDataLocation() + FsPathSeparator + AppIdentity::GetInstance().PackageName();
    case AppKnownLocation::State:
      return GetStateLocation() + FsPathSeparator + AppIdentity::GetInstance().PackageName();
    default:
      throw Exception{"unsupported app known location"};
    }
  }

  std::string EnsureAppKnownLocation(AppKnownLocation location)
  {
    auto path = GetAppKnownLocation(location);
    std::filesystem::create_directories(path);
    return std::move(path);
  }

  void RunProcessAndForget(std::string const& program, std::vector<std::string> const& arguments)
  {
#if defined(COIL_PLATFORM_WINDOWS)
    // not clear how to transition from arguments to command line, Windows escaping is crazy
    throw Exception{"RunProcessAndForget is not implemented on Windows yet"};
#elif defined(COIL_PLATFORM_POSIX)
    std::vector<char*> args;
    args.reserve(arguments.size() + 2);
    args.push_back((char*)program.c_str());
    for(size_t i = 0; i < arguments.size(); ++i)
      args.push_back((char*)arguments[i].c_str());
    args.push_back(nullptr);

    if(::fork() == 0)
    {
      ::execvp(program.c_str(), args.data());
    }
#endif
  }

  void RunOrOpenFile(std::string const& fileName)
  {
#if defined(COIL_PLATFORM_WINDOWS)
    std::wstring fileNameStr;
    Unicode::Convert<char, char16_t>(fileName.begin(), fileName.end(), fileNameStr);
    SHELLEXECUTEINFOW info =
    {
      .cbSize = sizeof(info),
      .fMask = SEE_MASK_NOASYNC,
      .hwnd = NULL,
      .lpVerb = NULL,
      .lpFile = fileNameStr.c_str(),
      .lpParameters = NULL,
      .lpDirectory = NULL,
      .nShow = SW_NORMAL,
    };
    if(!ShellExecuteExW(&info))
      throw Exception{"failed to run or open file: "} << fileName;
#elif defined(COIL_PLATFORM_POSIX)
    RunProcessAndForget("xdg-open", {fileName});
#endif
  }
}
