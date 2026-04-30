#include "auth/auth_service.h"

#include <trantor/utils/Utilities.h>

#include <chrono>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <unordered_map>

namespace toide::server::auth {
namespace {

struct StoredUser {
    std::string id;
    std::string username;
    std::string salt;
    std::string passwordHash;
};

std::mutex g_authMutex;
std::unordered_map<std::string, StoredUser> g_usersByName;
std::unordered_map<std::string, AuthenticatedUser> g_tokens;

std::string hexHash(const trantor::utils::Hash256 &hash)
{
    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (const unsigned char b : hash.bytes) {
        os << std::setw(2) << static_cast<int>(b);
    }
    return os.str();
}

std::string sha256Hex(const std::string &value)
{
    return hexHash(trantor::utils::sha256(value));
}

std::string randomHex()
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<unsigned long long> dist;
    std::ostringstream os;
    os << std::hex << dist(rng) << dist(rng);
    return os.str();
}

std::string trimmed(std::string s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    return s;
}

std::string makePasswordHash(const std::string &salt, const std::string &password)
{
    return sha256Hex(salt + ":" + password);
}

std::string makeToken(const AuthenticatedUser &user)
{
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    return std::string("toide_") + sha256Hex(user.id + ":" + user.username + ":" + std::to_string(now) + ":" + randomHex());
}

} // namespace

AuthService &AuthService::instance()
{
    static AuthService service;
    return service;
}

AuthService::AuthService() = default;

AuthResult AuthService::registerUser(const std::string &usernameInput, const std::string &password)
{
    const std::string username = trimmed(usernameInput);
    if (username.size() < 3) {
        AuthResult result;
        result.message = "Username must be at least 3 characters.";
        return result;
    }
    if (password.size() < 6) {
        AuthResult result;
        result.message = "Password must be at least 6 characters.";
        return result;
    }

    std::lock_guard<std::mutex> lock(g_authMutex);
    if (g_usersByName.find(username) != g_usersByName.end()) {
        AuthResult result;
        result.message = "Username already exists.";
        return result;
    }

    StoredUser stored;
    stored.id = randomHex();
    stored.username = username;
    stored.salt = randomHex();
    stored.passwordHash = makePasswordHash(stored.salt, password);
    g_usersByName.insert({username, stored});

    AuthenticatedUser user{stored.id, stored.username};
    const std::string token = makeToken(user);
    g_tokens.insert({token, user});
    return {.ok = true, .message = "Registered.", .token = token, .user = user};
}

AuthResult AuthService::login(const std::string &usernameInput, const std::string &password)
{
    const std::string username = trimmed(usernameInput);
    std::lock_guard<std::mutex> lock(g_authMutex);
    const auto it = g_usersByName.find(username);
    if (it == g_usersByName.end()) {
        AuthResult result;
        result.message = "Invalid username or password.";
        return result;
    }
    const StoredUser &stored = it->second;
    if (stored.passwordHash != makePasswordHash(stored.salt, password)) {
        AuthResult result;
        result.message = "Invalid username or password.";
        return result;
    }

    AuthenticatedUser user{stored.id, stored.username};
    const std::string token = makeToken(user);
    g_tokens.insert({token, user});
    return {.ok = true, .message = "Logged in.", .token = token, .user = user};
}

std::optional<AuthenticatedUser> AuthService::userFromToken(const std::string &token) const
{
    std::lock_guard<std::mutex> lock(g_authMutex);
    const auto it = g_tokens.find(token);
    if (it == g_tokens.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace toide::server::auth
