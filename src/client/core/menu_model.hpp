#pragma once

#include <cstdint>
#include <string_view>

namespace client::core {

enum class MenuAction : uint8_t {
    None = 0,
    StartServer,
    JoinServer,
    Singleplayer,
    Options,
    Quit,
};

[[nodiscard]] inline std::string_view MenuActionName(MenuAction action) {
    switch (action) {
    case MenuAction::None:
        return "None";
    case MenuAction::StartServer:
        return "Start Server";
    case MenuAction::JoinServer:
        return "Join Server";
    case MenuAction::Singleplayer:
        return "Singleplayer";
    case MenuAction::Options:
        return "Options";
    case MenuAction::Quit:
        return "Quit";
    }

    return "Unknown";
}

enum class JoinFormField : uint8_t {
    Host = 0,
    Port,
    Name,
    Connect,
    Back,
};

[[nodiscard]] inline std::string_view JoinFormFieldName(JoinFormField field) {
    switch (field) {
    case JoinFormField::Host:
        return "Host";
    case JoinFormField::Port:
        return "Port";
    case JoinFormField::Name:
        return "Name";
    case JoinFormField::Connect:
        return "Connect";
    case JoinFormField::Back:
        return "Back";
    }

    return "Unknown";
}

}  // namespace client::core
