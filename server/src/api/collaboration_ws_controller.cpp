#include "api/collaboration_ws_controller.h"

#include <json/json.h>
#include <json/writer.h>

#include <algorithm>
#include <cctype>
#include <ctime>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace toide::server::api {
namespace {

struct WsCollabContext {
    std::string projectId;
    std::string clientId;
};

struct RoomMember {
    drogon::WebSocketConnectionPtr conn;
    std::string clientId;
    std::string currentFile;
};

std::mutex g_roomMutex;
std::unordered_map<std::string, std::vector<RoomMember>> g_rooms;

std::string trimCopy(std::string s)
{
    auto notSpace = [](unsigned char c) {
        return !std::isspace(c);
    };
    while (!s.empty() && !notSpace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }
    while (!s.empty() && !notSpace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    return s;
}

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

std::string makeGuestClientId()
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;
    return std::string("guest-") + std::to_string(dist(rng));
}

std::string utcTimestampIso8601z()
{
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[40];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buf);
}

std::string jsonString(const Json::Value &v)
{
    Json::StreamWriterBuilder b;
    b["indentation"] = "";
    return Json::writeString(b, v);
}

RoomMember *findMember(const std::string &projectId, const drogon::WebSocketConnectionPtr &conn)
{
    auto it = g_rooms.find(projectId);
    if (it == g_rooms.end()) {
        return nullptr;
    }
    for (auto &m : it->second) {
        if (m.conn == conn) {
            return &m;
        }
    }
    return nullptr;
}

void removeMemberByConn(const std::string &projectId, const drogon::WebSocketConnectionPtr &conn)
{
    auto mapIt = g_rooms.find(projectId);
    if (mapIt == g_rooms.end()) {
        return;
    }
    auto &vec = mapIt->second;
    vec.erase(std::remove_if(vec.begin(),
                             vec.end(),
                             [&conn](const RoomMember &m) {
                                 return m.conn == conn;
                             }),
              vec.end());
    if (vec.empty()) {
        g_rooms.erase(mapIt);
    }
}

void addMember(const std::string &projectId,
               const drogon::WebSocketConnectionPtr &conn,
               const std::string &clientId)
{
    RoomMember m;
    m.conn = conn;
    m.clientId = clientId;
    m.currentFile.clear();
    g_rooms[projectId].push_back(std::move(m));
}

std::string rosterSnapshotStringUnderLock(const std::string &projectId)
{
    Json::Value root;
    root["type"] = "presence.roster";
    root["projectId"] = projectId;
    Json::Value members(Json::arrayValue);
    const auto it = g_rooms.find(projectId);
    if (it != g_rooms.end()) {
        for (const auto &m : it->second) {
            Json::Value o;
            o["clientId"] = m.clientId;
            o["filePath"] = m.currentFile;
            members.append(o);
        }
    }
    root["members"] = members;
    return jsonString(root);
}

std::string welcomePayload(const std::string &projectId, const std::string &clientId)
{
    Json::Value root;
    root["type"] = "server.welcome";
    root["projectId"] = projectId;
    root["clientId"] = clientId;
    return jsonString(root);
}

std::string userJoinedPayload(const std::string &projectId, const std::string &clientId)
{
    Json::Value root;
    root["type"] = "presence.user_joined";
    root["projectId"] = projectId;
    Json::Value user;
    user["id"] = clientId;
    user["username"] = clientId;
    root["user"] = user;
    return jsonString(root);
}

std::string userLeftPayload(const std::string &projectId, const std::string &clientId)
{
    Json::Value root;
    root["type"] = "presence.user_left";
    root["projectId"] = projectId;
    Json::Value user;
    user["id"] = clientId;
    root["user"] = user;
    return jsonString(root);
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
        for (const auto &m : it->second) {
            snapshot.push_back(m.conn);
        }
    }
    for (const auto &c : snapshot) {
        if (c == except || !c || c->disconnected()) {
            continue;
        }
        c->send(text);
    }
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

    std::string clientId = trimCopy(req->getParameter("clientId"));
    if (clientId.empty()) {
        clientId = makeGuestClientId();
    }

    auto state = std::make_shared<WsCollabContext>();
    state->projectId = projectId;
    state->clientId = clientId;
    conn->setContext(state);

    std::string rosterJson;
    {
        std::lock_guard<std::mutex> lock(g_roomMutex);
        addMember(projectId, conn, clientId);
        rosterJson = rosterSnapshotStringUnderLock(projectId);
    }

    conn->send(welcomePayload(projectId, clientId));
    conn->send(rosterJson);

    broadcastToRoom(projectId, conn, userJoinedPayload(projectId, clientId));
}

void CollaborationWsController::handleNewMessage(const drogon::WebSocketConnectionPtr &conn,
                                                 std::string &&message,
                                                 const drogon::WebSocketMessageType & /*type*/)
{
    if (!conn || !conn->hasContext()) {
        return;
    }
    const auto ctx = conn->getContext<WsCollabContext>();
    if (!ctx || ctx->projectId.empty() || ctx->clientId.empty()) {
        return;
    }
    const std::string &projectId = ctx->projectId;
    const std::string &clientId = ctx->clientId;

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
        ack["clientId"] = clientId;
        conn->send(jsonString(ack));
        return;
    }

    if (msgType == "presence.current_file") {
        std::string filePath = in.isMember("filePath") && in["filePath"].isString() ? in["filePath"].asString() : std::string{};
        {
            std::lock_guard<std::mutex> lock(g_roomMutex);
            RoomMember *m = findMember(projectId, conn);
            if (m != nullptr) {
                m->currentFile = filePath;
            }
        }
        Json::Value changed;
        changed["type"] = "presence.current_file_changed";
        changed["projectId"] = projectId;
        changed["userId"] = clientId;
        changed["filePath"] = filePath;
        changed["timestamp"] = utcTimestampIso8601z();
        broadcastToRoom(projectId, conn, jsonString(changed));
        return;
    }

    if (msgType == "collab.file_saved") {
        Json::Value out;
        out["type"] = "collab.file_saved";
        out["projectId"] = projectId;
        out["clientId"] = clientId;
        if (in.isMember("filePath") && in["filePath"].isString()) {
            out["filePath"] = in["filePath"].asString();
        }
        out["timestamp"] = utcTimestampIso8601z();
        broadcastToRoom(projectId, conn, jsonString(out));
        return;
    }

    if (msgType == "presence.join") {
        Json::Value echo = in;
        echo["projectId"] = projectId;
        echo["clientId"] = clientId;
        broadcastToRoom(projectId, conn, jsonString(echo));
    }
}

void CollaborationWsController::handleConnectionClosed(const drogon::WebSocketConnectionPtr &conn)
{
    if (!conn || !conn->hasContext()) {
        return;
    }
    const auto ctx = conn->getContext<WsCollabContext>();
    if (!ctx || ctx->projectId.empty() || ctx->clientId.empty()) {
        return;
    }
    const std::string projectId = ctx->projectId;
    const std::string clientId = ctx->clientId;
    {
        std::lock_guard<std::mutex> lock(g_roomMutex);
        removeMemberByConn(projectId, conn);
    }
    broadcastToRoom(projectId, conn, userLeftPayload(projectId, clientId));
}

} // namespace toide::server::api
