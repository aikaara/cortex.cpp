#include "database.h"
#include "utils/env_utils.h"
#include <stdexcept>

namespace cortex::db {

Database::Database() : type_(DatabaseType::SQLite), pg_conn_(nullptr) {
  // Check if PostgreSQL is configured in environment
  if (auto db_type = cortex::utils::EnvUtils::GetEnv("DB_TYPE")) {
    if (*db_type == "postgresql") {
      Initialize(DatabaseType::PostgreSQL, cortex::utils::EnvUtils::GetPostgresConnectionString());
      return;
    }
  }
  // Default to SQLite
  Initialize(DatabaseType::SQLite);
}

Database::~Database() {
  if (pg_conn_) {
    PQfinish(pg_conn_);
  }
}

void Database::Initialize(DatabaseType type, const std::string& connection_string) {
  type_ = type;
  
  if (type == DatabaseType::SQLite) {
    sqlite_db_ = std::make_unique<SQLite::Database>(
        file_manager_utils::GetCortexDataPath() / "cortex.db",
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
  } else if (type == DatabaseType::PostgreSQL) {
    if (connection_string.empty()) {
      throw std::runtime_error("PostgreSQL connection string is required");
    }
    
    pg_conn_ = PQconnectdb(connection_string.c_str());
    if (PQstatus(pg_conn_) != CONNECTION_OK) {
      std::string error = PQerrorMessage(pg_conn_);
      PQfinish(pg_conn_);
      pg_conn_ = nullptr;
      throw std::runtime_error("Failed to connect to PostgreSQL: " + error);
    }
  }
}

}  // namespace cortex::db 