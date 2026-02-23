#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <unordered_map>
#include "core/Config.h"
#include "core/CommandSystem.h"
#include "world/KingdomWorld.h"
#include "database/ConcurrentQueue.h"
#include "network/NetworkManager.h"
#include "database/DatabaseManager.h"
#include "database/repositories/IAccountRepository.h"
#include "database/repositories/IPlayerRepository.h"


class GameLoop 
{
public:
    explicit GameLoop(const MMO::ServerConfig& config);
    void Run();
    void Stop();

    // Planifie un callback sur le thread principal (thread-safe)
    void EnqueueMainThreadCallback(std::function<void()> callback);

private:
    void ProcessNetworkIn();
    void UpdateLogic(float dt);
    void ProcessNetworkOut();

    // Configure le nettoyage ECS a la deconnexion d'un joueur
    void SetupDisconnectHandler();

    // Charge les royaumes depuis le fichier de configuration
    void LoadKingdoms();

    // Enregistre tous les handlers reseau
    void RegisterHandlers();

    MMO::ServerConfig m_config;
    std::atomic<bool> m_isRunning;
    std::chrono::milliseconds m_tickDuration;

    // Royaumes — chaque monde a sa propre registry ECS
    std::unordered_map<int, std::unique_ptr<MMO::Core::KingdomWorld>> m_kingdoms;

    std::unique_ptr<MMO::Network::NetworkManager> m_networkManager;
    std::shared_ptr<MMO::Database::DatabaseManager> m_dbManager;
    std::shared_ptr<MMO::Database::IAccountRepository> m_accountRepo;
    std::shared_ptr<MMO::Database::IPlayerRepository> m_playerRepo;
    MMO::Core::CommandSystem m_commandSystem;
    MMO::Core::ConcurrentQueue<std::function<void()>> m_mainThreadCallbacks;
};
