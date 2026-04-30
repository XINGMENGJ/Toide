#pragma once

#include <string>

namespace toide::server::collab {

void redisPresenceUserJoin(const std::string &projectId, const std::string &userId);
void redisPresenceUserLeave(const std::string &projectId, const std::string &userId);

void redisSessionSet(const std::string &clientId,
                     const std::string &userId,
                     const std::string &projectId,
                     const std::string &currentFile,
                     const std::string &timestampIso);

void redisSessionClear(const std::string &clientId);

void redisCursorSet(const std::string &projectId,
                    const std::string &filePath,
                    const std::string &userId,
                    const std::string &jsonPayload,
                    int ttlSeconds);

void redisSoftLockSet(const std::string &projectId,
                      const std::string &filePath,
                      const std::string &userId,
                      const std::string &jsonPayload,
                      int ttlSeconds);

} // namespace toide::server::collab
