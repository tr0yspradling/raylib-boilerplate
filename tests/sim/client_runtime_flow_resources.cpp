#include <cassert>

#include "client/runtime/runtime_resources.hpp"

int main() {
    using client::core::RuntimeMode;
    using client::runtime::ClientFlowState;
    using client::runtime::ClientRuntimeSessionSnapshot;
    using client::runtime::LocalServerStartupState;
    using client::runtime::MenuStatusMessageForReturn;
    using client::runtime::RefreshClientFlowState;

    {
        ClientFlowState flow;
        LocalServerStartupState localServer;
        flow.runtime.mode = RuntimeMode::Boot;
        flow.runtime.splashCompleted = true;

        RefreshClientFlowState(flow, localServer, {});
        assert(flow.runtime.mode == RuntimeMode::Menu);
    }

    {
        ClientFlowState flow;
        LocalServerStartupState localServer;
        flow.runtime.mode = RuntimeMode::JoiningServer;
        flow.disconnectReason = "timeout";

        RefreshClientFlowState(flow, localServer, {});
        assert(flow.runtime.mode == RuntimeMode::JoiningServer);
        assert(!flow.runtime.joiningInProgress);
        assert(flow.statusMessage == "Join failed: timeout");
        assert(flow.disconnectReason.empty());
        assert(flow.runtime.disconnectReason.empty());
    }

    {
        ClientFlowState flow;
        LocalServerStartupState localServer;
        flow.runtime.mode = RuntimeMode::JoiningServer;
        flow.statusMessage = "Connecting...";
        flow.runtime.requestedLocalServerStart = true;
        localServer.startupInProgress = true;

        RefreshClientFlowState(flow, localServer,
                               {
                                   .connecting = false,
                                   .connected = true,
                                   .serverWelcomed = true,
                                   .singleplayerActive = false,
                               });

        assert(flow.runtime.mode == RuntimeMode::Multiplayer);
        assert(!flow.runtime.joiningInProgress);
        assert(!flow.runtime.requestedLocalServerStart);
        assert(!localServer.startupInProgress);
        assert(flow.statusMessage.empty());
    }

    {
        ClientFlowState flow;
        LocalServerStartupState localServer;
        flow.runtime.mode = RuntimeMode::Multiplayer;

        RefreshClientFlowState(flow, localServer, {});
        assert(flow.runtime.mode == RuntimeMode::Disconnected);
        assert(flow.disconnectReason == "connection closed");
        assert(flow.runtime.disconnectReason == "connection closed");
    }

    {
        ClientFlowState flow;
        LocalServerStartupState localServer;
        flow.runtime.mode = RuntimeMode::Singleplayer;

        RefreshClientFlowState(flow, localServer, {});
        assert(flow.runtime.mode == RuntimeMode::Menu);
    }

    {
        ClientFlowState flow;
        flow.runtime.mode = RuntimeMode::Disconnected;
        flow.disconnectReason = "server closed";

        assert(MenuStatusMessageForReturn(flow, {}) == "server closed");
        assert(MenuStatusMessageForReturn(flow, "custom") == "custom");
    }

    return 0;
}
