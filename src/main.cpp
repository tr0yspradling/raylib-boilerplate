#include "client/client_entry.hpp"

// Compatibility entrypoint: preserves the historical src/main.cpp path while
// forwarding to the authoritative multiplayer client runtime.
int main(int argc, char** argv) {
    return client::RunClientEntry(argc, argv);
}
