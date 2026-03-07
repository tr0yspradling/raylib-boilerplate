#include "client/runtime/singleplayer_session_service.hpp"

namespace client::runtime {

void SingleplayerSessionService::Start(std::string_view playerName, ClientSessionState& session) {
    runtime_.Start(playerName);
    PublishSessionState(session);
}

void SingleplayerSessionService::Stop(ClientSessionState& session) {
    runtime_.Stop();
    session.localPlayerId = {};
    session.predictedLocalPlayer = {};
    session.latestServerTick = 0;
    session.renderInterpolationTick = 0.0f;
    session.serverKinematics = {};
}

bool SingleplayerSessionService::IsActive() const { return runtime_.IsActive(); }

void SingleplayerSessionService::Step(const game::PlayerInputFrame& inputFrame, float fixedDeltaSeconds,
                                      ClientSessionState& session) {
    if (!runtime_.IsActive()) {
        return;
    }

    runtime_.Step(inputFrame, fixedDeltaSeconds);
    PublishSessionState(session);
}

void SingleplayerSessionService::PublishSessionState(ClientSessionState& session) const {
    if (const game::PlayerState* localPlayer = runtime_.LocalPlayer(); localPlayer != nullptr) {
        session.predictedLocalPlayer = *localPlayer;
        session.localPlayerId = localPlayer->playerId;
    } else {
        session.localPlayerId = {};
        session.predictedLocalPlayer = {};
    }

    session.serverKinematics = runtime_.Kinematics();
    session.latestServerTick = runtime_.CurrentTick();
    session.renderInterpolationTick = static_cast<float>(session.latestServerTick);
}

}  // namespace client::runtime
