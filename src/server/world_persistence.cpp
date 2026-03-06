#include "server/world_persistence.hpp"

#include <filesystem>
#include <fstream>

namespace server {

bool SaveWorldState(const std::string& filePath, const shared::game::GameState& state, std::string& error) {
    const std::filesystem::path path{filePath};
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            error = "failed to create persistence directory: " + ec.message();
            return false;
        }
    }

    std::ofstream out{path};
    if (!out.is_open()) {
        error = "failed to open persistence file for write";
        return false;
    }

    const shared::game::SnapshotView snapshot = state.BuildSnapshotView();
    out << "version=1\n";
    out << "tick=" << snapshot.tick << '\n';
    out << "players=" << snapshot.players.size() << '\n';

    for (const shared::game::SnapshotPlayerView& player : snapshot.players) {
        out << "player=" << player.playerId.Value() << ',' << player.entityId.Value() << ',' << player.displayName << ','
            << player.position.x << ',' << player.position.y << ',' << player.velocity.x << ',' << player.velocity.y
            << ',' << static_cast<int>(player.onGround) << ',' << player.lastProcessedInputSequence << '\n';
    }

    return true;
}

bool LoadWorldState(const std::string& filePath, shared::game::GameState&, std::string& warning) {
    if (!std::filesystem::exists(filePath)) {
        warning = "persistence file not found, starting fresh";
        return false;
    }

    // TODO: implement robust versioned load path. The save format is present and
    // versioned, but this first network slice starts a fresh in-memory state.
    warning = "persistence load scaffolded; file found but load path not yet applied to live state";
    return false;
}

}  // namespace server
