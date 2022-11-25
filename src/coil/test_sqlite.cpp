#include "entrypoint.hpp"
#include "sqlite.hpp"
#include <iostream>

using namespace Coil;

___COIL_ENTRY_POINT = [](std::vector<std::string>&& args) -> int
{
  SqliteDb db = SqliteDb::Open(":memory:");
  auto stmt = db.CreateStatement("VALUES (?, ?, ?), (?, ?, ?)");
  {
    auto tx = db.CreateTransaction();
    auto query = stmt(uint32_t(123), int64_t(456), "abc", uint32_t(789), int64_t(101112), std::string("def"));
    while(auto result = query.Next())
    {
      auto const& r = result.value();
      std::cout << r.Get<uint32_t>(0) << ' ' << r.Get<uint32_t>(1) << ' ' << r.Get<std::string>(2) << '\n';
    }
    tx.Commit();
  }

  return 0;
};
