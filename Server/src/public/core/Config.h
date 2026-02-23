#pragma once
#include <string>
#include <cstdint>


namespace MMO
{
    // Configuration globale du serveur
    struct ServerConfig 
    {
        int tickRate = 20;
        std::uint16_t port = 7777;
        int maxPlayers = 1000;
        std::string kingdomsConfigPath = "kingdoms.json";    // Chemin du fichier de config des royaumes
        std::string dbPath = "game.db";                      // Chemin de la base de donnees
    };
}