#pragma once
#include <entt/entt.hpp>
#include <string>
#include "core/Types.h"


namespace MMO::ECS
{
    // Identite du joueur
    struct PlayerInfoComponent
    {
        PlayerID playerID = INVALID_PLAYER;
        int accountID = -1;
        std::string username;
    };

    // Position sur la carte du monde
    struct PositionComponent
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    // Ressources du joueur
    struct ResourcesComponent
    {
        int food  = 500;
        int wood  = 500;
        int stone = 200;
        int gold  = 100;
    };
}
