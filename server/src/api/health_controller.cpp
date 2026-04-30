#include "api/health_controller.h"

#include "health/health_response.h"

#include <json/json.h>

namespace toide::server::api {
namespace {

toide::server::DependencyConfig dependencyConfigFromCustomConfig()
{
    toide::server::DependencyConfig config;
    const auto &custom = drogon::app().getCustomConfig();
    if (!custom.isObject()) {
        return config;
    }

    const auto &toide = custom["toide"];
    if (!toide.isObject()) {
        return config;
    }

    if (toide.isMember("mysql") && toide["mysql"].isObject()) {
        const auto &mysql = toide["mysql"];
        if (mysql.isMember("host") && mysql["host"].isString()) {
            config.mysqlConfigured = !mysql["host"].asString().empty();
        }
    }

    if (toide.isMember("redis") && toide["redis"].isObject()) {
        const auto &redis = toide["redis"];
        if (redis.isMember("host") && redis["host"].isString()) {
            config.redisConfigured = !redis["host"].asString().empty();
        }
    }

    return config;
}

drogon::HttpResponsePtr jsonResponse(const std::string &body)
{
    auto response = drogon::HttpResponse::newHttpResponse();
    response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    response->setBody(body);
    return response;
}

} // namespace

void HealthController::health(const drogon::HttpRequestPtr &,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback) const
{
    callback(jsonResponse(buildHealthResponse()));
}

void HealthController::dependencies(const drogon::HttpRequestPtr &,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback) const
{
    callback(jsonResponse(buildDependencyHealthResponse(dependencyConfigFromCustomConfig())));
}

} // namespace toide::server::api
