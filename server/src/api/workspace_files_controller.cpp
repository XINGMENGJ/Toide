#include "api/workspace_files_controller.h"

#include "auth/auth_service.h"

#include <drogon/orm/DbClient.h>

#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>

#include <mutex>
#include <optional>
#include <string>

namespace toide::server::api {
namespace {

std::mutex g_wsSchemaMutex;

void ensureWorkspaceFilesTable(const drogon::orm::DbClientPtr &db)
{
    if (!db) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_wsSchemaMutex);
    static bool done = false;
    if (done) {
        return;
    }
    db->execSqlSync(
        "CREATE TABLE IF NOT EXISTS workspace_file_versions ("
        "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
        "project_key VARCHAR(128) NOT NULL,"
        "file_path VARCHAR(512) NOT NULL,"
        "version BIGINT NOT NULL,"
        "content MEDIUMTEXT NOT NULL,"
        "updated_by CHAR(36) NOT NULL,"
        "updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),"
        "UNIQUE KEY uq_workspace_file_ver (project_key, file_path, version),"
        "KEY idx_workspace_lookup (project_key, file_path)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    done = true;
}

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

} // namespace

void WorkspaceFilesController::getVersion(const drogon::HttpRequestPtr &req,
                                          std::function<void(const drogon::HttpResponsePtr &)> &&callback) const
{
    Json::Value err(Json::objectValue);
    auth::AuthenticatedUser user;
    if (!requireAuth(req, user, err)) {
        callback(jsonHttp(err, drogon::k401Unauthorized));
        return;
    }

    try {
        auto db = drogon::app().getDbClient("default");
        if (!db) {
            err["error"]["code"] = "INTERNAL_ERROR";
            err["error"]["message"] = "Database not configured.";
            callback(jsonHttp(err, drogon::k503ServiceUnavailable));
            return;
        }
        ensureWorkspaceFilesTable(db);

        const std::string projectKey = req->getParameter("projectKey");
        const std::string filePath = req->getParameter("path");
        if (projectKey.empty() || filePath.empty()) {
            err["error"]["code"] = "INVALID_REQUEST";
            err["error"]["message"] = "projectKey and path are required.";
            callback(jsonHttp(err, drogon::k400BadRequest));
            return;
        }
        if (filePath.size() > 512) {
            err["error"]["code"] = "INVALID_REQUEST";
            err["error"]["message"] = "path is too long.";
            callback(jsonHttp(err, drogon::k400BadRequest));
            return;
        }

        const auto r = db->execSqlSync(
            "SELECT COALESCE(MAX(version), 0) AS v FROM workspace_file_versions WHERE project_key=? AND file_path=?",
            projectKey,
            filePath);

        Json::Value ok(Json::objectValue);
        ok["projectKey"] = projectKey;
        ok["filePath"] = filePath;
        ok["version"] = static_cast<Json::Int64>(r.size() > 0 ? r[0]["v"].as<int64_t>() : 0);
        callback(jsonHttp(ok));
    } catch (const std::exception &e) {
        err["error"]["code"] = "INTERNAL_ERROR";
        err["error"]["message"] = e.what();
        callback(jsonHttp(err, drogon::k500InternalServerError));
    }
}

void WorkspaceFilesController::getManifest(const drogon::HttpRequestPtr &req,
                                           std::function<void(const drogon::HttpResponsePtr &)> &&callback) const
{
    Json::Value err(Json::objectValue);
    auth::AuthenticatedUser user;
    if (!requireAuth(req, user, err)) {
        callback(jsonHttp(err, drogon::k401Unauthorized));
        return;
    }

    try {
        auto db = drogon::app().getDbClient("default");
        if (!db) {
            err["error"]["code"] = "INTERNAL_ERROR";
            err["error"]["message"] = "Database not configured.";
            callback(jsonHttp(err, drogon::k503ServiceUnavailable));
            return;
        }
        ensureWorkspaceFilesTable(db);

        const std::string projectKey = req->getParameter("projectKey");
        if (projectKey.empty()) {
            err["error"]["code"] = "INVALID_REQUEST";
            err["error"]["message"] = "projectKey is required.";
            callback(jsonHttp(err, drogon::k400BadRequest));
            return;
        }

        const auto r = db->execSqlSync(
            "SELECT file_path, MAX(version) AS v FROM workspace_file_versions WHERE project_key=? GROUP BY file_path "
            "ORDER BY file_path",
            projectKey);

        Json::Value files(Json::arrayValue);
        for (const auto &row : r) {
            Json::Value item(Json::objectValue);
            item["path"] = row["file_path"].as<std::string>();
            item["version"] = static_cast<Json::Int64>(row["v"].as<int64_t>());
            files.append(item);
        }

        Json::Value ok(Json::objectValue);
        ok["projectKey"] = projectKey;
        ok["files"] = files;
        callback(jsonHttp(ok));
    } catch (const std::exception &e) {
        err["error"]["code"] = "INTERNAL_ERROR";
        err["error"]["message"] = e.what();
        callback(jsonHttp(err, drogon::k500InternalServerError));
    }
}

void WorkspaceFilesController::getLatest(const drogon::HttpRequestPtr &req,
                                         std::function<void(const drogon::HttpResponsePtr &)> &&callback) const
{
    Json::Value err(Json::objectValue);
    auth::AuthenticatedUser user;
    if (!requireAuth(req, user, err)) {
        callback(jsonHttp(err, drogon::k401Unauthorized));
        return;
    }

    try {
        auto db = drogon::app().getDbClient("default");
        if (!db) {
            err["error"]["code"] = "INTERNAL_ERROR";
            err["error"]["message"] = "Database not configured.";
            callback(jsonHttp(err, drogon::k503ServiceUnavailable));
            return;
        }
        ensureWorkspaceFilesTable(db);

        const std::string projectKey = req->getParameter("projectKey");
        const std::string filePath = req->getParameter("path");
        if (projectKey.empty() || filePath.empty()) {
            err["error"]["code"] = "INVALID_REQUEST";
            err["error"]["message"] = "projectKey and path are required.";
            callback(jsonHttp(err, drogon::k400BadRequest));
            return;
        }
        if (filePath.size() > 512) {
            err["error"]["code"] = "INVALID_REQUEST";
            err["error"]["message"] = "path is too long.";
            callback(jsonHttp(err, drogon::k400BadRequest));
            return;
        }

        const auto r = db->execSqlSync(
            "SELECT file_path, version, content FROM workspace_file_versions WHERE project_key=? AND file_path=? ORDER "
            "BY version DESC LIMIT 1",
            projectKey,
            filePath);

        if (r.empty()) {
            err["error"]["code"] = "NOT_FOUND";
            err["error"]["message"] = "No content for this file.";
            callback(jsonHttp(err, drogon::k404NotFound));
            return;
        }

        Json::Value ok(Json::objectValue);
        ok["projectKey"] = projectKey;
        ok["filePath"] = r[0]["file_path"].as<std::string>();
        ok["version"] = static_cast<Json::Int64>(r[0]["version"].as<int64_t>());
        ok["content"] = r[0]["content"].as<std::string>();
        callback(jsonHttp(ok));
    } catch (const std::exception &e) {
        err["error"]["code"] = "INTERNAL_ERROR";
        err["error"]["message"] = e.what();
        callback(jsonHttp(err, drogon::k500InternalServerError));
    }
}

void WorkspaceFilesController::putContent(const drogon::HttpRequestPtr &req,
                                          std::function<void(const drogon::HttpResponsePtr &)> &&callback) const
{
    Json::Value err(Json::objectValue);
    auth::AuthenticatedUser user;
    if (!requireAuth(req, user, err)) {
        callback(jsonHttp(err, drogon::k401Unauthorized));
        return;
    }

    try {
        auto db = drogon::app().getDbClient("default");
        if (!db) {
            err["error"]["code"] = "INTERNAL_ERROR";
            err["error"]["message"] = "Database not configured.";
            callback(jsonHttp(err, drogon::k503ServiceUnavailable));
            return;
        }
        ensureWorkspaceFilesTable(db);

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

        const std::string projectKey = body.get("projectKey", "").asString();
        const std::string filePath = body.get("filePath", "").asString();
        const auto baseVersion = body.get("baseVersion", Json::Int64(0)).asInt64();
        const std::string content = body.get("content", "").asString();
        if (projectKey.empty() || filePath.empty()) {
            err["error"]["code"] = "INVALID_REQUEST";
            err["error"]["message"] = "projectKey and filePath are required.";
            callback(jsonHttp(err, drogon::k400BadRequest));
            return;
        }
        if (filePath.size() > 512) {
            err["error"]["code"] = "INVALID_REQUEST";
            err["error"]["message"] = "filePath is too long.";
            callback(jsonHttp(err, drogon::k400BadRequest));
            return;
        }

        const auto rmax = db->execSqlSync(
            "SELECT COALESCE(MAX(version), 0) AS v FROM workspace_file_versions WHERE project_key=? AND file_path=?",
            projectKey,
            filePath);
        const int64_t maxV = rmax.size() > 0 ? rmax[0]["v"].as<int64_t>() : 0;
        if (maxV != baseVersion) {
            Json::Value conflict(Json::objectValue);
            conflict["error"]["code"] = "FILE_VERSION_CONFLICT";
            conflict["error"]["message"] = "File was modified on the server.";
            conflict["error"]["latestVersion"] = static_cast<Json::Int64>(maxV);
            conflict["error"]["baseVersion"] = static_cast<Json::Int64>(baseVersion);
            callback(jsonHttp(conflict, drogon::k409Conflict));
            return;
        }

        const int64_t nextVer = maxV + 1;
        db->execSqlSync(
            "INSERT INTO workspace_file_versions (project_key, file_path, version, content, updated_by) VALUES (?,?,?,?,?)",
            projectKey,
            filePath,
            nextVer,
            content,
            user.id);

        Json::Value ok(Json::objectValue);
        ok["status"] = "saved";
        ok["filePath"] = filePath;
        ok["version"] = static_cast<Json::Int64>(nextVer);
        callback(jsonHttp(ok));
    } catch (const std::exception &e) {
        err["error"]["code"] = "INTERNAL_ERROR";
        err["error"]["message"] = e.what();
        callback(jsonHttp(err, drogon::k500InternalServerError));
    }
}

} // namespace toide::server::api
