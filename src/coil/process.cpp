#include "process.hpp"
#include "appidentity.hpp"
#include <filesystem>
#include <unistd.h>

namespace
{
  std::string const& GetHomeLocation()
  {
    static std::string const location = []()
    {
      char const* str = std::getenv("HOME");
      return str ? str : ".";
    }();
    return location;
  }

  std::string const& GetConfigLocation()
  {
    static std::string const location = []() -> std::string
    {
      char const* str = std::getenv("XDG_CONFIG_HOME");
      if(str) return str;
      return GetHomeLocation() + "/.config";
    }();
    return location;
  }

  std::string const& GetDataLocation()
  {
    static std::string const location = []() -> std::string
    {
      char const* str = std::getenv("XDG_DATA_HOME");
      if(str) return str;
      return GetHomeLocation() + "/.local/share";
    }();
    return location;
  }

  std::string const& GetStateLocation()
  {
    static std::string const location = []() -> std::string
    {
      char const* str = std::getenv("XDG_STATE_HOME");
      if(str) return str;
      return GetHomeLocation() + "/.local/state";
    }();
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
      return GetConfigLocation() + "/" + AppIdentity::GetInstance().PackageName();
    case AppKnownLocation::Data:
      return GetDataLocation() + "/" + AppIdentity::GetInstance().PackageName();
    case AppKnownLocation::State:
      return GetStateLocation() + "/" + AppIdentity::GetInstance().PackageName();
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
  }
}
