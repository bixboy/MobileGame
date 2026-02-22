#pragma once
#include <cstdint>


namespace MMO 
{
    struct ServerConfig 
    {
        int tickRate = 20; // Tick Rate
        
        std::uint16_t port = 7777; // Port du server
        
        int maxPlayers = 1000; // Max players
        
        int workerThreads = 4; // Number of threads
    };
}