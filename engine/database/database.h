#pragma once

#include <string>
#include <memory>
#include "SQLiteCpp/SQLiteCpp.h"
#include "utils/file_manager_utils.h"
#include <libpq-fe.h>

namespace cortex::db {

enum class DatabaseType {
  SQLite,
  PostgreSQL
};

class Database {
 public:
  Database(Database const&) = delete;
  Database& operator=(Database const&) = delete;
  ~Database();

  static Database& GetInstance() {
    static Database db;
    return db;
  }

  // Initialize database with configuration
  void Initialize(DatabaseType type, const std::string& connection_string = "");

  // Get database connection based on type
  SQLite::Database& GetSQLite() { return *sqlite_db_; }
  PGconn* GetPostgreSQL() { return pg_conn_; }

  DatabaseType GetType() const { return type_; }

 private:
  Database();
  
  DatabaseType type_;
  std::unique_ptr<SQLite::Database> sqlite_db_;
  PGconn* pg_conn_;
};
}  // namespace cortex::db
