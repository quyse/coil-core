#include "sqlite.hpp"
#include <cstring>

namespace
{
  template <int... additionalSuccessCodes>
  int CheckError(int error)
  {
    if(((error != SQLITE_OK) && ... && (error != additionalSuccessCodes)))
    {
      throw Coil::Exception("sqlite error: ") << sqlite3_errstr(error);
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

  void SqliteValue<std::string>::Bind(sqlite3_stmt* stmt, int index, std::string const& value)
  {
    sqlite3_bind_text64(stmt, index, value.c_str(), value.length(), SQLITE_TRANSIENT, SQLITE_UTF8);
  }
  std::string SqliteValue<std::string>::Get(sqlite3_stmt* stmt, int index)
  {
    return std::string((char const*)sqlite3_column_text(stmt, index), sqlite3_column_bytes(stmt, index));
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
    return Buffer(sqlite3_column_blob(stmt, index), sqlite3_column_bytes(stmt, index));
  }

  SqliteDb::Result::Result(sqlite3_stmt* stmt)
  : _stmt(stmt) {}

  SqliteDb::Query::Query(sqlite3_stmt* stmt)
  : _stmt(stmt) {}

  SqliteDb::Query::~Query()
  {
    sqlite3_reset(_stmt);
    sqlite3_clear_bindings(_stmt);
  }

  void SqliteDb::Query::operator()()
  {
    CheckError<SQLITE_DONE>(sqlite3_step(_stmt));
  }

  std::optional<SqliteDb::Result> SqliteDb::Query::Next()
  {
    if(CheckError<SQLITE_ROW, SQLITE_DONE>(sqlite3_step(_stmt)) == SQLITE_ROW)
    {
      return Result(_stmt);
    }
    return {};
  }

  SqliteDb::Statement::Statement(sqlite3_stmt* stmt)
  : _stmt(stmt) {}

  SqliteDb::Statement::Statement(Statement&& stmt)
  {
    std::swap(_stmt, stmt._stmt);
  }

  SqliteDb::Statement::~Statement()
  {
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

  SqliteDb::SqliteDb(sqlite3* db)
  : _db(db)
  , _stmtSavepoint(CreateStatement("SAVEPOINT T"))
  , _stmtRelease(CreateStatement("RELEASE T"))
  , _stmtRollback(CreateStatement("ROLLBACK TO T"))
  {}

  SqliteDb::SqliteDb(SqliteDb&& db)
  : _stmtSavepoint(std::move(db._stmtSavepoint))
  , _stmtRelease(std::move(db._stmtRelease))
  , _stmtRollback(std::move(db._stmtRollback))
  {
    std::swap(_db, db._db);
  }

  SqliteDb::~SqliteDb()
  {
    if(_db)
    {
      sqlite3_close_v2(_db);
    }
  }

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
    SqliteDb db(dbHandle);
    CheckError(r);
    return std::move(db);
  }

  SqliteDb::Statement SqliteDb::CreateStatement(char const* sql)
  {
    sqlite3_stmt* stmt;
    CheckError(sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr));
    return stmt;
  }

  SqliteDb::Transaction SqliteDb::CreateTransaction()
  {
    return *this;
  }
}
