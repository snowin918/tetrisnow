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
        "  Tetrisnow.exe                     Local two-player (same window/keyboard)\n"
        "  Tetrisnow.exe --host [port]       Host a LAN match (default port 7777)\n"
        "  Tetrisnow.exe --join <ip> [port]  Join a host at <ip>[:port]\n");
}

// Parses argv into a NetworkConfig. Returns false (after printing usage)
// if the arguments don't form one of the three supported shapes.
bool parseArgs(int argc, char** argv, NetworkConfig& config)
{
    if (argc <= 1) {
        config.role = NetworkRole::Local;
        return true;
    }

    const std::string mode = argv[1];
    if (mode == "--host") {
        config.role = NetworkRole::Host;
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
