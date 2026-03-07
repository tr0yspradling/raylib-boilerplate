#include <cassert>

#include "client/runtime/singleplayer_session_service.hpp"

int main() {
    client::runtime::SingleplayerSessionService service;
    client::runtime::ClientSessionState session;

    service.Start("alice", session);

    assert(service.IsActive());
    assert(session.localPlayerId.IsValid());
    assert(session.predictedLocalPlayer.displayName == "alice");
    assert(session.predictedLocalPlayer.position.x == 0.0f);
    assert(session.predictedLocalPlayer.position.y == 0.0f);
    assert(session.latestServerTick == 0);
    assert(session.renderInterpolationTick == 0.0f);

    service.Step({.clientTick = 0, .sequence = 1, .moveX = 1.0f, .jumpPressed = false}, 1.0f / 30.0f, session);
    assert(session.predictedLocalPlayer.position.x > 0.0f);
    assert(session.predictedLocalPlayer.position.y == 0.0f);
    assert(session.latestServerTick == 1);
    assert(session.renderInterpolationTick == 1.0f);

    const float yBeforeJump = session.predictedLocalPlayer.position.y;
    service.Step({.clientTick = 1, .sequence = 2, .moveX = 0.0f, .jumpPressed = true}, 1.0f / 30.0f, session);
    assert(session.predictedLocalPlayer.position.y > yBeforeJump);
    assert(session.predictedLocalPlayer.onGround == false);
    assert(session.latestServerTick == 2);
    assert(session.renderInterpolationTick == 2.0f);

    service.Stop(session);
    assert(!service.IsActive());
    assert(!session.localPlayerId.IsValid());
    assert(session.predictedLocalPlayer.playerId == shared::game::PlayerId{});
    assert(session.latestServerTick == 0);
    assert(session.renderInterpolationTick == 0.0f);

    return 0;
}
