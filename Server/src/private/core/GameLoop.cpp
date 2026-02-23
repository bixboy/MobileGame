#include "core/GameLoop.h"
#include "world/KingdomRegistry.h"
#include "core/ServerCommands.h"
#include "network/handlers/PingHandler.h"
#include "network/handlers/LoginHandler.h"
#include "network/handlers/ResourceHandler.h"
#include "network/handlers/KingdomSelectHandler.h"
#include "utils/Logger.h"
#include "utils/Time.h"
#include "database/repositories/SqliteAccountRepository.h"
#include "database/repositories/SqlitePlayerRepository.h"
#include <thread>


GameLoop::GameLoop(const MMO::ServerConfig& config) : m_config(config), m_isRunning(false), m_tickDuration(1000 / config.tickRate) 
{
}

void GameLoop::Run() 
{
    m_isRunning = true;

    // --- Initialisation du reseau ---
    m_networkManager = std::make_unique<MMO::Network::NetworkManager>();
    if (!m_networkManager->Initialize(m_config))
    {
        LOG_ERROR("Echec de l'initialisation du NetworkManager. Arret du serveur.");
        return;
    }

    // --- Initialisation de la base de donnees ---
    m_dbManager = std::make_shared<MMO::Database::DatabaseManager>();
    if (!m_dbManager->Initialize(m_config.dbPath))
    {
        LOG_ERROR("Echec de l'initialisation de la Base de Donnees. Arret du serveur.");
        return;
    }

    m_accountRepo = std::make_shared<MMO::Database::SqliteAccountRepository>(m_dbManager);
    m_playerRepo = std::make_shared<MMO::Database::SqlitePlayerRepository>(m_dbManager);

    // Nettoyage ECS a la deconnexion
    SetupDisconnectHandler();

    // --- Chargement des royaumes ---
    LoadKingdoms();

    // --- Enregistrement de tous les handlers ---
    RegisterHandlers();

    LOG_INFO("Demarrage du Serveur (Tickrate: {}, Port: {}, Royaumes: {})",
        m_config.tickRate, m_config.port, m_kingdoms.size());

    // Demarrage du systeme de commandes console
    MMO::Core::CommandContext cmdCtx{
        m_config.dbPath,
        [this]() { Stop(); }
    };
    MMO::Core::RegisterServerCommands(m_commandSystem, cmdCtx);
    m_commandSystem.Start();

    // --- Boucle principale a tick fixe ---
    auto next_tick_time = std::chrono::steady_clock::now();
    MMO::Time::Stopwatch tickTimer;
    const float dt = 1.0f / static_cast<float>(m_config.tickRate);

    while (m_isRunning) 
    {
        tickTimer.Reset();

        ProcessNetworkIn();
        UpdateLogic(dt);
        ProcessNetworkOut();

        float timeTaken = tickTimer.ElapsedMilliseconds();
        if (timeTaken > static_cast<float>(m_tickDuration.count())) 
        {
            LOG_WARN("Serveur en surcharge ! Le tick a pris : {:.2f} ms", timeTaken);
        }

        next_tick_time += m_tickDuration;
        auto now = std::chrono::steady_clock::now();
        auto sleep_time = next_tick_time - now;

        if (sleep_time > std::chrono::milliseconds::zero())
        {
            auto os_sleep_time = sleep_time - std::chrono::milliseconds(2);
            if (os_sleep_time > std::chrono::milliseconds::zero()) 
            {
                std::this_thread::sleep_for(os_sleep_time);
            }
            while (std::chrono::steady_clock::now() < next_tick_time) 
            {
                std::this_thread::yield();
            }
        } 
        else
        {
            LOG_ERROR("Tick saute ! La boucle n'a pas pu se reposer.");
            next_tick_time = std::chrono::steady_clock::now();
        }
    }
    
    LOG_INFO("Game Loop arretee proprement.");
}

void GameLoop::Stop() 
{ 
    m_isRunning = false;
    m_commandSystem.Stop();
    if (m_networkManager)
    {
        m_networkManager->Shutdown();
    }
}

void GameLoop::EnqueueMainThreadCallback(std::function<void()> callback)
{
    m_mainThreadCallbacks.Push(std::move(callback));
}

void GameLoop::LoadKingdoms()
{
    MMO::Core::KingdomRegistry kingdomRegistry;
    if (!kingdomRegistry.LoadFromFile(m_config.kingdomsConfigPath))
    {
        LOG_WARN("Impossible de charger le fichier royaumes: {}. Creation d'un royaume par defaut.", m_config.kingdomsConfigPath);
        m_kingdoms[1] = std::make_unique<MMO::Core::KingdomWorld>(1, "Royaume Principal");
        return;
    }

    for (const auto& entry : kingdomRegistry.GetAll())
    {
        m_kingdoms[entry.id] = std::make_unique<MMO::Core::KingdomWorld>(entry.id, entry.name);
    }

    if (m_kingdoms.empty())
    {
        LOG_WARN("Aucun royaume charge. Creation d'un royaume par defaut.");
        m_kingdoms[1] = std::make_unique<MMO::Core::KingdomWorld>(1, "Royaume Principal");
    }

    LOG_INFO("{} royaume(s) charge(s).", m_kingdoms.size());
}

void GameLoop::RegisterHandlers()
{
    auto& dispatcher = m_networkManager->GetDispatcher();
    auto& sessionManager = m_networkManager->GetSessionManager();

    // Dummy registry pour le ping (ne l'utilise pas vraiment)
    // TODO: Supprimer le parametre registry de RegisterPingHandler
    static entt::registry dummyRegistry;

    auto runOnMainThread = [this](std::function<void()> cb) { EnqueueMainThreadCallback(std::move(cb)); };

    MMO::Network::RegisterPingHandler(dispatcher, dummyRegistry);
    MMO::Network::RegisterLoginHandler(dispatcher, sessionManager, m_accountRepo, runOnMainThread);
    MMO::Network::RegisterKingdomSelectHandler(dispatcher, sessionManager, m_kingdoms,
        m_accountRepo, m_playerRepo, runOnMainThread);
    MMO::Network::RegisterResourceHandler(dispatcher, sessionManager, m_kingdoms, m_playerRepo);

    LOG_INFO("Handlers reseau enregistres (Ping, Login, KingdomSelect, Resource)");
}

void GameLoop::SetupDisconnectHandler()
{
    m_networkManager->GetSessionManager().SetDisconnectCallback(
        [this](const MMO::Network::PlayerSession& session)
        {
            if (session.entityID != MMO::INVALID_ENTITY && session.kingdomId >= 0)
            {
                EnqueueMainThreadCallback([this, entityID = session.entityID,
                    playerID = session.playerID, kingdomId = session.kingdomId]()
                {
                    auto it = m_kingdoms.find(kingdomId);
                    if (it != m_kingdoms.end())
                    {
                        auto& registry = it->second->GetRegistry();
                        if (registry.valid(entityID))
                        {
                            it->second->GetSpatialGrid().Remove(entityID);
                            registry.destroy(entityID);
                            LOG_INFO("Entite ECS detruite pour le joueur {} dans le royaume {}",
                                playerID, kingdomId);
                        }
                    }
                });
            }
        });
}

void GameLoop::ProcessNetworkIn() 
{ 
    if (m_networkManager)
    {
        m_networkManager->ProcessEvents();
    }
}

void GameLoop::UpdateLogic(float dt) 
{
    // Traitement des callbacks main thread
    while (auto callbackOpt = m_mainThreadCallbacks.TryPop())
    {
        if (callbackOpt.value())
        {
            callbackOpt.value()();
        }
    }

    // Tick de chaque royaume
    for (auto& [id, world] : m_kingdoms)
    {
        world->OnTick(dt);
    }

    // Traitement des commandes console
    m_commandSystem.ProcessPending();
}

void GameLoop::ProcessNetworkOut() { /* TODO: batched broadcasts */ }