#pragma once

#include <array>
#include <cstdio>

#include <raylib-cpp.hpp>

#include "client/components/components.hpp"

namespace client::ui {

class DebugOverlay {
public:
    static void Draw(const components::NetworkDebugState& state) {
        DrawRectangleRounded({12.0f, 10.0f, 860.0f, 158.0f}, 0.14f, 12, raylib::Color{0, 0, 0, 140});

        std::array<char, 256> lineOne{};
        std::array<char, 256> lineTwo{};
        std::array<char, 256> lineThree{};
        std::array<char, 256> lineFour{};
        std::array<char, 256> lineFive{};

        std::snprintf(lineOne.data(), lineOne.size(), "Scene: %s | Net: %s%s", state.sceneName.c_str(),
                      state.connected ? "connected" : (state.connecting ? "connecting" : "disconnected"),
                      state.welcomed ? " (welcomed)" : "");
        std::snprintf(lineTwo.data(), lineTwo.size(), "Tick client=%u server=%u pending=%u", state.clientTick,
                      state.serverTick, static_cast<unsigned int>(state.pendingInputCount));

        if (state.metrics.has_value()) {
            std::snprintf(lineThree.data(), lineThree.size(),
                          "Ping=%dms in=%.1fKB/s out=%.1fKB/s queue=%dB quality=%.2f/%.2f", state.metrics->pingMs,
                          state.metrics->inBytesPerSec / 1024.0f, state.metrics->outBytesPerSec / 1024.0f,
                          state.metrics->pendingReliableBytes + state.metrics->pendingUnreliableBytes,
                          state.metrics->qualityLocal, state.metrics->qualityRemote);
        } else {
            std::snprintf(lineThree.data(), lineThree.size(), "Ping=-- in=-- out=--");
        }

        std::snprintf(lineFour.data(), lineFour.size(), "Disconnect: %s",
                      state.disconnectReason.empty() ? "none" : state.disconnectReason.c_str());
        std::snprintf(lineFive.data(), lineFive.size(), "Chunks loaded=%u version_conflicts=%u",
                      static_cast<unsigned int>(state.loadedChunkCount), state.chunkVersionConflicts);

        raylib::Color{245, 250, 255, 255}.DrawText(lineOne.data(), 22, 18, 20);
        raylib::Color{195, 215, 232, 255}.DrawText(lineTwo.data(), 22, 46, 20);
        raylib::Color{168, 198, 220, 255}.DrawText(lineThree.data(), 22, 74, 20);
        raylib::Color{255, 160, 130, 255}.DrawText(lineFour.data(), 22, 102, 18);
        raylib::Color{180, 220, 180, 255}.DrawText(lineFive.data(), 22, 126, 18);
    }
};

}  // namespace client::ui
