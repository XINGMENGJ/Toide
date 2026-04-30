#include <drogon/drogon.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

std::filesystem::path moduleDir()
{
#ifdef _WIN32
    std::wstring buf(4096, L'\0');
    const DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (n > 0 && n < buf.size()) {
        buf.resize(n);
        return std::filesystem::path(buf).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

std::filesystem::path resolveServerConfig(int argc, char *argv[])
{
    if (const char *env = std::getenv("TOIDE_SERVER_CONFIG")) {
        return std::filesystem::path(env);
    }

    std::vector<std::filesystem::path> candidates;
    const auto exeDir = moduleDir();
    auto dir = exeDir;
    for (int depth = 0; depth < 8; ++depth) {
        candidates.push_back(dir / "server" / "config" / "server.json");
        candidates.push_back(dir / "config" / "server.json");
        if (!dir.has_parent_path() || dir == dir.root_path()) {
            break;
        }
        dir = dir.parent_path();
    }
    candidates.push_back(std::filesystem::current_path() / "server" / "config" / "server.json");

    if (argc > 0 && argv[0] != nullptr && argv[0][0] != '\0') {
        try {
            std::filesystem::path fromArgv = std::filesystem::absolute(argv[0]);
            auto d = fromArgv.parent_path();
            for (int depth = 0; depth < 4; ++depth) {
                candidates.push_back(d / "server" / "config" / "server.json");
                if (!d.has_parent_path()) {
                    break;
                }
                d = d.parent_path();
            }
        } catch (...) {
        }
    }

    for (const auto &p : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(p, ec)) {
            return p;
        }
    }
    return std::filesystem::path("server/config/server.json");
}

} // namespace

int main(int argc, char *argv[])
{
    const auto configPath = resolveServerConfig(argc, argv);
    std::error_code ec;
    if (!std::filesystem::exists(configPath, ec)) {
        std::cerr << "Toide server: config file not found:\n  " << configPath.string() << "\n"
                  << "Tip: set TOIDE_SERVER_CONFIG to the full path of server.json, or build with CMake "
                     "(config is copied next to the executable).\n";
        return 1;
    }

    const auto absConfig = std::filesystem::weakly_canonical(configPath, ec);
    std::cout << "Toide server: loading " << (ec ? configPath.string() : absConfig.string()) << "\n";

    drogon::app().loadConfigFile((ec ? configPath : absConfig).string());
    drogon::app().run();
    return 0;
}
