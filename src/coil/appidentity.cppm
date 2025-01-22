module;

#include <string>
#include <cstdint>

export module coil.core.appidentity;

export namespace Coil
{
  // global app identity
  class AppIdentity
  {
  public:
    AppIdentity()
    : name_{"App"}, packageName_{"app"} {}

    // display name for app
    std::string& Name()
    {
      return name_;
    }
    // package name for use in directory names, etc
    // suggestion: use reverse-domain notation, i.e. "com.company.app"
    std::string& PackageName()
    {
      return packageName_;
    }
    // app version
    uint32_t& Version()
    {
      return version_;
    }

    static AppIdentity& GetInstance()
    {
      static AppIdentity instance;
      return instance;
    }

  private:
    std::string name_;
    std::string packageName_;
    uint32_t version_ = 0;
  };
}
