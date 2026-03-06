#pragma once

#include "server/app/server_app.hpp"

namespace server {

class GameServer {
public:
    explicit GameServer(ServerConfig config);

    [[nodiscard]] bool Initialize();
    int Run();
    void RequestStop();

private:
    app::ServerApp app_;
};

}  // namespace server
