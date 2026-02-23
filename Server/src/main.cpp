#include "core/GameLoop.h"
#include "core/Config.h"
#include "utils/Logger.h"
#include <csignal>
#include <sodium.h>
#include <string>
#include <vector>

static GameLoop* g_serverLoop = nullptr;

void SignalHandler(int signal)
{
    LOG_INFO("Signal recu ({}). Arret du serveur...", signal);
    if (g_serverLoop)
    {
        g_serverLoop->Stop();
    }
}

// Parse les arguments en ligne de commande
static MMO::ServerConfig ParseArgs(int argc, char* argv[])
{
    MMO::ServerConfig config;
    std::vector<std::string> args(argv + 1, argv + argc);

    for (size_t i = 0; i < args.size(); ++i)
    {
        if (args[i] == "--port" && i + 1 < args.size())
        {
            config.port = static_cast<std::uint16_t>(std::stoi(args[++i]));
        }
        else if (args[i] == "--db" && i + 1 < args.size())
        {
            config.dbPath = args[++i];
        }
        else if (args[i] == "--kingdoms-config" && i + 1 < args.size())
        {
            config.kingdomsConfigPath = args[++i];
        }
        else if (args[i] == "--tick-rate" && i + 1 < args.size())
        {
            config.tickRate = std::stoi(args[++i]);
        }
        else if (args[i] == "--max-players" && i + 1 < args.size())
        {
            config.maxPlayers = std::stoi(args[++i]);
        }
    }

    return config;
}

int main(int argc, char* argv[]) 
{
    if (sodium_init() < 0)
    {
        LOG_ERROR("Impossible d'initialiser libsodium. Arret.");
        return 1;
    }
    LOG_INFO("libsodium initialise.");

    MMO::ServerConfig config = ParseArgs(argc, argv);

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    LOG_INFO("Port: {} | TickRate: {} | DB: {}", config.port, config.tickRate, config.dbPath);

    GameLoop serverLoop(config);
    g_serverLoop = &serverLoop;

    serverLoop.Run();

    g_serverLoop = nullptr;
    return 0;
}