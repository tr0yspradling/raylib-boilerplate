cmake_minimum_required(VERSION 3.23)
project(raylib_boilerplate LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "")

set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)
option(USE_VENDORED_DEPS "Use dependencies from external directory" ON)

option(BUILD_GAME_CLIENT "Build networked game client" ON)
option(BUILD_GAME_SERVER "Build dedicated authoritative server" ON)
option(BUILD_LEGACY_SCENE_APP "Build old single-player scene app" OFF)

include(CTest)

function(apply_project_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    endif()
endfunction()

set(ARGPARSE_INCLUDE_DIR ${EXTERNAL_DIR}/argparse/include)
if(NOT EXISTS ${ARGPARSE_INCLUDE_DIR}/argparse/argparse.hpp)
    message(FATAL_ERROR "argparse not found. Provide external/argparse.")
endif()

add_library(argparse_vendor INTERFACE)
target_include_directories(argparse_vendor INTERFACE ${ARGPARSE_INCLUDE_DIR})
add_library(argparse::argparse ALIAS argparse_vendor)

set(NEEDS_RAYLIB OFF)
if(BUILD_GAME_CLIENT OR BUILD_LEGACY_SCENE_APP)
    set(NEEDS_RAYLIB ON)
endif()

if(NEEDS_RAYLIB)
    set(RAYLIB_TARGET raylib)
    if(USE_VENDORED_DEPS)
        if(EXISTS ${EXTERNAL_DIR}/raylib/CMakeLists.txt)
            message(STATUS "Using vendored raylib at ${EXTERNAL_DIR}/raylib")
            add_subdirectory(${EXTERNAL_DIR}/raylib)
        else()
            find_package(raylib QUIET)
            if(NOT raylib_FOUND)
                message(FATAL_ERROR "Vendored raylib not found and no system raylib available.")
            endif()
        endif()
    else()
        find_package(raylib REQUIRED)
    endif()

    if(TARGET raylib::raylib)
        set(RAYLIB_TARGET raylib::raylib)
    endif()

    set(RAYLIB_CPP_DIR ${EXTERNAL_DIR}/raylib-cpp)
    if(EXISTS ${RAYLIB_CPP_DIR}/include/raylib-cpp.hpp)
        set(RAYLIB_CPP_HEADER_PATH ${RAYLIB_CPP_DIR}/include)
    elseif(EXISTS ${RAYLIB_CPP_DIR}/raylib-cpp.hpp)
        set(RAYLIB_CPP_HEADER_PATH ${RAYLIB_CPP_DIR})
    else()
        message(FATAL_ERROR "raylib-cpp not found. Provide external/raylib-cpp.")
    endif()

    add_library(raylib_cpp INTERFACE)
    target_include_directories(raylib_cpp INTERFACE ${RAYLIB_CPP_HEADER_PATH})
    target_link_libraries(raylib_cpp INTERFACE ${RAYLIB_TARGET})
endif()

set(GNS_TARGET)
if(BUILD_GAME_CLIENT OR BUILD_GAME_SERVER)
    if(USE_VENDORED_DEPS AND EXISTS ${EXTERNAL_DIR}/GameNetworkingSockets/CMakeLists.txt)
        set(BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
        set(BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
        set(ENABLE_ICE OFF CACHE BOOL "" FORCE)
        set(USE_STEAMWEBRTC OFF CACHE BOOL "" FORCE)
        add_subdirectory(${EXTERNAL_DIR}/GameNetworkingSockets EXCLUDE_FROM_ALL)
    else()
        find_package(GameNetworkingSockets CONFIG REQUIRED)
    endif()

    if(TARGET GameNetworkingSockets::static)
        set(GNS_TARGET GameNetworkingSockets::static)
    elseif(TARGET GameNetworkingSockets::GameNetworkingSockets_s)
        set(GNS_TARGET GameNetworkingSockets::GameNetworkingSockets_s)
    elseif(TARGET GameNetworkingSockets_s)
        set(GNS_TARGET GameNetworkingSockets_s)
    else()
        message(FATAL_ERROR "Could not find a usable static GameNetworkingSockets target.")
    endif()
endif()

if(BUILD_LEGACY_SCENE_APP)
    set(IMGUI_DIR ${EXTERNAL_DIR}/imgui)
    if(NOT EXISTS ${IMGUI_DIR}/imgui.cpp)
        message(FATAL_ERROR "ImGui not found for legacy app")
    endif()

    add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
    )
    target_include_directories(imgui PUBLIC ${IMGUI_DIR})

    if(EXISTS ${IMGUI_DIR}/imgui_demo.cpp)
        target_sources(imgui PRIVATE ${IMGUI_DIR}/imgui_demo.cpp)
    endif()

    set(RLIMGUI_DIR ${EXTERNAL_DIR}/rlImGui)
    add_library(rlImGui STATIC ${RLIMGUI_DIR}/rlImGui.cpp)
    target_include_directories(rlImGui PUBLIC ${RLIMGUI_DIR} ${IMGUI_DIR})
    target_link_libraries(rlImGui PUBLIC ${RAYLIB_TARGET} imgui)

    set(FLECS_DIR ${EXTERNAL_DIR}/flecs)
    if(EXISTS ${FLECS_DIR}/flecs.c AND EXISTS ${FLECS_DIR}/flecs.h)
        add_library(flecs STATIC ${FLECS_DIR}/flecs.c)
        target_include_directories(flecs PUBLIC ${FLECS_DIR})
    elseif(EXISTS ${FLECS_DIR}/CMakeLists.txt)
        add_subdirectory(${FLECS_DIR})
    else()
        find_package(flecs CONFIG REQUIRED)
    endif()

    set(FLECS_LIB flecs)
    if(TARGET flecs::flecs)
        set(FLECS_LIB flecs::flecs)
    elseif(TARGET flecs_static)
        set(FLECS_LIB flecs_static)
    endif()
endif()

add_library(shared_game INTERFACE)
target_include_directories(shared_game INTERFACE ${CMAKE_SOURCE_DIR}/src)

if(BUILD_GAME_CLIENT OR BUILD_GAME_SERVER)
    add_library(shared_net STATIC
        src/shared/net/transport_gns.cpp
        src/shared/net/transport.hpp
        src/shared/net/transport_gns.hpp
        src/shared/net/serializer.hpp
        src/shared/net/protocol.hpp
        src/shared/net/message_ids.hpp
        src/shared/net/lanes.hpp
        src/shared/net/send_policy.hpp
        src/shared/net/snapshot.hpp
        src/shared/net/delta.hpp
        src/shared/net/auth.hpp
    )
    target_include_directories(shared_net PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(shared_net PUBLIC shared_game ${GNS_TARGET})
    apply_project_warnings(shared_net)
endif()

if(BUILD_GAME_CLIENT)
    add_library(client_runtime STATIC
        src/client/client_entry.cpp
        src/client/client_entry.hpp
        src/client/game_client.cpp
        src/client/game_client.hpp
        src/client/components/components.hpp
        src/client/core/application.hpp
        src/client/core/config.hpp
        src/client/core/menu_model.hpp
        src/client/core/runtime_state.hpp
        src/client/core/scene.hpp
        src/client/core/scene_manager.hpp
        src/client/input/input_manager.hpp
        src/client/physics/movement_system.hpp
        src/client/scenes/menu_scene.hpp
        src/client/scenes/sandbox_scene.hpp
        src/client/scenes/splash_scene.hpp
        src/client/systems/render_system.hpp
        src/client/ui/debug_overlay.hpp
    )
    target_include_directories(client_runtime PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(client_runtime PUBLIC shared_game shared_net raylib_cpp argparse::argparse)
    apply_project_warnings(client_runtime)

    add_executable(game_client src/client/main.cpp)
    target_include_directories(game_client PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(game_client PRIVATE client_runtime)
    apply_project_warnings(game_client)

    # Backward-compatible build target name without producing a duplicate runtime binary.
    add_custom_target(raylib_boilerplate DEPENDS game_client)

    set_target_properties(game_client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

    if(APPLE)
        target_link_libraries(game_client PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(game_client PRIVATE m pthread GL dl X11)
    endif()
endif()

if(BUILD_GAME_SERVER)
    add_library(server_runtime STATIC
        src/server/game_server.cpp
        src/server/game_server.hpp
        src/server/world_persistence.cpp
        src/server/world_persistence.hpp
        src/server/config/server_config.cpp
        src/server/config/server_config.hpp
    )
    target_include_directories(server_runtime PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(server_runtime PUBLIC shared_game shared_net)
    apply_project_warnings(server_runtime)

    add_executable(game_server src/server/main.cpp)
    target_include_directories(game_server PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(game_server PRIVATE server_runtime)
    apply_project_warnings(game_server)
    set_target_properties(game_server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

if(BUILD_LEGACY_SCENE_APP)
    add_executable(legacy_singleplayer src/main.cpp)
    target_include_directories(legacy_singleplayer PRIVATE ${CMAKE_SOURCE_DIR}/src ${IMGUI_DIR} ${RLIMGUI_DIR})
    target_link_libraries(legacy_singleplayer PRIVATE raylib_cpp rlImGui ${FLECS_LIB})
    apply_project_warnings(legacy_singleplayer)

    if(APPLE)
        target_link_libraries(legacy_singleplayer PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(legacy_singleplayer PRIVATE m pthread GL dl X11)
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
