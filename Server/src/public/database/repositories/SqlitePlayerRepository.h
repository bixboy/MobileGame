#pragma once
#include "database/repositories/IPlayerRepository.h"
#include "database/DatabaseManager.h"
#include <memory>


namespace MMO::Database
{
    // Implementation SQLite du repository joueur
    class SqlitePlayerRepository : public IPlayerRepository
    {
    public:
        explicit SqlitePlayerRepository(std::shared_ptr<DatabaseManager> dbManager);
        ~SqlitePlayerRepository() override = default;

        void GetByAccountAndKingdom(int accountId, int kingdomId,
            std::function<void(std::optional<PlayerData>)> callback) override;

        void Create(int accountId, int kingdomId,
            std::function<void(std::optional<PlayerData>)> callback) override;

        void UpdateResources(int accountId, int kingdomId,
            int food, int wood, int stone, int gold) override;

    private:
        std::shared_ptr<DatabaseManager> m_dbManager;
    };
}
