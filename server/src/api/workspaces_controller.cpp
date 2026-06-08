#include "api/workspaces_controller.h"

#include "auth/auth_service.h"

#include <drogon/orm/DbClient.h>

#include <json/json.h>
#include <json/writer.h>

#include <mutex>
#include <random>
#include <sstream>
#include <string>

namespace toide::server::api {
namespace {

std::mutex g_schemaMutex;

std::string jsonString(const Json::Value &v)
{
    Json::StreamWriterBuilder b;
    b["indentation"] = "";
    return Json::writeString(b, v);
}

drogon::HttpResponsePtr jsonHttp(const Json::Value &payload, drogon::HttpStatusCode code = drogon::k200OK)
{
    auto r = drogon::HttpResponse::newHttpResponse();
    r->setStatusCode(code);
    r->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    r->setBody(jsonString(payload));
    return r;
}

std::optional<std::string> bearerToken(const drogon::HttpRequestPtr &req)
{
    if (!req) {
        return std::nullopt;
    }
    const std::string &auth = req->getHeader("authorization");
    if (auth.size() > 7 && auth.compare(0, 7, "Bearer ") == 0) {
        return auth.substr(7);
    }
    return std::nullopt;
}

bool requireAuth(const drogon::HttpRequestPtr &req, auth::AuthenticatedUser &outUser, Json::Value &err)
{
    auto tok = bearerToken(req);
    if (!tok.has_value()) {
        err["error"]["code"] = "UNAUTHORIZED";
        err["error"]["message"] = "Missing or invalid Authorization header.";
        return false;
    }
    const auto u = auth::AuthService::instance().userFromToken(*tok);
    if (!u.has_value()) {
        err["error"]["code"] = "UNAUTHORIZED";
        err["error"]["message"] = "Invalid or expired token.";
        return false;
    }
    outUser = *u;
    return true;
}

void ensureWorkspacesTable(const drogon::orm::DbClientPtr &db)
{
    if (!db) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_schemaMutex);
    static bool done = false;
    if (done) {
        return;
    }
    db->execSqlSync(
        "CREATE TABLE IF NOT EXISTS workspaces ("
        "id CHAR(36) PRIMARY KEY,"
        "name VARCHAR(128) NOT NULL,"
        "created_by CHAR(36) NOT NULL,"
        "created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),"
        "KEY idx_workspaces_created_by (created_by),"
        "KEY idx_workspaces_created_at (created_at)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    done = true;
}

std::string randomHexChunk()
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<unsigned long long> dist;
    std::ostringstream os;
    os << std::hex << dist(rng) << dist(rng);
    return os.str();
}

std::string makeWorkspaceId()
{
    std::string h = randomHexChunk();
    while (h.size() < 32) {
        h.append(randomHexChunk());
    }
    h.resize(32);
    return h.substr(0, 8) + "-" + h.substr(8, 4) + "-" + h.substr(12, 4) + "-" + h.substr(16, 4) + "-" + h.substr(20, 12);
}

} // namespace

void WorkspacesController::listWorkspaces(const drogon::HttpRequestPtr &req,
                                          std::function<void(const drogon::HttpResponsePtr &)> &&callback) const
try {
    Json::Value err(Json::objectValue);
    auth::AuthenticatedUser user;
    if (!requireAuth(req, user, err)) {
        callback(jsonHttp(err, drogon::k401Unauthorized));
        return;
    }

    auto db = drogon::app().getDbClient("default");
    if (!db) {
        err["error"]["code"] = "INTERNAL_ERROR";
        err["error"]["message"] = "Database not configured.";
        callback(jsonHttp(err, drogon::k503ServiceUnavailable));
        return;
    }
    ensureWorkspacesTable(db);

    const auto r = db->execSqlSync(
        "SELECT w.id, w.name, w.created_by, w.created_at, u.username AS creator_username "
        "FROM workspaces w "
        "LEFT JOIN users u ON u.id = w.created_by "
        "ORDER BY w.created_at DESC");

    Json::Value list(Json::arrayValue);
    for (const auto &row : r) {
        Json::Value item(Json::objectValue);
        item["id"] = row["id"].as<std::string>();
        item["name"] = row["name"].as<std::string>();
        item["createdBy"] = row["created_by"].as<std::string>();
        item["createdByUsername"] = row["creator_username"].isNull() ? "" : row["creator_username"].as<std::string>();
        item["createdAt"] = row["created_at"].as<std::string>();
        list.append(item);
    }

    Json::Value ok(Json::objectValue);
    ok["workspaces"] = list;
    callback(jsonHttp(ok));
} catch (const std::exception &e) {
    Json::Value err(Json::objectValue);
    err["error"]["code"] = "INTERNAL_ERROR";
    err["error"]["message"] = e.what();
    callback(jsonHttp(err, drogon::k500InternalServerError));
}

void WorkspacesController::createWorkspace(const drogon::HttpRequestPtr &req,
                                           std::function<void(const drogon::HttpResponsePtr &)> &&callback) const
try {
    Json::Value err(Json::objectValue);
    auth::AuthenticatedUser user;
    if (!requireAuth(req, user, err)) {
        callback(jsonHttp(err, drogon::k401Unauthorized));
        return;
    }

    auto db = drogon::app().getDbClient("default");
    if (!db) {
        err["error"]["code"] = "INTERNAL_ERROR";
        err["error"]["message"] = "Database not configured.";
        callback(jsonHttp(err, drogon::k503ServiceUnavailable));
        return;
    }
    ensureWorkspacesTable(db);

    Json::Value body;
    Json::CharReaderBuilder cb;
    std::string errs;
    const std::string raw = std::string(req->getBody());
    std::unique_ptr<Json::CharReader> reader(cb.newCharReader());
    if (!reader->parse(raw.data(), raw.data() + raw.size(), &body, &errs)) {
        err["error"]["code"] = "INVALID_REQUEST";
        err["error"]["message"] = "Invalid JSON body.";
        callback(jsonHttp(err, drogon::k400BadRequest));
        return;
    }

    std::string name = body.get("name", "").asString();
    while (!name.empty() && (name.front() == ' ' || name.front() == '\t')) {
        name.erase(name.begin());
    }
    while (!name.empty() && (name.back() == ' ' || name.back() == '\t')) {
        name.pop_back();
    }
    if (name.empty() || name.size() > 128) {
        err["error"]["code"] = "INVALID_REQUEST";
        err["error"]["message"] = "name is required and must be 1–128 characters.";
        callback(jsonHttp(err, drogon::k400BadRequest));
        return;
    }

    const std::string id = makeWorkspaceId();
    db->execSqlSync("INSERT INTO workspaces (id, name, created_by) VALUES (?,?,?)", id, name, user.id);

    Json::Value ok(Json::objectValue);
    ok["id"] = id;
    ok["name"] = name;
    ok["createdBy"] = user.id;
    ok["createdByUsername"] = user.username;
    callback(jsonHttp(ok, drogon::k201Created));
} catch (const std::exception &e) {
    Json::Value err(Json::objectValue);
    err["error"]["code"] = "INTERNAL_ERROR";
    err["error"]["message"] = e.what();
    callback(jsonHttp(err, drogon::k500InternalServerError));
}

} // namespace toide::server::api
