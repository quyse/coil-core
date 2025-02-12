module;

#include <sqlite3.h>
#include <concepts>
#include <optional>
#include <string>

export module coil.core.sqlite;

import coil.core.base;

namespace Coil
{
  template <int... additionalSuccessCodes>
  int CheckError(sqlite3* db, int error)
  {
    if(((error != SQLITE_OK) && ... && (error != additionalSuccessCodes)))
    {
      throw Exception("sqlite error: ") << sqlite3_errmsg(db);
    }
    return error;
  }
}

export namespace Coil
{
  template <typename T>
  struct SqliteValue;

  template <typename T>
  concept IsSqliteValueBindable = requires(sqlite3_stmt* stmt, int index, T value)
  {
    { SqliteValue<T>::Bind(stmt, index, value) } -> std::same_as<void>;
  };
  template <typename T>
  concept IsSqliteValueGettable = requires(sqlite3_stmt* stmt, int index)
  {
    { SqliteValue<T>::Get(stmt, index) } -> std::same_as<T>;
  };

  template <>
  struct SqliteValue<int32_t>
  {
    static void Bind(sqlite3_stmt* stmt, int index, int32_t value)
    {
      sqlite3_bind_int64(stmt, index, value);
    }
    static int32_t Get(sqlite3_stmt* stmt, int index)
    {
      return (int32_t)sqlite3_column_int64(stmt, index);
    }
  };
  template <>
  struct SqliteValue<uint32_t>
  {
    static void Bind(sqlite3_stmt* stmt, int index, uint32_t value)
    {
      sqlite3_bind_int64(stmt, index, value);
    }
    static uint32_t Get(sqlite3_stmt* stmt, int index)
    {
      return (uint32_t)sqlite3_column_int64(stmt, index);
    }
  };
  template <>
  struct SqliteValue<int64_t>
  {
    static void Bind(sqlite3_stmt* stmt, int index, int64_t value)
    {
      sqlite3_bind_int64(stmt, index, value);
    }
    static int64_t Get(sqlite3_stmt* stmt, int index)
    {
      return (int64_t)sqlite3_column_int64(stmt, index);
    }
  };
  template <>
  struct SqliteValue<bool>
  {
    static void Bind(sqlite3_stmt* stmt, int index, bool value)
    {
      sqlite3_bind_int64(stmt, index, value ? 1 : 0);
    }
    static bool Get(sqlite3_stmt* stmt, int index)
    {
      return sqlite3_column_int64(stmt, index) > 0;
    }
  };
  template <>
  struct SqliteValue<std::string>
  {
    static void Bind(sqlite3_stmt* stmt, int index, std::string const& value)
    {
      sqlite3_bind_text64(stmt, index, value.c_str(), value.length(), SQLITE_TRANSIENT, SQLITE_UTF8);
    }
    static std::string Get(sqlite3_stmt* stmt, int index)
    {
      return { (char const*)sqlite3_column_text(stmt, index), (size_t)sqlite3_column_bytes(stmt, index) };
    }
  };
  // more efficient specializations for static strings
  template <>
  struct SqliteValue<char const*>
  {
    static void Bind(sqlite3_stmt* stmt, int index, char const* value)
    {
      Bind(stmt, index, value, strlen(value));
    }
    static void Bind(sqlite3_stmt* stmt, int index, char const* value, size_t length)
    {
      sqlite3_bind_text64(stmt, index, value, length, SQLITE_STATIC, SQLITE_UTF8);
    }
  };
  template <size_t N>
  struct SqliteValue<char const(&)[N]>
  {
    static void Bind(sqlite3_stmt* stmt, int index, char const (&value)[N])
    {
      SqliteValue<char const*>::Bind(stmt, index, value, sizeof(value) - 1);
    }
  };
  template <>
  struct SqliteValue<Buffer>
  {
    static void Bind(sqlite3_stmt* stmt, int index, Buffer const& value)
    {
      sqlite3_bind_blob64(stmt, index, value.data, value.size, SQLITE_TRANSIENT);
    }
    static Buffer Get(sqlite3_stmt* stmt, int index)
    {
      return { sqlite3_column_blob(stmt, index), (size_t)sqlite3_column_bytes(stmt, index) };
    }
  };
  template <typename T>
  struct SqliteValue<std::optional<T>>
  {
    static void Bind(sqlite3_stmt* stmt, int index, std::optional<T> const& value) requires IsSqliteValueBindable<T>
    {
      if(value.has_value())
      {
        SqliteValue<T>::Bind(stmt, index, value.value());
      }
      else
      {
        sqlite3_bind_null(stmt, index);
      }
    }
    static std::optional<T> Get(sqlite3_stmt* stmt, int index) requires IsSqliteValueGettable<T>
    {
      if(sqlite3_column_type(stmt, index) != SQLITE_NULL)
      {
        return { SqliteValue<T>::Get(stmt, index) };
      }
      else
      {
        return {};
      }
    }
  };

  class SqliteDb
  {
  public:
    class Result
    {
    public:
      Result(sqlite3_stmt* stmt)
      : _stmt(stmt) {}

      template <IsSqliteValueGettable T>
      T Get(int index) const
      {
        return SqliteValue<T>::Get(_stmt, index);
      }

    private:
      sqlite3_stmt* _stmt;
    };

    class Query
    {
    public:
      Query(sqlite3* db, sqlite3_stmt* stmt)
      : _db(db), _stmt(stmt) {}
      Query(Query const&) = delete;
      Query(Query&&) = delete;
      ~Query()
      {
        sqlite3_reset(_stmt);
        sqlite3_clear_bindings(_stmt);
      }

      // perform operation not returning result (expects SQLITE_DONE)
      void operator()()
      {
        while(Next().has_value());
      }

      // perform operation returning result
      std::optional<Result> Next()
      {
        if(CheckError<SQLITE_ROW, SQLITE_DONE>(_db, sqlite3_step(_stmt)) == SQLITE_ROW)
        {
          return Result(_stmt);
        }
        return {};
      }

    private:
      sqlite3* _db = nullptr;
      sqlite3_stmt* _stmt = nullptr;
    };

    class Statement
    {
    public:
      Statement(sqlite3* db, sqlite3_stmt* stmt)
      : _db(db), _stmt(stmt) {}
      Statement(Statement const&) = delete;
      Statement(Statement&& stmt) noexcept
      {
        std::swap(_stmt, stmt._stmt);
      }
      ~Statement()
      {
        // null is ok
        sqlite3_finalize(_stmt);
      }

      template <typename... Args>
      requires (IsSqliteValueBindable<std::decay_t<Args>> && ... && true)
      Query operator()(Args&&... args)
      {
        ([&]<int... indices>(std::integer_sequence<int, indices...> seq)
        {
          (SqliteValue<std::decay_t<Args>>::Bind(_stmt, indices + 1, std::forward<Args>(args)) , ...);
        })(std::make_integer_sequence<int, sizeof...(Args)>());
        return {_db, _stmt};
      }

    private:
      sqlite3* _db = nullptr;
      sqlite3_stmt* _stmt = nullptr;
    };

    class Transaction
    {
    public:
      Transaction(SqliteDb& db)
      : _db(db)
      {
        _db._stmtSavepoint()();
      }
      Transaction(Transaction const&) = delete;
      Transaction(Transaction&&) = delete;
      ~Transaction()
      {
        if(!_finished)
        {
          Rollback();
        }
      }

      void Commit()
      {
        if(_finished)
        {
          throw Exception("sqlite transaction already finished");
        }
        _db._stmtRelease()();
        _finished = true;
      }

      void Rollback()
      {
        if(_finished)
        {
          throw Exception("sqlite transaction already finished");
        }
        _db._stmtRollback()();
        _db._stmtRelease()();
        _finished = true;
      }

    private:
      SqliteDb& _db;
      bool _finished = false;
    };

    using OpenFlags = uint32_t;
    enum class OpenFlag : OpenFlags
    {
      Write = 1,
      Create = 2,
    };

    class DbHandle
    {
    public:
      DbHandle() = default;
      DbHandle(sqlite3* db)
      : _db(db) {}
      DbHandle(DbHandle const&) = delete;
      DbHandle(DbHandle&& db) noexcept
      {
        std::swap(_db, db._db);
      }
      ~DbHandle()
      {
        // null is ok
        sqlite3_close_v2(_db);
      }

      operator sqlite3*() const
      {
        return _db;
      }

    private:
      sqlite3* _db = nullptr;
    };

  private:
    SqliteDb(DbHandle&& db)
    : _db(std::move(db))
    , _stmtSavepoint(CreateStatement("SAVEPOINT T"))
    , _stmtRelease(CreateStatement("RELEASE T"))
    , _stmtRollback(CreateStatement("ROLLBACK TO T"))
    {}
  public:
    SqliteDb(SqliteDb const&) = delete;

    static SqliteDb Open(char const* fileName, OpenFlags flags = (OpenFlags)OpenFlag::Write | (OpenFlags)OpenFlag::Create)
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

    Statement CreateStatement(char const* sql)
    {
      sqlite3_stmt* stmt;
      CheckError(_db, sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr));
      return {_db, stmt};
    }

    Transaction CreateTransaction()
    {
      return *this;
    }

    void Exec(char const* sql)
    {
      CheckError(_db, sqlite3_exec(_db, sql, nullptr, nullptr, nullptr));
    }

  private:
    DbHandle _db;
    Statement _stmtSavepoint, _stmtRelease, _stmtRollback;
  };
}
