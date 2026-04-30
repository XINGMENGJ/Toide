#pragma once

#include "auth/auth_service.h"

#include <drogon/orm/DbClient.h>

namespace toide::server::auth::db {

drogon::orm::DbClientPtr tryDefaultDbClient() noexcept;

void ensureUsersTable(const drogon::orm::DbClientPtr &db);

AuthResult registerUserDb(const drogon::orm::DbClientPtr &db,
                          const std::string &username,
                          const std::string &password);

AuthResult loginUserDb(const drogon::orm::DbClientPtr &db,
                       const std::string &username,
                       const std::string &password);

} // namespace toide::server::auth::db
