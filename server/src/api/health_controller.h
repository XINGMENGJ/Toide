#pragma once

#include <drogon/HttpController.h>

namespace toide::server::api {

class HealthController final : public drogon::HttpController<HealthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthController::health, "/api/health", drogon::Get);
    ADD_METHOD_TO(HealthController::dependencies, "/api/health/dependencies", drogon::Get);
    METHOD_LIST_END

    void health(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;
    void dependencies(const drogon::HttpRequestPtr &request,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;
};

} // namespace toide::server::api
