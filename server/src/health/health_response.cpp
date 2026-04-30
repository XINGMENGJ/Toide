#include "health/health_response.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace toide::server {
namespace {

std::string utcTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);

    std::tm utcTime{};
#if defined(_WIN32)
    gmtime_s(&utcTime, &time);
#else
    gmtime_r(&time, &utcTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

} // namespace

std::string buildHealthResponse()
{
    std::ostringstream stream;
    stream << "{\"service\":\"toide-server\","
           << "\"status\":\"ok\","
           << "\"version\":\"0.1.0\","
           << "\"timestamp\":\"" << utcTimestamp() << "\"}";
    return stream.str();
}

std::string buildDependencyHealthResponse(const DependencyConfig &config)
{
    const auto mysqlConfigured = config.mysqlConfigured ? "true" : "false";
    const auto redisConfigured = config.redisConfigured ? "true" : "false";
    const auto mysqlStatus = config.mysqlConfigured ? "configured" : "not_configured";
    const auto redisStatus = config.redisConfigured ? "configured" : "not_configured";
    const auto overallStatus = config.mysqlConfigured && config.redisConfigured ? "ok" : "degraded";

    std::ostringstream stream;
    stream << "{\"service\":\"toide-server\","
           << "\"status\":\"" << overallStatus << "\","
           << "\"dependencies\":{"
           << "\"mysql\":{\"configured\":" << mysqlConfigured << ",\"status\":\"" << mysqlStatus << "\"},"
           << "\"redis\":{\"configured\":" << redisConfigured << ",\"status\":\"" << redisStatus << "\"}"
           << "}}";
    return stream.str();
}

} // namespace toide::server
