#pragma once

#include <string>

namespace toide::server {

struct DependencyConfig {
    bool mysqlConfigured = false;
    bool redisConfigured = false;
};

std::string buildHealthResponse();
std::string buildDependencyHealthResponse(const DependencyConfig &config = {});

} // namespace toide::server
