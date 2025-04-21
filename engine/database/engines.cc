#include "engines.h"
#include <SQLiteCpp/Database.h>
#include "database.h"
#include <libpq-fe.h>

namespace cortex::db {

void Engines::CreateTable() {
  if (Database::GetInstance().GetType() == DatabaseType::SQLite) {
    db_.exec(R"(
      CREATE TABLE IF NOT EXISTS engines (
        engine_name TEXT NOT NULL,
        type TEXT NOT NULL,
        api_key TEXT,
        url TEXT,
        version TEXT,
        variant TEXT NOT NULL,
        status TEXT,
        metadata TEXT,
        PRIMARY KEY (engine_name, variant)
      )
    )");
  }
}

void Engines::CreatePostgreSQLTable() {
  if (Database::GetInstance().GetType() == DatabaseType::PostgreSQL) {
    pg_conn_ = Database::GetInstance().GetPostgreSQL();
    const char* query = R"(
      CREATE TABLE IF NOT EXISTS engines (
        engine_name TEXT NOT NULL,
        type TEXT NOT NULL,
        api_key TEXT,
        url TEXT,
        version TEXT,
        variant TEXT NOT NULL,
        status TEXT,
        metadata TEXT,
        PRIMARY KEY (engine_name, variant)
      )
    )";
    
    PGresult* res = PQexec(pg_conn_, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      PQclear(res);
      throw std::runtime_error("Failed to create PostgreSQL table: " + std::string(PQerrorMessage(pg_conn_)));
    }
    PQclear(res);
  }
}

Engines::Engines() : db_(Database::GetInstance().GetSQLite()) {
  CreateTable();
  CreatePostgreSQLTable();
}

Engines::Engines(SQLite::Database& db) : db_(db) {
  CreateTable();
}

Engines::~Engines() {}

std::optional<EngineEntry> Engines::UpsertEngine(
    const std::string& engine_name, const std::string& type,
    const std::string& api_key, const std::string& url,
    const std::string& version, const std::string& variant,
    const std::string& status, const std::string& metadata) {
  if (Database::GetInstance().GetType() == DatabaseType::SQLite) {
    try {
      SQLite::Statement query(
          db_,
          "INSERT INTO engines (engine_name, type, api_key, url, version, "
          "variant, status, metadata) "
          "VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
          "ON CONFLICT(engine_name, variant) DO UPDATE SET "
          "type = excluded.type, "
          "api_key = excluded.api_key, "
          "url = excluded.url, "
          "version = excluded.version, "
          "status = excluded.status, "
          "metadata = excluded.metadata");
      
      query.bind(1, engine_name);
      query.bind(2, type);
      query.bind(3, api_key);
      query.bind(4, url);
      query.bind(5, version);
      query.bind(6, variant);
      query.bind(7, status);
      query.bind(8, metadata);
      
      query.exec();
      
      return GetEngine(engine_name, variant);
    } catch (const std::exception& e) {
      return std::nullopt;
    }
  } else {
    // PostgreSQL implementation
    const char* query = "INSERT INTO engines (engine_name, type, api_key, url, version, variant, status, metadata) "
                       "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) "
                       "ON CONFLICT (engine_name, variant) DO UPDATE SET "
                       "type = EXCLUDED.type, "
                       "api_key = EXCLUDED.api_key, "
                       "url = EXCLUDED.url, "
                       "version = EXCLUDED.version, "
                       "status = EXCLUDED.status, "
                       "metadata = EXCLUDED.metadata";
    
    const char* values[8] = {
      engine_name.c_str(),
      type.c_str(),
      api_key.c_str(),
      url.c_str(),
      version.c_str(),
      variant.c_str(),
      status.c_str(),
      metadata.c_str()
    };
    
    PGresult* res = PQexecParams(pg_conn_, query, 8, nullptr, values, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      PQclear(res);
      return std::nullopt;
    }
    PQclear(res);
    
    return GetEngine(engine_name, variant);
  }
}

std::optional<EngineEntry> Engines::GetEngine(const std::string& engine_name,
                                            const std::string& variant) {
  if (Database::GetInstance().GetType() == DatabaseType::SQLite) {
    try {
      SQLite::Statement query(db_,
                            "SELECT * FROM engines WHERE engine_name = ? AND variant = ?");
      query.bind(1, engine_name);
      query.bind(2, variant);
      
      if (query.executeStep()) {
        EngineEntry entry;
        entry.engine_name = query.getColumn(0).getString();
        entry.type = query.getColumn(1).getString();
        entry.api_key = query.getColumn(2).getString();
        entry.url = query.getColumn(3).getString();
        entry.version = query.getColumn(4).getString();
        entry.variant = query.getColumn(5).getString();
        entry.status = query.getColumn(6).getString();
        entry.metadata = query.getColumn(7).getString();
        return entry;
      }
    } catch (const std::exception& e) {
      return std::nullopt;
    }
  } else {
    // PostgreSQL implementation
    const char* query = "SELECT * FROM engines WHERE engine_name = $1 AND variant = $2";
    const char* values[2] = {engine_name.c_str(), variant.c_str()};
    
    PGresult* res = PQexecParams(pg_conn_, query, 2, nullptr, values, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      PQclear(res);
      return std::nullopt;
    }
    
    if (PQntuples(res) > 0) {
      EngineEntry entry;
      entry.engine_name = PQgetvalue(res, 0, 0);
      entry.type = PQgetvalue(res, 0, 1);
      entry.api_key = PQgetvalue(res, 0, 2);
      entry.url = PQgetvalue(res, 0, 3);
      entry.version = PQgetvalue(res, 0, 4);
      entry.variant = PQgetvalue(res, 0, 5);
      entry.status = PQgetvalue(res, 0, 6);
      entry.metadata = PQgetvalue(res, 0, 7);
      PQclear(res);
      return entry;
    }
    PQclear(res);
  }
  return std::nullopt;
}

std::vector<EngineEntry> Engines::GetAllEngines() {
  std::vector<EngineEntry> entries;
  
  if (Database::GetInstance().GetType() == DatabaseType::SQLite) {
    try {
      SQLite::Statement query(db_, "SELECT * FROM engines");
      while (query.executeStep()) {
        EngineEntry entry;
        entry.engine_name = query.getColumn(0).getString();
        entry.type = query.getColumn(1).getString();
        entry.api_key = query.getColumn(2).getString();
        entry.url = query.getColumn(3).getString();
        entry.version = query.getColumn(4).getString();
        entry.variant = query.getColumn(5).getString();
        entry.status = query.getColumn(6).getString();
        entry.metadata = query.getColumn(7).getString();
        entries.push_back(entry);
      }
    } catch (const std::exception& e) {
      return entries;
    }
  } else {
    // PostgreSQL implementation
    PGresult* res = PQexec(pg_conn_, "SELECT * FROM engines");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      PQclear(res);
      return entries;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
      EngineEntry entry;
      entry.engine_name = PQgetvalue(res, i, 0);
      entry.type = PQgetvalue(res, i, 1);
      entry.api_key = PQgetvalue(res, i, 2);
      entry.url = PQgetvalue(res, i, 3);
      entry.version = PQgetvalue(res, i, 4);
      entry.variant = PQgetvalue(res, i, 5);
      entry.status = PQgetvalue(res, i, 6);
      entry.metadata = PQgetvalue(res, i, 7);
      entries.push_back(entry);
    }
    PQclear(res);
  }
  
  return entries;
}

bool Engines::DeleteEngine(const std::string& engine_name, const std::string& variant) {
  if (Database::GetInstance().GetType() == DatabaseType::SQLite) {
    try {
      SQLite::Statement query(db_,
                            "DELETE FROM engines WHERE engine_name = ? AND variant = ?");
      query.bind(1, engine_name);
      query.bind(2, variant);
      return query.exec() > 0;
    } catch (const std::exception& e) {
      return false;
    }
  } else {
    // PostgreSQL implementation
    const char* query = "DELETE FROM engines WHERE engine_name = $1 AND variant = $2";
    const char* values[2] = {engine_name.c_str(), variant.c_str()};
    
    PGresult* res = PQexecParams(pg_conn_, query, 2, nullptr, values, nullptr, nullptr, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK && PQcmdTuples(res)[0] != '0');
    PQclear(res);
    return success;
  }
}

}  // namespace cortex::db