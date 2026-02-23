#pragma once
#include <string>
#include <vector>
#include <memory>
#include <entt/entt.hpp>
#include "world/IGameSystem.h"
#include "world/SpatialGrid.h"


namespace MMO::Core
{
    // Un royaume = un monde autonome avec sa propre registry ECS et ses systemes de jeu
    class KingdomWorld
    {
    public:
        KingdomWorld(int id, const std::string& name);

        // Tick tous les systemes enregistres
        void OnTick(float dt);

        // Enregistre un systeme de gameplay (movement, combat, production...)
        void AddSystem(std::unique_ptr<IGameSystem> system);

        // Accesseurs
        int GetId() const { return m_id; }
        const std::string& GetName() const { return m_name; }
        entt::registry& GetRegistry() { return m_registry; }
        const entt::registry& GetRegistry() const { return m_registry; }
        SpatialGrid& GetSpatialGrid() { return m_spatialGrid; }

    private:
        int m_id;
        std::string m_name;
        entt::registry m_registry;
        SpatialGrid m_spatialGrid;
        std::vector<std::unique_ptr<IGameSystem>> m_systems;
    };
}
