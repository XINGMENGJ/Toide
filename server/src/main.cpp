#include "api/health_controller.h"

#include <drogon/drogon.h>

int main()
{
    drogon::app().loadConfigFile("server/config/server.json");
    drogon::app().run();
    return 0;
}
