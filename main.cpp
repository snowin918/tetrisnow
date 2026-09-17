#include <cstdio>
#include <cstdlib>
#include <string>

#include "Engine/GameWindow.h"

namespace
{
void printUsage()
{
    std::fprintf(
        stderr,
        "Usage:\n"
        "  Tetrisnow.exe                     Show the main menu\n"
        "  Tetrisnow.exe --local             Local two-player (same window/keyboard)\n"
        "  Tetrisnow.exe --host [port]       Host a LAN match (default port 7777)\n"
        "  Tetrisnow.exe --join <ip> [port]  Join a host at <ip>[:port]\n");
}

// Parses argv into a NetworkConfig. With no arguments, the app shows the
// main menu instead (skipMenu stays false). Returns false (after printing
// usage) if arguments were given but don't form one of the supported
// shapes.
bool parseArgs(int argc, char** argv, NetworkConfig& config)
{
    if (argc <= 1) {
        return true;
    }

    const std::string mode = argv[1];
    if (mode == "--local") {
        config.role = NetworkRole::Local;
        config.skipMenu = true;
        return true;
    }

    if (mode == "--host") {
        config.role = NetworkRole::Host;
        config.skipMenu = true;
        if (argc >= 3) {
            config.port = static_cast<uint16_t>(std::atoi(argv[2]));
        }
        return true;
    }

    if (mode == "--join") {
        if (argc < 3) {
            printUsage();
            return false;
        }
        config.role = NetworkRole::Client;
        config.skipMenu = true;
        config.hostAddress = argv[2];
        if (argc >= 4) {
            config.port = static_cast<uint16_t>(std::atoi(argv[3]));
        }
        return true;
    }

    printUsage();
    return false;
}
} // namespace

int main(int argc, char** argv)
{
    NetworkConfig networkConfig;
    if (!parseArgs(argc, argv, networkConfig)) {
        return -1;
    }

    GameWindow window(1280, 720, "Tetrisnow", networkConfig);
    if (!window.initialize()) {
        return -1;
    }

    window.run();
    return 0;
}
