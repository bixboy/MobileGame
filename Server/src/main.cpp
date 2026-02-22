#include "core/GameLoop.h"
#include "core/Config.h"

int main() 
{
    MMO::ServerConfig config;
    config.tickRate = 20;
    
    GameLoop serverLoop(config);
    serverLoop.Run();
    return 0;
}