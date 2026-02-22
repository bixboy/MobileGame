#pragma once
#include "network/PacketDispatcher.h"
#include <entt/entt.hpp>


namespace MMO::Network 
{
    void RegisterAllNetworkHandlers(PacketDispatcher& dispatcher, entt::registry& registry);
}
