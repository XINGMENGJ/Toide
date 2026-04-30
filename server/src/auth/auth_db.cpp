#include "auth/auth_db.h"

#include <drogon/drogon.h>

#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <trantor/utils/Utilities.h>

namespace toide::server::auth::db {
namespace {

std::mutex g_schemaMutex;

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

std::string makePasswordHash(const std::string &salt, const std::string &password)
{
    return sha256Hex(salt + ":" + password);
}

std::string syntheticEmail(const std::string &username)
{
    return username + "@toide.local";
}

std::string randomHex()
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<unsigned long long> dist;
    std::ostringstream os;
    os << std::hex << dist(rng) << dist(rng);
    return os.str();
}

std::string makeUserId()
{
    std::string h = randomHex();
    while (h.size() < 32) {
        h.append(randomHex());
    }
    h.resize(32);
    return h.substr(0, 8) + "-" + h.substr(8, 4) + "-" + h.substr(12, 4) + "-" + h.substr(16, 4) + "-" + h.substr(20, 12);
}

bool isDuplicateKeyError(const char *what)
{
    if (what == nullptr) {
        return false;
    }
    const std::string msg(what);
    return msg.find("Duplicate") != std::string::npos || msg.find("1062") != std::string::npos;
}

} // namespace

drogon::orm::DbClientPtr tryDefaultDbClient() noexcept
{
    try {
        return drogon::app().getDbClient("default");
    } catch (const std::exception &e) {
        LOG_WARN << "MySQL client unavailable: " << e.what();
        return {};
    } catch (...) {
        LOG_WARN << "MySQL client unavailable (unknown error).";
        return {};
    }
}

void ensureUsersTable(const drogon::orm::DbClientPtr &db)
{
    if (!db) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_schemaMutex);
    static bool done = false;
    if (done) {
        return;
    }
    db->execSqlSync(
        "CREATE TABLE IF NOT EXISTS users ("
        "id CHAR(36) PRIMARY KEY,"
        "username VARCHAR(64) NOT NULL UNIQUE,"
        "email VARCHAR(255) NOT NULL UNIQUE,"
        "salt VARCHAR(128) NOT NULL,"
        "password_hash VARCHAR(128) NOT NULL,"
        "created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),"
        "last_login_at DATETIME(3) NULL"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    done = true;
}

AuthResult registerUserDb(const drogon::orm::DbClientPtr &db,
                          const std::string &username,
                          const std::string &password)
{
    AuthResult out;
    if (!db) {
        out.message = "Database unavailable.";
        return out;
    }
    ensureUsersTable(db);
    const std::string id = makeUserId();
    const std::string salt = randomHex();
    const std::string phash = makePasswordHash(salt, password);
    const std::string email = syntheticEmail(username);
    try {
        db->execSqlSync(
            "INSERT INTO users (id, username, email, salt, password_hash) VALUES (?,?,?,?,?)",
            id,
            username,
            email,
            salt,
            phash);
    } catch (const std::exception &e) {
        if (isDuplicateKeyError(e.what())) {
            out.message = "Username already exists.";
            return out;
        }
        LOG_WARN << "registerUserDb: " << e.what();
        out.message = "Database error.";
        return out;
    }
    out.ok = true;
    out.message = "Registered.";
    out.user = AuthenticatedUser{id, username};
    return out;
}

AuthResult loginUserDb(const drogon::orm::DbClientPtr &db,
                       const std::string &username,
                       const std::string &password)
{
    AuthResult out;
    if (!db) {
        out.message = "Database unavailable.";
        return out;
    }
    ensureUsersTable(db);
    try {
        const auto r =
            db->execSqlSync("SELECT id, username, salt, password_hash FROM users WHERE username=?", username);
        if (r.size() == 0) {
            out.message = "Invalid username or password.";
            return out;
        }
        const auto row = r[0];
        const std::string id = row["id"].as<std::string>();
        const std::string name = row["username"].as<std::string>();
        const std::string salt = row["salt"].as<std::string>();
        const std::string hash = row["password_hash"].as<std::string>();
        if (hash != makePasswordHash(salt, password)) {
            out.message = "Invalid username or password.";
            return out;
        }
        db->execSqlSync("UPDATE users SET last_login_at = CURRENT_TIMESTAMP(3) WHERE id=?", id);
        out.ok = true;
        out.message = "Logged in.";
        out.user = AuthenticatedUser{id, name};
        return out;
    } catch (const std::exception &e) {
        LOG_WARN << "loginUserDb: " << e.what();
        out.message = "Database error.";
        return out;
    }
}

} // namespace toide::server::auth::db
