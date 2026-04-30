#include "api/health_controller.h"

#include "health/health_response.h"

namespace toide::server::api {
namespace {

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
    callback(jsonResponse(buildDependencyHealthResponse()));
}

} // namespace toide::server::api
