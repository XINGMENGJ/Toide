#include "health/health_response.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main()
{
    const auto response = toide::server::buildHealthResponse();

    require(response.find("\"service\":\"toide-server\"") != std::string::npos, "missing service name");
    require(response.find("\"status\":\"ok\"") != std::string::npos, "missing ok status");
    require(response.find("\"version\":\"0.1.0\"") != std::string::npos, "missing version");
    require(response.find("\"timestamp\"") != std::string::npos, "missing timestamp");

    const auto dependencies = toide::server::buildDependencyHealthResponse();
    require(dependencies.find("\"mysql\"") != std::string::npos, "missing mysql dependency");
    require(dependencies.find("\"redis\"") != std::string::npos, "missing redis dependency");
    require(dependencies.find("\"configured\":false") != std::string::npos, "missing configured state");

    const auto configuredDependencies = toide::server::buildDependencyHealthResponse({true, true});
    require(configuredDependencies.find("\"status\":\"ok\"") != std::string::npos, "missing ok dependency status");
    require(configuredDependencies.find("\"configured\":true") != std::string::npos, "missing configured true state");

    return 0;
}
