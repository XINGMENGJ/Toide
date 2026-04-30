#pragma once

#include <drogon/WebSocketController.h>

namespace toide::server::api {

class CollaborationWsController final : public drogon::WebSocketController<CollaborationWsController> {
public:
    void handleNewConnection(const drogon::HttpRequestPtr &req,
                             const drogon::WebSocketConnectionPtr &conn) override;

    void handleNewMessage(const drogon::WebSocketConnectionPtr &conn,
                          std::string &&message,
                          const drogon::WebSocketMessageType &type) override;

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &conn) override;

    WS_PATH_LIST_BEGIN
    WS_ADD_PATH_VIA_REGEX("^/ws/projects/([^/]+)$");
    WS_PATH_LIST_END
};

} // namespace toide::server::api
