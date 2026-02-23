#pragma once
#include "network/PacketDispatcher.h"
#include <entt/entt.hpp>

namespace MMO::Network
{
    // Enregistre le handler Ping
    void RegisterPingHandler(PacketDispatcher& dispatcher, entt::registry& registry);
}
