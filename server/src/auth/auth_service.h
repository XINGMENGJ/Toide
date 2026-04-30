#pragma once

#include <optional>
#include <string>

namespace toide::server::auth {

struct AuthenticatedUser {
    std::string id;
    std::string username;
};

struct AuthResult {
    bool ok = false;
    std::string message;
    std::string token;
    AuthenticatedUser user;
};

class AuthService final {
public:
    static AuthService &instance();

    AuthResult registerUser(const std::string &username, const std::string &password);
    AuthResult login(const std::string &username, const std::string &password);
    std::optional<AuthenticatedUser> userFromToken(const std::string &token) const;

private:
    AuthService();
};

} // namespace toide::server::auth
