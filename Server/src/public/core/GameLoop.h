#pragma once
#include <atomic>
#include <chrono>
#include <entt/entt.hpp>
#include <memory>
#include "core/Config.h"
#include "network/NetworkManager.h"


class GameLoop 
{
public:
    explicit GameLoop(const MMO::ServerConfig& config);
    void Run();
    void Stop();

private:
    void ProcessNetworkIn();
    void UpdateLogic(float dt);
    void ProcessNetworkOut();

    MMO::ServerConfig m_config;
    std::atomic<bool> m_isRunning;
    std::chrono::milliseconds m_tickDuration;
    entt::registry m_registry;
    std::unique_ptr<MMO::Network::NetworkManager> m_networkManager;
};