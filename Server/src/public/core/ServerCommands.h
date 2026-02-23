#pragma once
#include "core/CommandSystem.h"
#include <string>
#include <functional>


namespace MMO::Core
{
    // Contexte partage pour les commandes console
    struct CommandContext
    {
        std::string dbPath;
        std::function<void()> stopServer;
    };

    // Enregistre toutes les commandes serveur
    void RegisterServerCommands(CommandSystem& commandSystem, CommandContext ctx);
}
