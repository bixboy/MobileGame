#pragma once
#include <string>
#include <entt/entt.hpp>

namespace MMO::Core
{
    // Interface extensible pour les systemes de gameplay
    // Chaque systeme est ticke par son KingdomWorld a chaque frame serveur
    class IGameSystem
    {
    public:
        virtual ~IGameSystem() = default;

        // Appele chaque tick par le KingdomWorld parent
        virtual void OnTick(float dt, entt::registry& registry) = 0;

        // Nom du systeme (pour logs et debug)
        virtual std::string GetName() const = 0;
    };
}
