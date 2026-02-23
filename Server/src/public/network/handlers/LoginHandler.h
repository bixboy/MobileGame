#pragma once
#include "network/PacketDispatcher.h"
#include "network/SessionManager.h"
#include "database/repositories/IAccountRepository.h"
#include "database/repositories/IPlayerRepository.h"
#include <entt/entt.hpp>
#include <memory>
#include <functional>


namespace MMO::Network
{
    // Enregistre le handler de login (Login -> C2S_Login)
    void RegisterLoginHandler(PacketDispatcher& dispatcher, SessionManager& sessionManager,
        std::shared_ptr<Database::IAccountRepository> accountRepo,
        std::function<void(std::function<void()>)> runOnMainThread);
}
