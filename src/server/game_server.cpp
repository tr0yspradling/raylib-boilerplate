#include "server/game_server.hpp"

namespace server {

GameServer::GameServer(ServerConfig config) : app_(std::move(config)) {}

bool GameServer::Initialize() { return app_.Initialize(); }

int GameServer::Run() { return app_.Run(); }

void GameServer::RequestStop() { app_.RequestStop(); }

}  // namespace server
