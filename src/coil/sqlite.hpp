#pragma once

#include "base.hpp"
#include <concepts>
#include <optional>
#include <utility>
#include <sqlite3.h>

namespace Coil
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
    static void Bind(sqlite3_stmt* stmt, int index, int32_t value);
    static int32_t Get(sqlite3_stmt* stmt, int index);
  };
  template <>
  struct SqliteValue<uint32_t>
  {
    static void Bind(sqlite3_stmt* stmt, int index, uint32_t value);
    static uint32_t Get(sqlite3_stmt* stmt, int index);
  };
  template <>
  struct SqliteValue<int64_t>
  {
    static void Bind(sqlite3_stmt* stmt, int index, int64_t value);
    static int64_t Get(sqlite3_stmt* stmt, int index);
  };
  template <>
  struct SqliteValue<std::string>
  {
    static void Bind(sqlite3_stmt* stmt, int index, std::string const& value);
    static std::string Get(sqlite3_stmt* stmt, int index);
  };
  // more efficient specializations for static strings
  template <>
  struct SqliteValue<char const*>
  {
    static void Bind(sqlite3_stmt* stmt, int index, char const* value);
    static void Bind(sqlite3_stmt* stmt, int index, char const* value, size_t length);
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
    static void Bind(sqlite3_stmt* stmt, int index, Buffer const& value);
    static Buffer Get(sqlite3_stmt* stmt, int index);
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
      Result(sqlite3_stmt* stmt);

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
      Query(sqlite3_stmt* stmt);
      Query(Query const&) = delete;
      Query(Query&&) = delete;
      ~Query();

      // perform operation not returning result (expects SQLITE_DONE)
      void operator()();

      // perform operation returning result
      std::optional<Result> Next();

    private:
      sqlite3_stmt* _stmt;
    };

    class Statement
    {
    public:
      Statement(sqlite3_stmt* stmt);
      Statement(Statement const&) = delete;
      Statement(Statement&& stmt) noexcept;
      ~Statement();

      template <typename... Args>
      requires (IsSqliteValueBindable<std::decay_t<Args>> && ... && true)
      Query operator()(Args&&... args)
      {
        ([&]<int... indices>(std::integer_sequence<int, indices...> seq)
        {
          (SqliteValue<std::decay_t<Args>>::Bind(_stmt, indices + 1, std::forward<Args>(args)) , ...);
        })(std::make_integer_sequence<int, sizeof...(Args)>());
        return _stmt;
      }

    private:
      sqlite3_stmt* _stmt = nullptr;
    };

    class Transaction
    {
    public:
      Transaction(SqliteDb& db);
      Transaction(Transaction const&) = delete;
      Transaction(Transaction&&) = delete;
      ~Transaction();

      void Commit();
      void Rollback();

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
      DbHandle(sqlite3* _db);
      DbHandle(DbHandle const&) = delete;
      DbHandle(DbHandle&& db) noexcept;
      ~DbHandle();

      operator sqlite3*() const;

    private:
      sqlite3* _db = nullptr;
    };

  private:
    SqliteDb(DbHandle&& db);
  public:
    SqliteDb(SqliteDb const&) = delete;

    static SqliteDb Open(char const* fileName, OpenFlags flags = (OpenFlags)OpenFlag::Write | (OpenFlags)OpenFlag::Create);

    Statement CreateStatement(char const* sql);
    Transaction CreateTransaction();
    void Exec(char const* sql);

  private:
    DbHandle _db;
    Statement _stmtSavepoint, _stmtRelease, _stmtRollback;
  };
}
