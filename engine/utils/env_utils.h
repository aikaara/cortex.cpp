#pragma once

#include <string>
#include <optional>

namespace cortex::utils {

class EnvUtils {
 public:
  static std::optional<std::string> GetEnv(const std::string& key);
  static std::string GetPostgresConnectionString();
};

}  // namespace cortex::utils 