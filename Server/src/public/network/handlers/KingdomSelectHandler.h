#pragma once
#include "network/PacketDispatcher.h"
#include "network/SessionManager.h"
#include "database/DatabaseManager.h"
#include "database/repositories/IAccountRepository.h"
#include "database/repositories/IPlayerRepository.h"
#include <memory>
#include <functional>
#include <unordered_map>

namespace MMO::Core { class KingdomWorld; }

namespace MMO::Network
{
    // Handler unifie pour la selection de royaume (remplace KingdomHandler + JoinHandler)
    // Sur une meme connexion : RequestKingdoms → KingdomList, SelectKingdom → PlayerData
    void RegisterKingdomSelectHandler(PacketDispatcher& dispatcher, SessionManager& sessionManager,
        std::unordered_map<int, std::unique_ptr<MMO::Core::KingdomWorld>>& kingdoms,
        std::shared_ptr<Database::IAccountRepository> accountRepo,
        std::shared_ptr<Database::IPlayerRepository> playerRepo,
        std::function<void(std::function<void()>)> runOnMainThread);
}
