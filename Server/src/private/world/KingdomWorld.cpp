#include "world/KingdomWorld.h"
#include "utils/Logger.h"


namespace MMO::Core
{
    KingdomWorld::KingdomWorld(int id, const std::string& name)
        : m_id(id), m_name(name)
    {
        LOG_INFO("Royaume '{}' (ID: {}) cree.", m_name, m_id);
    }

    void KingdomWorld::OnTick(float dt)
    {
        for (auto& system : m_systems)
        {
            system->OnTick(dt, m_registry);
        }
    }

    void KingdomWorld::AddSystem(std::unique_ptr<IGameSystem> system)
    {
        LOG_INFO("Royaume '{}': systeme '{}' enregistre.", m_name, system->GetName());
        m_systems.push_back(std::move(system));
    }
}
