#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "shared/game/entity.hpp"
#include "shared/game/world.hpp"

namespace shared::game {

struct SnapshotPlayerView {
    PlayerId playerId{};
    EntityId entityId{};
    std::string displayName;
    Vec2f position{};
    Vec2f velocity{};
    bool onGround = true;
    uint32_t lastProcessedInputSequence = 0;
};

struct SnapshotView {
    TickId tick = 0;
    std::vector<SnapshotPlayerView> players;
};

class GameState {
public:
    explicit GameState(WorldConfig worldConfig = {}, PlayerKinematicsConfig kinematics = {})
        : worldConfig_(worldConfig), kinematics_(kinematics) {}

    PlayerState& SpawnPlayer(PlayerId playerId, std::string playerName, Vec2f spawnPosition = {0.0f, 0.0f}) {
        auto [it, inserted] = players_.try_emplace(playerId);
        PlayerState& player = it->second;
        player.playerId = playerId;
        player.entityId = EntityId{playerId.Value()};
        player.displayName = std::move(playerName);
        player.position = spawnPosition;
        player.velocity = {0.0f, 0.0f};
        player.onGround = true;
        player.lastInput = {};
        player.lastReceivedInputSequence = 0;
        player.lastProcessedInputSequence = 0;

        if (inserted) {
            playerOrder_.push_back(playerId);
        }

        return player;
    }

    void RemovePlayer(PlayerId playerId) {
        players_.erase(playerId);
        playerOrder_.erase(std::remove(playerOrder_.begin(), playerOrder_.end(), playerId), playerOrder_.end());
    }

    [[nodiscard]] bool HasPlayer(PlayerId playerId) const { return players_.contains(playerId); }

    void ApplyInputFrame(PlayerId playerId, const PlayerInputFrame& inputFrame) {
        auto it = players_.find(playerId);
        if (it == players_.end()) {
            return;
        }

        PlayerState& player = it->second;
        if (inputFrame.sequence < player.lastReceivedInputSequence) {
            return;
        }

        player.lastReceivedInputSequence = inputFrame.sequence;
        player.lastInput = inputFrame;
    }

    void Step(float fixedDeltaSeconds) {
        ++tick_;
        for (PlayerId playerId : playerOrder_) {
            auto it = players_.find(playerId);
            if (it == players_.end()) {
                continue;
            }

            PlayerState& player = it->second;
            SimulatePlayerStep(player, player.lastInput, fixedDeltaSeconds, kinematics_);
            player.lastProcessedInputSequence = player.lastInput.sequence;
        }
    }

    [[nodiscard]] SnapshotView BuildSnapshotView() const {
        SnapshotView snapshot;
        snapshot.tick = tick_;
        snapshot.players.reserve(playerOrder_.size());

        for (PlayerId playerId : playerOrder_) {
            auto it = players_.find(playerId);
            if (it == players_.end()) {
                continue;
            }

            const PlayerState& player = it->second;
            snapshot.players.push_back({
                .playerId = player.playerId,
                .entityId = player.entityId,
                .displayName = player.displayName,
                .position = player.position,
                .velocity = player.velocity,
                .onGround = player.onGround,
                .lastProcessedInputSequence = player.lastProcessedInputSequence,
            });
        }

        return snapshot;
    }

    [[nodiscard]] const std::unordered_map<PlayerId, PlayerState, IdHash<PlayerIdTag>>& Players() const {
        return players_;
    }

    [[nodiscard]] TickId CurrentTick() const { return tick_; }
    [[nodiscard]] const PlayerKinematicsConfig& Kinematics() const { return kinematics_; }
    [[nodiscard]] const WorldConfig& World() const { return worldConfig_; }

private:
    WorldConfig worldConfig_{};
    PlayerKinematicsConfig kinematics_{};
    TickId tick_ = 0;
    std::unordered_map<PlayerId, PlayerState, IdHash<PlayerIdTag>> players_;
    std::vector<PlayerId> playerOrder_;
};

}  // namespace shared::game
