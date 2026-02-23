#include "database/repositories/SqlitePlayerRepository.h"
#include "utils/Logger.h"


namespace MMO::Database
{
    SqlitePlayerRepository::SqlitePlayerRepository(std::shared_ptr<DatabaseManager> dbManager) : m_dbManager(std::move(dbManager))
    {
    }

    // Charge les donnees d'un joueur par (account_id, kingdom_id)
    void SqlitePlayerRepository::GetByAccountAndKingdom(int accountId, int kingdomId, std::function<void(std::optional<PlayerData>)> callback)
    {
        m_dbManager->EnqueueJob([accountId, kingdomId, callback](SQLite::Database& db)
        {
            try
            {
                SQLite::Statement query(db,
                    "SELECT id, account_id, kingdom_id, pos_x, pos_y, food, wood, stone, gold "
                    "FROM player_data WHERE account_id = ? AND kingdom_id = ?");
                query.bind(1, accountId);
                query.bind(2, kingdomId);

                if (query.executeStep())
                {
                    PlayerData data;
                    data.id        = query.getColumn(0).getInt();
                    data.accountId = query.getColumn(1).getInt();
                    data.kingdomId = query.getColumn(2).getInt();
                    data.posX      = static_cast<float>(query.getColumn(3).getDouble());
                    data.posY      = static_cast<float>(query.getColumn(4).getDouble());
                    data.food      = query.getColumn(5).getInt();
                    data.wood      = query.getColumn(6).getInt();
                    data.stone     = query.getColumn(7).getInt();
                    data.gold      = query.getColumn(8).getInt();

                    if (callback)
                        callback(data);
                }
                else
                {
                    if (callback)
                        callback(std::nullopt);
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("SqlitePlayerRepository::GetByAccountAndKingdom erreur: {}", e.what());
                if (callback)
                    callback(std::nullopt);
            }
        });
    }

    // Cree un nouveau profil joueur dans un royaume specifique
    void SqlitePlayerRepository::Create(int accountId, int kingdomId, std::function<void(std::optional<PlayerData>)> callback)
    {
        m_dbManager->EnqueueJob([accountId, kingdomId, callback](SQLite::Database& db)
        {
            try
            {
                // Valeurs de depart pour un nouveau joueur
                PlayerData data;
                data.accountId = accountId;
                data.kingdomId = kingdomId;

                SQLite::Transaction transaction(db);

                SQLite::Statement query(db,
                    "INSERT INTO player_data (account_id, kingdom_id, pos_x, pos_y, food, wood, stone, gold) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
                query.bind(1, accountId);
                query.bind(2, kingdomId);
                query.bind(3, static_cast<double>(data.posX));
                query.bind(4, static_cast<double>(data.posY));
                query.bind(5, data.food);
                query.bind(6, data.wood);
                query.bind(7, data.stone);
                query.bind(8, data.gold);

                query.exec();
                data.id = static_cast<int>(db.getLastInsertRowid());
                transaction.commit();

                LOG_INFO("Profil joueur cree (AccountID: {}, KingdomID: {}, PlayerDataID: {})", accountId, kingdomId, data.id);

                if (callback)
                    callback(data);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("SqlitePlayerRepository::Create erreur: {}", e.what());
                if (callback)
                    callback(std::nullopt);
            }
        });
    }

    // Sauvegarde les ressources en DB (fire-and-forget)
    void SqlitePlayerRepository::UpdateResources(int accountId, int kingdomId, int food, int wood, int stone, int gold)
    {
        m_dbManager->EnqueueJob([accountId, kingdomId, food, wood, stone, gold](SQLite::Database& db)
        {
            try
            {
                SQLite::Statement query(db,
                    "UPDATE player_data SET food = ?, wood = ?, stone = ?, gold = ? "
                    "WHERE account_id = ? AND kingdom_id = ?");
                query.bind(1, food);
                query.bind(2, wood);
                query.bind(3, stone);
                query.bind(4, gold);
                query.bind(5, accountId);
                query.bind(6, kingdomId);
                query.exec();
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("SqlitePlayerRepository::UpdateResources erreur: {}", e.what());
            }
        });
    }
}
