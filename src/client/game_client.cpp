#include "client/game_client.hpp"

namespace client {

GameClient::GameClient(ClientConfig config) : app_(std::move(config)) {}

bool GameClient::Initialize() { return app_.Initialize(); }

int GameClient::Run() { return app_.Run(); }

}  // namespace client
