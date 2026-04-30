#include "collaboration/redis_collab_store.h"

#include <drogon/drogon.h>
#include <drogon/nosql/RedisClient.h>
#include <drogon/nosql/RedisResult.h>

namespace toide::server::collab {
namespace {

drogon::nosql::RedisClientPtr redisOrNull() noexcept
{
    try {
        return drogon::app().getRedisClient("default");
    } catch (...) {
        return {};
    }
}

std::string safeSegment(std::string s)
{
    for (auto &c : s) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u <= 32 || u >= 127 || c == ':') {
            c = '_';
        }
    }
    return s;
}

} // namespace

void redisPresenceUserJoin(const std::string &projectId, const std::string &userId)
{
    auto r = redisOrNull();
    if (!r || projectId.empty() || userId.empty()) {
        return;
    }
    try {
        const std::string key = "toide:presence:project:" + safeSegment(projectId);
        r->execCommandSync([](const drogon::nosql::RedisResult &) { return 0; },
                           "SADD %s %s",
                           key.c_str(),
                           userId.c_str());
    } catch (const std::exception &e) {
        LOG_WARN << "redisPresenceUserJoin: " << e.what();
    }
}

void redisPresenceUserLeave(const std::string &projectId, const std::string &userId)
{
    auto r = redisOrNull();
    if (!r || projectId.empty() || userId.empty()) {
        return;
    }
    try {
        const std::string key = "toide:presence:project:" + safeSegment(projectId);
        r->execCommandSync([](const drogon::nosql::RedisResult &) { return 0; },
                           "SREM %s %s",
                           key.c_str(),
                           userId.c_str());
    } catch (const std::exception &e) {
        LOG_WARN << "redisPresenceUserLeave: " << e.what();
    }
}

void redisSessionSet(const std::string &clientId,
                     const std::string &userId,
                     const std::string &projectId,
                     const std::string &currentFile,
                     const std::string &timestampIso)
{
    auto r = redisOrNull();
    if (!r || clientId.empty()) {
        return;
    }
    try {
        const std::string key = "toide:session:" + safeSegment(clientId);
        r->execCommandSync([](const drogon::nosql::RedisResult &) { return 0; },
                           "HMSET %s userId %s projectId %s currentFile %s lastSeenAt %s",
                           key.c_str(),
                           userId.c_str(),
                           projectId.c_str(),
                           currentFile.c_str(),
                           timestampIso.c_str());
        r->execCommandSync([](const drogon::nosql::RedisResult &) { return 0; },
                           "EXPIRE %s %d",
                           key.c_str(),
                           900);
    } catch (const std::exception &e) {
        LOG_WARN << "redisSessionSet: " << e.what();
    }
}

void redisSessionClear(const std::string &clientId)
{
    auto r = redisOrNull();
    if (!r || clientId.empty()) {
        return;
    }
    try {
        const std::string key = "toide:session:" + safeSegment(clientId);
        r->execCommandSync([](const drogon::nosql::RedisResult &) { return 0; }, "DEL %s", key.c_str());
    } catch (const std::exception &e) {
        LOG_WARN << "redisSessionClear: " << e.what();
    }
}

void redisCursorSet(const std::string &projectId,
                    const std::string &filePath,
                    const std::string &userId,
                    const std::string &jsonPayload,
                    int ttlSeconds)
{
    auto r = redisOrNull();
    if (!r || projectId.empty() || userId.empty()) {
        return;
    }
    try {
        const std::string key = std::string("toide:cursor:project:") + safeSegment(projectId) + ":" +
                                safeSegment(filePath) + ":" + safeSegment(userId);
        r->execCommandSync([](const drogon::nosql::RedisResult &) { return 0; },
                           "SET %s %s",
                           key.c_str(),
                           jsonPayload.c_str());
        if (ttlSeconds > 0) {
            r->execCommandSync([](const drogon::nosql::RedisResult &) { return 0; },
                               "EXPIRE %s %d",
                               key.c_str(),
                               ttlSeconds);
        }
    } catch (const std::exception &e) {
        LOG_WARN << "redisCursorSet: " << e.what();
    }
}

void redisSoftLockSet(const std::string &projectId,
                      const std::string &filePath,
                      const std::string &userId,
                      const std::string &jsonPayload,
                      int ttlSeconds)
{
    auto r = redisOrNull();
    if (!r || projectId.empty() || userId.empty()) {
        return;
    }
    try {
        const std::string key = std::string("toide:softlock:project:") + safeSegment(projectId) + ":" +
                                safeSegment(filePath) + ":" + safeSegment(userId);
        r->execCommandSync([](const drogon::nosql::RedisResult &) { return 0; },
                           "SET %s %s",
                           key.c_str(),
                           jsonPayload.c_str());
        if (ttlSeconds > 0) {
            r->execCommandSync([](const drogon::nosql::RedisResult &) { return 0; },
                               "EXPIRE %s %d",
                               key.c_str(),
                               ttlSeconds);
        }
    } catch (const std::exception &e) {
        LOG_WARN << "redisSoftLockSet: " << e.what();
    }
}

} // namespace toide::server::collab
