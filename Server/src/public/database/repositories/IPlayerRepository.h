#pragma once
#include <string>
#include <functional>
#include <optional>


namespace MMO::Database
{
    // Donnees persistantes d'un joueur dans un royaume specifique
    struct PlayerData
    {
        int id = -1;
        int accountId = -1;
        int kingdomId = -1;
        float posX = 0.0f;
        float posY = 0.0f;
        int food  = 500;
        int wood  = 500;
        int stone = 200;
        int gold  = 100;
    };

    // Interface pour l'acces aux donnees joueur
    class IPlayerRepository
    {
    public:
        virtual ~IPlayerRepository() = default;

        // Charge les donnees d'un joueur par (account_id, kingdom_id)
        virtual void GetByAccountAndKingdom(int accountId, int kingdomId,
            std::function<void(std::optional<PlayerData>)> callback) = 0;

        // Cree un nouveau profil joueur dans un royaume
        virtual void Create(int accountId, int kingdomId,
            std::function<void(std::optional<PlayerData>)> callback) = 0;

        // Met a jour les ressources d'un joueur dans un royaume
        virtual void UpdateResources(int accountId, int kingdomId,
            int food, int wood, int stone, int gold) = 0;
    };
}
