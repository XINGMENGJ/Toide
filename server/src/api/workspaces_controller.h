#pragma once

#include <drogon/HttpController.h>

namespace toide::server::api {

class WorkspacesController final : public drogon::HttpController<WorkspacesController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(WorkspacesController::listWorkspaces, "/api/workspaces", drogon::Get);
    ADD_METHOD_TO(WorkspacesController::createWorkspace, "/api/workspaces", drogon::Post);
    METHOD_LIST_END

    void listWorkspaces(const drogon::HttpRequestPtr &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;

    void createWorkspace(const drogon::HttpRequestPtr &req,
                         std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;
};

} // namespace toide::server::api
