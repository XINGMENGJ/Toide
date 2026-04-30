#include "api/auth_controller.h"

#include "auth/auth_service.h"

#include <json/json.h>
#include <json/writer.h>

namespace toide::server::api {
namespace {

std::string jsonString(const Json::Value &v)
{
    Json::StreamWriterBuilder b;
    b["indentation"] = "";
    return Json::writeString(b, v);
}

drogon::HttpResponsePtr jsonResponse(const Json::Value &payload, drogon::HttpStatusCode status = drogon::k200OK)
{
    auto response = drogon::HttpResponse::newHttpResponse();
    response->setStatusCode(status);
    response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    response->setBody(jsonString(payload));
    return response;
}

Json::Value authPayload(const auth::AuthResult &result)
{
    Json::Value root;
    root["success"] = result.ok;
    root["message"] = result.message;
    if (result.ok) {
        root["token"] = result.token;
        root["user"]["id"] = result.user.id;
        root["user"]["username"] = result.user.username;
    }
    return root;
}

std::string bearerToken(const drogon::HttpRequestPtr &req)
{
    const auto header = req->getHeader("authorization");
    static const std::string prefix = "Bearer ";
    if (header.compare(0, prefix.size(), prefix) != 0) {
        return {};
    }
    return header.substr(prefix.size());
}

} // namespace

void AuthController::registerUser(const drogon::HttpRequestPtr &req,
                                  std::function<void(const drogon::HttpResponsePtr &)> &&callback) const
{
    const auto body = req->getJsonObject();
    if (!body) {
        Json::Value err;
        err["success"] = false;
        err["message"] = "Expected JSON body.";
        callback(jsonResponse(err, drogon::k400BadRequest));
        return;
    }

    const auto result = auth::AuthService::instance().registerUser((*body)["username"].asString(), (*body)["password"].asString());
    callback(jsonResponse(authPayload(result), result.ok ? drogon::k200OK : drogon::k400BadRequest));
}

void AuthController::login(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback) const
{
    const auto body = req->getJsonObject();
    if (!body) {
        Json::Value err;
        err["success"] = false;
        err["message"] = "Expected JSON body.";
        callback(jsonResponse(err, drogon::k400BadRequest));
        return;
    }

    const auto result = auth::AuthService::instance().login((*body)["username"].asString(), (*body)["password"].asString());
    callback(jsonResponse(authPayload(result), result.ok ? drogon::k200OK : drogon::k401Unauthorized));
}

void AuthController::me(const drogon::HttpRequestPtr &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback) const
{
    const auto user = auth::AuthService::instance().userFromToken(bearerToken(req));
    Json::Value root;
    if (!user.has_value()) {
        root["success"] = false;
        root["message"] = "Not authenticated.";
        callback(jsonResponse(root, drogon::k401Unauthorized));
        return;
    }
    root["success"] = true;
    root["user"]["id"] = user->id;
    root["user"]["username"] = user->username;
    callback(jsonResponse(root));
}

} // namespace toide::server::api
