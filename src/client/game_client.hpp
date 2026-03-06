#pragma once

#include "client/app/client_app.hpp"

namespace client {

class GameClient {
public:
    explicit GameClient(ClientConfig config);

    [[nodiscard]] bool Initialize();
    int Run();

private:
    app::ClientApp app_;
};

}  // namespace client
