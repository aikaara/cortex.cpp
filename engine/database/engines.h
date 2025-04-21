#pragma once

#include <string>
#include <optional>
#include <vector>
#include "database.h"

namespace cortex::db {

struct EngineEntry {
  std::string engine_name;
  std::string type;
  std::string api_key;
  std::string url;
  std::string version;
  std::string variant;
  std::string status;
  std::string metadata;
};

class Engines {
 public:
  Engines();
  explicit Engines(SQLite::Database& db);
  ~Engines();

  std::optional<EngineEntry> UpsertEngine(
      const std::string& engine_name, const std::string& type,
      const std::string& api_key, const std::string& url,
      const std::string& version, const std::string& variant,
      const std::string& status, const std::string& metadata);

  std::optional<EngineEntry> GetEngine(const std::string& engine_name,
                                      const std::string& variant);
  std::vector<EngineEntry> GetAllEngines();
  bool DeleteEngine(const std::string& engine_name, const std::string& variant);

 private:
  void CreateTable();
  void CreatePostgreSQLTable();
  
  SQLite::Database& db_;
  PGconn* pg_conn_{nullptr};
};
}  // namespace cortex::db