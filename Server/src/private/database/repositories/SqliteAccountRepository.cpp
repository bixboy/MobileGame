#include "database/repositories/SqliteAccountRepository.h"
#include "utils/Logger.h"
#include "utils/PasswordHasher.h"

namespace MMO::Database 
{
    SqliteAccountRepository::SqliteAccountRepository(std::shared_ptr<DatabaseManager> dbManager) : m_dbManager(std::move(dbManager)) 
    {
    }

    // Recherche un compte par nom d'utilisateur
    void SqliteAccountRepository::GetAccountByUsername(const std::string& username, std::function<void(std::optional<Account>)> callback) 
    {
        m_dbManager->EnqueueJob([username, callback](SQLite::Database& db) 
        {
            try 
            {
                SQLite::Statement query(db, "SELECT id, username, password_hash, created_at, last_login_at FROM accounts WHERE username = ?");
                query.bind(1, username);

                if (query.executeStep()) 
                {
                    // Mapping des colonnes vers la structure Account
                    Account acc;
                    acc.id = query.getColumn(0).getInt();
                    acc.username = query.getColumn(1).getString();
                    acc.passwordHash = query.getColumn(2).getString();
                    acc.createdAt = query.getColumn(3).getString();
                    
                    if (!query.getColumn(4).isNull()) 
                        acc.lastLoginAt = query.getColumn(4).getString();
                    
                    if (callback)
                        callback(acc);
                } 
                else 
                {
                    if (callback)
                        callback(std::nullopt);
                }
            } 
            catch (const std::exception& e) 
            {
                LOG_ERROR("SqliteAccountRepository::GetAccountByUsername erreur: {}", e.what());
                if (callback)
                    callback(std::nullopt);
            }
        });
    }

    // Recherche un compte par ID
    void SqliteAccountRepository::GetById(int accountId, std::function<void(std::optional<Account>)> callback)
    {
        m_dbManager->EnqueueJob([accountId, callback](SQLite::Database& db)
        {
            try
            {
                SQLite::Statement query(db, "SELECT id, username, password_hash, created_at, last_login_at FROM accounts WHERE id = ?");
                query.bind(1, accountId);

                if (query.executeStep())
                {
                    Account acc;
                    acc.id = query.getColumn(0).getInt();
                    acc.username = query.getColumn(1).getString();
                    acc.passwordHash = query.getColumn(2).getString();
                    acc.createdAt = query.getColumn(3).getString();
                    if (!query.getColumn(4).isNull())
                        acc.lastLoginAt = query.getColumn(4).getString();

                    if (callback) callback(acc);
                }
                else
                {
                    if (callback) callback(std::nullopt);
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("SqliteAccountRepository::GetById erreur: {}", e.what());
                if (callback) callback(std::nullopt);
            }
        });
    }

    // Cree un nouveau compte
    void SqliteAccountRepository::CreateAccount(const std::string& username, const std::string& passwordHash, std::function<void(bool success, std::optional<Account>)> callback) 
    {
        m_dbManager->EnqueueJob([username, passwordHash, callback](SQLite::Database& db) 
        {
            try 
            {
                // Hashing du mot de passe avec Argon2id
                std::string hashedPassword = Utils::PasswordHasher::Hash(passwordHash);
                if (hashedPassword.empty())
                {
                    LOG_ERROR("Echec du hashing du mot de passe pour: {}", username);
                    if (callback)
                        callback(false, std::nullopt);
                    
                    return;
                }

                // Insertion en transaction
                SQLite::Transaction transaction(db);
                
                SQLite::Statement query(db, "INSERT INTO accounts (username, password_hash) VALUES (?, ?)");
                query.bind(1, username);
                query.bind(2, hashedPassword);
                
                query.exec();
                int newId = static_cast<int>(db.getLastInsertRowid());
                transaction.commit();

                // Retour du compte cree
                Account acc;
                acc.id = newId;
                acc.username = username;
                acc.passwordHash = hashedPassword;
                
                if (callback)
                    callback(true, acc);
            } 
            catch (const std::exception& e) 
            {
                LOG_WARN("Creation de compte echouee: {}", e.what());
                
                if (callback)
                    callback(false, std::nullopt);
            }
        });
    }

    // Met a jour la date de derniere connexion
    void SqliteAccountRepository::UpdateLastLogin(int accountId) 
    {
        m_dbManager->EnqueueJob([accountId](SQLite::Database& db) 
        {
            try 
            {
                SQLite::Statement query(db, "UPDATE accounts SET last_login_at = CURRENT_TIMESTAMP WHERE id = ?");
                query.bind(1, accountId);
                query.exec();
            } 
            catch (const std::exception& e) 
            {
                LOG_ERROR("UpdateLastLogin erreur: {}", e.what());
            }
        });
    }
}
