#pragma once
#include <cstdint>
#include <entt/entt.hpp>


namespace MMO 
{
    // Alias de types globaux du serveur
    using PlayerID = std::uint64_t;
    using EntityID = entt::entity;
    using PacketSize = std::uint16_t;

    // Sentinelles pour les valeurs invalides
    constexpr EntityID INVALID_ENTITY = entt::null;
    constexpr PlayerID INVALID_PLAYER = 0;
}