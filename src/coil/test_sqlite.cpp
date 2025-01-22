#include "entrypoint.hpp"
#include <iostream>

import coil.core.sqlite;

using namespace Coil;

int COIL_ENTRY_POINT(std::vector<std::string> args)
{
  SqliteDb db = SqliteDb::Open(":memory:");
  auto stmt = db.CreateStatement("VALUES (?, ?, ?, ?), (?, ?, ?, ?), (131415, 161718, 'DEF', NULL)");
  {
    auto tx = db.CreateTransaction();
    auto query = stmt(
      uint32_t(123), int64_t(456), "abc", std::optional<std::string>{"ABC"},
      uint32_t(789), int64_t(101112), std::string("def"), std::optional<std::string>{}
    );
    while(auto result = query.Next())
    {
      auto const& r = result.value();
      std::cout << r.Get<uint32_t>(0) << ' ' << r.Get<uint32_t>(1) << ' ' << r.Get<std::string>(2) << ' ';
      auto opt = r.Get<std::optional<std::string>>(3);
      std::cout << (opt.has_value() ? opt.value() : "NULL") << '\n';
    }
    tx.Commit();
  }

  return 0;
}
