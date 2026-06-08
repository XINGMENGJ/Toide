#pragma once

#include <drogon/HttpController.h>

namespace toide::server::api {

class WorkspaceFilesController final : public drogon::HttpController<WorkspaceFilesController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(WorkspaceFilesController::getVersion, "/api/workspace/files/version", drogon::Get);
    ADD_METHOD_TO(WorkspaceFilesController::getManifest, "/api/workspace/files/manifest", drogon::Get);
    ADD_METHOD_TO(WorkspaceFilesController::getLatest, "/api/workspace/files/latest", drogon::Get);
    ADD_METHOD_TO(WorkspaceFilesController::putContent, "/api/workspace/files/content", drogon::Put);
    METHOD_LIST_END

    void getVersion(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;

    void getManifest(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;

    void getLatest(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;

    void putContent(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;
};

} // namespace toide::server::api
