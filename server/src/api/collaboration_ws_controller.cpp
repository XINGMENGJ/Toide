#include "api/collaboration_ws_controller.h"

#include <json/json.h>
#include <json/writer.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace toide::server::api {
namespace {

struct WsCollabContext {
    std::string projectId;
};

std::mutex g_roomMutex;
std::unordered_map<std::string, std::vector<drogon::WebSocketConnectionPtr>> g_rooms;

std::string projectIdFromRequestPath(const drogon::HttpRequestPtr &req)
{
    if (!req) {
        return {};
    }
    std::string path = req->path();
    const auto q = path.find('?');
    if (q != std::string::npos) {
        path = path.substr(0, q);
    }
    static const std::string prefix = "/ws/projects/";
    if (path.compare(0, prefix.size(), prefix) != 0) {
        return {};
    }
    return path.substr(prefix.size());
}

void addToRoom(const std::string &projectId, const drogon::WebSocketConnectionPtr &conn)
{
    if (projectId.empty() || !conn) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_roomMutex);
    g_rooms[projectId].push_back(conn);
}

void removeFromRoom(const std::string &projectId, const drogon::WebSocketConnectionPtr &conn)
{
    if (projectId.empty() || !conn) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_roomMutex);
    const auto mapIt = g_rooms.find(projectId);
    if (mapIt == g_rooms.end()) {
        return;
    }
    auto &vec = mapIt->second;
    vec.erase(std::remove(vec.begin(), vec.end(), conn), vec.end());
    if (vec.empty()) {
        g_rooms.erase(mapIt);
    }
}

void broadcastToRoom(const std::string &projectId,
                     const drogon::WebSocketConnectionPtr &except,
                     const std::string &text)
{
    std::vector<drogon::WebSocketConnectionPtr> snapshot;
    {
        std::lock_guard<std::mutex> lock(g_roomMutex);
        const auto it = g_rooms.find(projectId);
        if (it == g_rooms.end()) {
            return;
        }
        snapshot = it->second;
    }
    for (const auto &c : snapshot) {
        if (c == except || !c || c->disconnected()) {
            continue;
        }
        c->send(text);
    }
}

std::string jsonString(const Json::Value &v)
{
    Json::StreamWriterBuilder b;
    b["indentation"] = "";
    return Json::writeString(b, v);
}

std::string welcomePayload(const std::string &projectId)
{
    Json::Value root;
    root["type"] = "server.welcome";
    root["projectId"] = projectId;
    return jsonString(root);
}

std::string userJoinedPayload(const std::string &projectId)
{
    Json::Value root;
    root["type"] = "presence.user_joined";
    root["projectId"] = projectId;
    Json::Value user;
    user["id"] = "anonymous";
    user["username"] = "anonymous";
    root["user"] = user;
    return jsonString(root);
}

std::string userLeftPayload(const std::string &projectId)
{
    Json::Value root;
    root["type"] = "presence.user_left";
    root["projectId"] = projectId;
    Json::Value user;
    user["id"] = "anonymous";
    root["user"] = user;
    return jsonString(root);
}

} // namespace

void CollaborationWsController::handleNewConnection(const drogon::HttpRequestPtr &req,
                                                      const drogon::WebSocketConnectionPtr &conn)
{
    if (!conn) {
        return;
    }
    const std::string projectId = projectIdFromRequestPath(req);
    if (projectId.empty()) {
        conn->forceClose();
        return;
    }

    auto state = std::make_shared<WsCollabContext>();
    state->projectId = projectId;
    conn->setContext(state);
    addToRoom(projectId, conn);
    conn->send(welcomePayload(projectId));
    broadcastToRoom(projectId, conn, userJoinedPayload(projectId));
}

void CollaborationWsController::handleNewMessage(const drogon::WebSocketConnectionPtr &conn,
                                                 std::string &&message,
                                                 const drogon::WebSocketMessageType & /*type*/)
{
    if (!conn || !conn->hasContext()) {
        return;
    }
    const auto ctx = conn->getContext<WsCollabContext>();
    if (!ctx || ctx->projectId.empty()) {
        return;
    }
    const std::string &projectId = ctx->projectId;

    Json::Value in;
    Json::CharReaderBuilder b;
    std::unique_ptr<Json::CharReader> reader(b.newCharReader());
    const char *begin = message.data();
    const char *end = begin + message.size();
    std::string errs;
    if (!reader->parse(begin, end, &in, &errs)) {
        return;
    }

    const auto msgType = in.isMember("type") && in["type"].isString() ? in["type"].asString() : std::string{};

    if (msgType == "heartbeat") {
        Json::Value ack;
        ack["type"] = "heartbeat.ack";
        ack["projectId"] = projectId;
        conn->send(jsonString(ack));
        return;
    }

    if (msgType == "presence.join" || msgType == "presence.current_file") {
        Json::Value out = in;
        out["projectId"] = projectId;
        broadcastToRoom(projectId, conn, jsonString(out));
    }
}

void CollaborationWsController::handleConnectionClosed(const drogon::WebSocketConnectionPtr &conn)
{
    if (!conn || !conn->hasContext()) {
        return;
    }
    const auto ctx = conn->getContext<WsCollabContext>();
    if (!ctx || ctx->projectId.empty()) {
        return;
    }
    const std::string projectId = ctx->projectId;
    removeFromRoom(projectId, conn);
    broadcastToRoom(projectId, conn, userLeftPayload(projectId));
}

} // namespace toide::server::api
