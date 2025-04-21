#include "env_utils.h"
#include <cstdlib>
#include <stdexcept>

namespace cortex::utils {

std::optional<std::string> EnvUtils::GetEnv(const std::string& key) {
  const char* value = std::getenv(key.c_str());
  if (value == nullptr) {
    return std::nullopt;
  }
  return std::string(value);
}

std::string EnvUtils::GetPostgresConnectionString() {
  // Try to get the full connection string first
  if (auto conn_str = GetEnv("POSTGRES_CONNECTION_STRING")) {
    return *conn_str;
  }

  // If not found, construct from individual components
  std::string conn_str = "postgresql://";
  
  if (auto user = GetEnv("POSTGRES_USER")) {
    conn_str += *user;
    if (auto password = GetEnv("POSTGRES_PASSWORD")) {
      conn_str += ":" + *password;
    }
    conn_str += "@";
  }

  if (auto host = GetEnv("POSTGRES_HOST")) {
    conn_str += *host;
  } else {
    conn_str += "localhost";
  }

  if (auto port = GetEnv("POSTGRES_PORT")) {
    conn_str += ":" + *port;
  } else {
    conn_str += ":5432";
  }

  if (auto dbname = GetEnv("POSTGRES_DB")) {
    conn_str += "/" + *dbname;
  } else {
    conn_str += "/cortex";
  }

  return conn_str;
}

}  // namespace cortex::utils 