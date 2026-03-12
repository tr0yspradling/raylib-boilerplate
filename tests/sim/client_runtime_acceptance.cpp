#include <cassert>
#include <filesystem>
#include <flecs.h>
#include <memory>
#include <string>

#include "client/components/components.hpp"
#include "client/core/config.hpp"
#include "client/core/server_launcher.hpp"
#include "client/runtime/client_runtime.hpp"
#include "client/runtime/client_runtime_policy.hpp"
#include "client/ui/ui_policy.hpp"

namespace client::runtime {

struct ClientRuntimeTestHooks {
    static void InitializeWithoutWindow(ClientRuntime& runtime, flecs::world world) {
        runtime.world_ = world;
        runtime.InitializeWorldState(world);
    }

    static void SetServerLauncher(ClientRuntime& runtime, std::unique_ptr<core::IServerLauncher> launcher) {
        runtime.serverLauncher_ = std::move(launcher);
    }

    static void ReturnToMenu(ClientRuntime& runtime, flecs::world world, std::string statusMessage = {}) {
        runtime.ReturnToMenu(world, std::move(statusMessage));
    }
};

}  // namespace client::runtime

namespace {

using client::runtime::ClientRuntime;
using client::runtime::ClientRuntimeTestHooks;

class FailingServerLauncher final : public client::core::IServerLauncher {
public:
    [[nodiscard]] bool Launch(const client::core::LocalServerLaunchRequest&, std::string& error) override {
        error = "launch blocked by test";
        return false;
    }

    [[nodiscard]] client::core::ServerProcessStatus PollStatus() override {
        return {.state = client::core::ServerProcessState::NotRunning, .exitCode = std::nullopt};
    }

    void Stop() override {}
};

void SeedClientRuntimeWorld(flecs::world& world) {
    world.set<client::components::WorldRenderState>({});
    world.set<client::components::StatusRenderState>({});
    world.set<client::components::NetworkDebugState>({});
    world.set<client::ui::UiDocument>({});
    world.set<client::ui::UiInputState>({});
    world.set<client::ui::UiInteractionState>({});
    world.set<client::ui::UiCommandQueue>({});
    world.set<client::ui::MenuScreenState>({});
    world.set<client::ui::JoinServerScreenState>({});
    world.set<client::ui::OptionsScreenState>({});
    world.set<client::ui::ScreenState>({});
    world.set<client::runtime::ClientFlowState>({});
    world.set<client::runtime::LocalServerStartupState>({});
    world.set<client::runtime::ClientSessionState>({});
}

ClientRuntime MakeRuntime(std::filesystem::path configPath = {}) {
    client::ClientConfig config;
    config.skipSplash = true;
    config.configFilePath = configPath.string();
    config.executablePath = "/tmp/game_client";
    return ClientRuntime{config};
}

void BootToMenu(ClientRuntime& runtime, flecs::world& world) {
    SeedClientRuntimeWorld(world);
    ClientRuntimeTestHooks::InitializeWithoutWindow(runtime, world);
    runtime.ProcessRuntimeIntent(world);
    runtime.BuildUiState(world);

    const auto& flow = world.get<client::runtime::ClientFlowState>();
    assert(flow.runtime.mode == client::core::RuntimeMode::Menu);
    assert(world.get<client::ui::ScreenState>().activeScene == client::core::SceneKind::MainMenu);
}

void TestStartServerLaunchFailureReturnsToMenu() {
    flecs::world world;
    ClientRuntime runtime = MakeRuntime();
    BootToMenu(runtime, world);
    ClientRuntimeTestHooks::SetServerLauncher(runtime, std::make_unique<FailingServerLauncher>());

    const client::ui::UiDocument& document = world.get<client::ui::UiDocument>();
    assert(!document.widgets.empty());
    const client::ui::UiRect& startServerBounds = document.widgets.front().bounds;
    world.set<client::ui::UiInputState>({
        .mouseX = startServerBounds.x + 4.0f,
        .mouseY = startServerBounds.y + 4.0f,
        .mouseMoved = true,
        .primaryPressed = true,
    });

    runtime.HandleUiInteraction(world);
    runtime.ProcessRuntimeIntent(world);

    const auto& flow = world.get<client::runtime::ClientFlowState>();
    const auto& screenState = world.get<client::ui::ScreenState>();
    const auto& localServer = world.get<client::runtime::LocalServerStartupState>();
    assert(flow.runtime.mode == client::core::RuntimeMode::Menu);
    assert(screenState.activeScene == client::core::SceneKind::MainMenu);
    assert(flow.statusMessage == "Failed to launch local dedicated server: launch blocked by test");
    assert(!localServer.startupInProgress);
    assert(!localServer.ownsProcess);
}

void TestMenuAndOptionsDocumentsUsePolicyCatalogs() {
    flecs::world world;
    ClientRuntime runtime = MakeRuntime();
    BootToMenu(runtime, world);

    const client::ui::UiDocument& menuDocument = world.get<client::ui::UiDocument>();
    assert(menuDocument.title == std::string{client::ui::policy::copy::kMenuTitle});
    assert(menuDocument.subtitle == std::string{client::ui::policy::copy::kMenuSubtitle});
    assert(menuDocument.footerHint == std::string{client::ui::policy::copy::kMenuFooterHint});
    assert(!menuDocument.widgets.empty());
    assert(menuDocument.widgets.front().id == client::ui::UiWidgetId::MenuStartServer);
    assert(menuDocument.widgets.front().bounds.width == client::ui::policy::layout::kMenuWidth);
    assert(menuDocument.widgets.front().bounds.height == client::ui::policy::layout::kRowHeight);

    client::ui::MenuScreenState& menu = world.get_mut<client::ui::MenuScreenState>();
    menu.SetSelectedIndex(3);
    world.get_mut<client::ui::UiInteractionState>().focusedWidget = client::ui::UiWidgetId::MenuOptions;
    world.set<client::ui::UiInputState>({.acceptPressed = true});

    runtime.HandleUiInteraction(world);
    runtime.ProcessRuntimeIntent(world);
    runtime.BuildUiState(world);

    const client::ui::UiDocument& optionsDocument = world.get<client::ui::UiDocument>();
    assert(optionsDocument.title == std::string{client::ui::policy::copy::kOptionsTitle});
    assert(optionsDocument.subtitle == std::string{client::ui::policy::copy::kOptionsSubtitle});
    assert(optionsDocument.footerHint == std::string{client::ui::policy::copy::kFormFooterHint});
    assert(!optionsDocument.widgets.empty());
    assert(optionsDocument.widgets.front().id == client::ui::UiWidgetId::OptionsPlayerName);
    assert(optionsDocument.widgets.front().bounds.width == client::ui::policy::layout::kOptionsWidth);
    assert(optionsDocument.widgets.front().bounds.height == client::ui::policy::layout::kRowHeight);
}

void TestLocalServerStartupTimeoutReturnsToMenu() {
    flecs::world world;
    ClientRuntime runtime = MakeRuntime();
    BootToMenu(runtime, world);

    auto& flow = world.get_mut<client::runtime::ClientFlowState>();
    auto& localServer = world.get_mut<client::runtime::LocalServerStartupState>();

    flow.runtime.mode = client::core::RuntimeMode::StartingLocalServer;
    flow.runtime.requestedLocalServerStart = false;
    flow.statusMessage = std::string{client::runtime::policy::kLocalServerWaitingStatus};
    localServer.startupInProgress = true;
    localServer.launchStartedAt =
        std::chrono::steady_clock::now() - client::runtime::policy::kLocalServerStartupTimeout -
        std::chrono::milliseconds{1};

    runtime.ProcessRuntimeIntent(world);

    assert(flow.runtime.mode == client::core::RuntimeMode::Menu);
    assert(flow.statusMessage == std::string{client::runtime::policy::kLocalServerTimeoutStatus});
    assert(!localServer.startupInProgress);
    assert(world.get<client::ui::ScreenState>().activeScene == client::core::SceneKind::MainMenu);
    assert(world.get<client::ui::UiInteractionState>().focusedWidget == client::ui::UiWidgetId::MenuStartServer);
}

void TestSingleplayerPublishesGameplayState() {
    flecs::world world;
    ClientRuntime runtime = MakeRuntime();
    BootToMenu(runtime, world);

    client::ui::MenuScreenState& menu = world.get_mut<client::ui::MenuScreenState>();
    menu.SetSelectedIndex(2);
    world.get_mut<client::ui::UiInteractionState>().focusedWidget = client::ui::UiWidgetId::MenuSingleplayer;
    runtime.BuildUiState(world);
    world.set<client::ui::UiInputState>({.acceptPressed = true});

    runtime.HandleUiInteraction(world);
    runtime.ProcessRuntimeIntent(world);
    runtime.PublishPresentation(world, 0.0f);

    const auto& flow = world.get<client::runtime::ClientFlowState>();
    const auto& session = world.get<client::runtime::ClientSessionState>();
    const auto& worldState = world.get<client::components::WorldRenderState>();
    assert(flow.runtime.mode == client::core::RuntimeMode::Singleplayer);
    assert(session.localPlayerId.IsValid());
    assert(session.predictedLocalPlayer.playerId == session.localPlayerId);
    assert(worldState.localPlayer.playerId == session.localPlayerId);
    assert(worldState.localPlayer.isLocal);

    ClientRuntimeTestHooks::ReturnToMenu(runtime, world);
    assert(world.get<client::runtime::ClientFlowState>().runtime.mode == client::core::RuntimeMode::Menu);
    assert(!world.get<client::runtime::ClientSessionState>().localPlayerId.IsValid());
}

void TestOptionsSavePersistsAndRefreshesJoinDefaults() {
    const std::filesystem::path tempRoot =
        std::filesystem::temp_directory_path() / "raylib_boilerplate_client_runtime_acceptance";
    std::filesystem::remove_all(tempRoot);
    std::filesystem::create_directories(tempRoot);
    const std::filesystem::path configPath = tempRoot / "client.cfg";

    flecs::world world;
    ClientRuntime runtime = MakeRuntime(configPath);
    BootToMenu(runtime, world);

    client::ui::MenuScreenState& menu = world.get_mut<client::ui::MenuScreenState>();
    menu.SetSelectedIndex(3);
    world.get_mut<client::ui::UiInteractionState>().focusedWidget = client::ui::UiWidgetId::MenuOptions;
    runtime.BuildUiState(world);
    world.set<client::ui::UiInputState>({.acceptPressed = true});

    runtime.HandleUiInteraction(world);
    runtime.ProcessRuntimeIntent(world);

    auto& flow = world.get_mut<client::runtime::ClientFlowState>();
    auto& options = world.get_mut<client::ui::OptionsScreenState>();
    auto& interaction = world.get_mut<client::ui::UiInteractionState>();
    auto& join = world.get_mut<client::ui::JoinServerScreenState>();
    assert(flow.runtime.mode == client::core::RuntimeMode::Options);

    options.playerName = "acceptance";
    options.host = "10.0.0.5";
    options.port = "28002";
    options.windowWidth = "1280";
    options.windowHeight = "720";
    options.targetFps = "144";
    options.interpolationDelay = "3";
    options.debugOverlayDefault = false;
    options.SetSelectedIndex(8);
    interaction.focusedWidget = client::ui::UiWidgetId::OptionsSave;

    runtime.BuildUiState(world);
    world.set<client::ui::UiInputState>({.acceptPressed = true});
    runtime.HandleUiInteraction(world);
    runtime.ProcessRuntimeIntent(world);

    std::string warning;
    const client::ClientConfig persisted = client::core::LoadClientConfigFile(configPath, warning);
    assert(warning.empty());
    assert(flow.statusMessage == "Saved client preferences to " + configPath.string());
    assert(persisted.playerName == "acceptance");
    assert(persisted.serverHost == "10.0.0.5");
    assert(persisted.serverPort == 28002);
    assert(persisted.windowWidth == 1280);
    assert(persisted.windowHeight == 720);
    assert(persisted.targetFps == 144);
    assert(persisted.interpolationDelayTicks == 3);
    assert(!persisted.debugOverlayDefault);
    assert(join.playerName == "acceptance");
    assert(join.host == "10.0.0.5");
    assert(join.port == "28002");

    std::filesystem::remove_all(tempRoot);
}

}  // namespace

int main() {
    TestStartServerLaunchFailureReturnsToMenu();
    TestMenuAndOptionsDocumentsUsePolicyCatalogs();
    TestLocalServerStartupTimeoutReturnsToMenu();
    TestSingleplayerPublishesGameplayState();
    TestOptionsSavePersistsAndRefreshesJoinDefaults();
    return 0;
}
