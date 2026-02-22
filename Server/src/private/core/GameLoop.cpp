#include "core/GameLoop.h"
#include "utils/Logger.h"
#include "utils/Time.h"
#include "NetworkCore_generated.h"
#include "network/Handlers.h"
#include <thread>


GameLoop::GameLoop(const MMO::ServerConfig& config) : m_config(config), m_isRunning(false), m_tickDuration(1000 / config.tickRate) 
{
}

void GameLoop::Run() 
{
    m_isRunning = true;
    m_networkManager = std::make_unique<MMO::Network::NetworkManager>();
    
    if (!m_networkManager->Initialize(m_config))
    {
        LOG_ERROR("Echec de l'initialisation du NetworkManager. Arrêt du serveur.");
        return;
    }

    // Enregistrement de tous les handlers
    MMO::Network::RegisterAllNetworkHandlers(m_networkManager->GetDispatcher(), m_registry);
    
    LOG_INFO("Demarrage du Serveur (Tickrate: {}, Port: {})", m_config.tickRate, m_config.port);

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
    if (m_networkManager)
    {
        m_networkManager->Shutdown();
    }
}

void GameLoop::ProcessNetworkIn() 
{ 
    if (m_networkManager)
    {
        m_networkManager->ProcessEvents();
    }
}

void GameLoop::UpdateLogic(float dt) { /* TODO */ }

void GameLoop::ProcessNetworkOut() { /* TODO */ }