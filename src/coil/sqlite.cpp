#include "sqlite.hpp"
#include <cstring>

namespace
{
  template <int... additionalSuccessCodes>
  int CheckError(sqlite3* db, int error)
  {
    if(((error != SQLITE_OK) && ... && (error != additionalSuccessCodes)))
    {
      throw Coil::Exception("sqlite error: ") << sqlite3_errmsg(db);
    }
    return error;
  }
}

namespace Coil
{
  void SqliteValue<int32_t>::Bind(sqlite3_stmt* stmt, int index, int32_t value)
  {
    sqlite3_bind_int64(stmt, index, value);
  }
  int32_t SqliteValue<int32_t>::Get(sqlite3_stmt* stmt, int index)
  {
    return (int32_t)sqlite3_column_int64(stmt, index);
  }

  void SqliteValue<uint32_t>::Bind(sqlite3_stmt* stmt, int index, uint32_t value)
  {
    sqlite3_bind_int64(stmt, index, value);
  }
  uint32_t SqliteValue<uint32_t>::Get(sqlite3_stmt* stmt, int index)
  {
    return (uint32_t)sqlite3_column_int64(stmt, index);
  }

  void SqliteValue<int64_t>::Bind(sqlite3_stmt* stmt, int index, int64_t value)
  {
    sqlite3_bind_int64(stmt, index, value);
  }
  int64_t SqliteValue<int64_t>::Get(sqlite3_stmt* stmt, int index)
  {
    return (int64_t)sqlite3_column_int64(stmt, index);
  }

  void SqliteValue<bool>::Bind(sqlite3_stmt* stmt, int index, bool value)
  {
    sqlite3_bind_int64(stmt, index, value ? 1 : 0);
  }
  bool SqliteValue<bool>::Get(sqlite3_stmt* stmt, int index)
  {
    return sqlite3_column_int64(stmt, index) > 0;
  }

  void SqliteValue<std::string>::Bind(sqlite3_stmt* stmt, int index, std::string const& value)
  {
    sqlite3_bind_text64(stmt, index, value.c_str(), value.length(), SQLITE_TRANSIENT, SQLITE_UTF8);
  }
  std::string SqliteValue<std::string>::Get(sqlite3_stmt* stmt, int index)
  {
    return { (char const*)sqlite3_column_text(stmt, index), (size_t)sqlite3_column_bytes(stmt, index) };
  }

  void SqliteValue<char const*>::Bind(sqlite3_stmt* stmt, int index, char const* value)
  {
    Bind(stmt, index, value, strlen(value));
  }
  void SqliteValue<char const*>::Bind(sqlite3_stmt* stmt, int index, char const* value, size_t length)
  {
    sqlite3_bind_text64(stmt, index, value, length, SQLITE_STATIC, SQLITE_UTF8);
  }

  void SqliteValue<Buffer>::Bind(sqlite3_stmt* stmt, int index, Buffer const& value)
  {
    sqlite3_bind_blob64(stmt, index, value.data, value.size, SQLITE_TRANSIENT);
  }
  Buffer SqliteValue<Buffer>::Get(sqlite3_stmt* stmt, int index)
  {
    return { sqlite3_column_blob(stmt, index), (size_t)sqlite3_column_bytes(stmt, index) };
  }

  SqliteDb::Result::Result(sqlite3_stmt* stmt)
  : _stmt(stmt) {}

  SqliteDb::Query::Query(sqlite3* db, sqlite3_stmt* stmt)
  : _db(db), _stmt(stmt) {}

  SqliteDb::Query::~Query()
  {
    sqlite3_reset(_stmt);
    sqlite3_clear_bindings(_stmt);
  }

  void SqliteDb::Query::operator()()
  {
    while(Next().has_value());
  }

  std::optional<SqliteDb::Result> SqliteDb::Query::Next()
  {
    if(CheckError<SQLITE_ROW, SQLITE_DONE>(_db, sqlite3_step(_stmt)) == SQLITE_ROW)
    {
      return Result(_stmt);
    }
    return {};
  }

  SqliteDb::Statement::Statement(sqlite3* db, sqlite3_stmt* stmt)
  : _db(db), _stmt(stmt) {}

  SqliteDb::Statement::Statement(Statement&& stmt) noexcept
  {
    std::swap(_stmt, stmt._stmt);
  }

  SqliteDb::Statement::~Statement()
  {
    // null is ok
    sqlite3_finalize(_stmt);
  }

  SqliteDb::Transaction::Transaction(SqliteDb& db)
  : _db(db)
  {
    _db._stmtSavepoint()();
  }

  SqliteDb::Transaction::~Transaction()
  {
    if(!_finished)
    {
      Rollback();
    }
  }

  void SqliteDb::Transaction::Commit()
  {
    if(_finished)
    {
      throw Exception("sqlite transaction already finished");
    }
    _db._stmtRelease()();
    _finished = true;
  }

  void SqliteDb::Transaction::Rollback()
  {
    if(_finished)
    {
      throw Exception("sqlite transaction already finished");
    }
    _db._stmtRollback()();
    _db._stmtRelease()();
    _finished = true;
  }

  SqliteDb::DbHandle::DbHandle(sqlite3* db)
  : _db(db) {}

  SqliteDb::DbHandle::DbHandle(DbHandle&& db) noexcept
  {
    std::swap(_db, db._db);
  }

  SqliteDb::DbHandle::~DbHandle()
  {
    // null is ok
    sqlite3_close_v2(_db);
  }

  SqliteDb::DbHandle::operator sqlite3*() const
  {
    return _db;
  }

  SqliteDb::SqliteDb(DbHandle&& db)
  : _db(std::move(db))
  , _stmtSavepoint(CreateStatement("SAVEPOINT T"))
  , _stmtRelease(CreateStatement("RELEASE T"))
  , _stmtRollback(CreateStatement("ROLLBACK TO T"))
  {}

  SqliteDb SqliteDb::Open(char const* fileName, OpenFlags flags)
  {
    // calculate flags
    int flagsValue = 0;
    flagsValue |= (flags & (OpenFlags)OpenFlag::Write) ? SQLITE_OPEN_READWRITE : SQLITE_OPEN_READONLY;
    if(flags & (OpenFlags)OpenFlag::Create) flagsValue |= SQLITE_OPEN_CREATE;

    // sqlite3 object always gets created, even if error happens
    // make sure it is wrapped
    sqlite3* dbHandle = nullptr;
    int r = sqlite3_open_v2(fileName, &dbHandle, flagsValue, nullptr);
    DbHandle db(dbHandle);
    CheckError(dbHandle, r);
    return db;
  }

  SqliteDb::Statement SqliteDb::CreateStatement(char const* sql)
  {
    sqlite3_stmt* stmt;
    CheckError(_db, sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr));
    return {_db, stmt};
  }

  SqliteDb::Transaction SqliteDb::CreateTransaction()
  {
    return *this;
  }

  void SqliteDb::Exec(char const* sql)
  {
    CheckError(_db, sqlite3_exec(_db, sql, nullptr, nullptr, nullptr));
  }
}
