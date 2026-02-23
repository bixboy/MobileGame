#pragma once
#include "network/PacketDispatcher.h"
#include "network/SessionManager.h"
#include "database/repositories/IPlayerRepository.h"
#include <entt/entt.hpp>
#include <memory>
#include <unordered_map>

namespace MMO::Core { class KingdomWorld; }

namespace MMO::Network
{
    // Enregistre le handler de modification des ressources
    void RegisterResourceHandler(PacketDispatcher& dispatcher, SessionManager& sessionManager,
        std::unordered_map<int, std::unique_ptr<MMO::Core::KingdomWorld>>& kingdoms,
        std::shared_ptr<Database::IPlayerRepository> playerRepo);
}
