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
                SQLite::Statement query(db, "SELECT id, username, password_hash, device_id, created_at, last_login_at FROM accounts WHERE username = ?");
                query.bind(1, username);

                if (query.executeStep()) 
                {
                    // Mapping des colonnes vers la structure Account
                    Account acc;
                    acc.id = query.getColumn(0).getInt();
                    acc.username = query.getColumn(1).getString();
                    acc.passwordHash = !query.getColumn(2).isNull() ? query.getColumn(2).getString() : "";
                    acc.deviceId = !query.getColumn(3).isNull() ? query.getColumn(3).getString() : "";
                    acc.createdAt = query.getColumn(4).getString();
                    
                    if (!query.getColumn(5).isNull()) 
                        acc.lastLoginAt = query.getColumn(5).getString();
                    
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
                SQLite::Statement query(db, "SELECT id, username, password_hash, device_id, created_at, last_login_at FROM accounts WHERE id = ?");
                query.bind(1, accountId);

                if (query.executeStep())
                {
                    Account acc;
                    acc.id = query.getColumn(0).getInt();
                    acc.username = query.getColumn(1).getString();
                    acc.passwordHash = !query.getColumn(2).isNull() ? query.getColumn(2).getString() : "";
                    acc.deviceId = !query.getColumn(3).isNull() ? query.getColumn(3).getString() : "";
                    acc.createdAt = query.getColumn(4).getString();
                    if (!query.getColumn(5).isNull())
                        acc.lastLoginAt = query.getColumn(5).getString();

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

    // Recherche un compte par Device ID
    void SqliteAccountRepository::GetAccountByDeviceId(const std::string& deviceId, std::function<void(std::optional<Account>)> callback)
    {
        m_dbManager->EnqueueJob([deviceId, callback](SQLite::Database& db)
        {
            try
            {
                SQLite::Statement query(db, "SELECT id, username, password_hash, device_id, created_at, last_login_at FROM accounts WHERE device_id = ?");
                query.bind(1, deviceId);

                if (query.executeStep())
                {
                    Account acc;
                    acc.id = query.getColumn(0).getInt();
                    acc.username = query.getColumn(1).getString();
                    acc.passwordHash = !query.getColumn(2).isNull() ? query.getColumn(2).getString() : "";
                    acc.deviceId = !query.getColumn(3).isNull() ? query.getColumn(3).getString() : "";
                    acc.createdAt = query.getColumn(4).getString();
                    if (!query.getColumn(5).isNull())
                        acc.lastLoginAt = query.getColumn(5).getString();

                    if (callback) callback(acc);
                }
                else
                {
                    if (callback) callback(std::nullopt);
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("SqliteAccountRepository::GetAccountByDeviceId erreur: {}", e.what());
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

    // Cree un compte invité
    void SqliteAccountRepository::CreateGuestAccount(const std::string& deviceId, const std::string& username, std::function<void(bool success, std::optional<Account>)> callback) 
    {
        m_dbManager->EnqueueJob([deviceId, username, callback](SQLite::Database& db) 
        {
            try 
            {
                // Insertion en transaction
                SQLite::Transaction transaction(db);
                
                SQLite::Statement query(db, "INSERT INTO accounts (username, device_id) VALUES (?, ?)");
                query.bind(1, username);
                query.bind(2, deviceId);
                
                query.exec();
                int newId = static_cast<int>(db.getLastInsertRowid());
                transaction.commit();

                // Retour du compte cree
                Account acc;
                acc.id = newId;
                acc.username = username;
                acc.passwordHash = "";
                acc.deviceId = deviceId;
                
                if (callback)
                    callback(true, acc);
            } 
            catch (const std::exception& e) 
            {
                LOG_WARN("Creation de compte invite echouee: {}", e.what());
                
                if (callback)
                    callback(false, std::nullopt);
            }
        });
    }

    // Lie un compte invite a des identifiants classiques
    void SqliteAccountRepository::BindAccount(int accountId, const std::string& username, const std::string& passwordHash, std::function<void(bool success)> callback)
    {
        m_dbManager->EnqueueJob([accountId, username, passwordHash, callback](SQLite::Database& db)
        {
            try
            {
                SQLite::Statement query(db, "UPDATE accounts SET username = ?, password_hash = ? WHERE id = ?");
                query.bind(1, username);
                query.bind(2, passwordHash);
                query.bind(3, accountId);

                int rowsModified = query.exec();
                
                if (rowsModified > 0)
                {
                    LOG_INFO("Compte {} lie avec succes au pseudo '{}'", accountId, username);
                    if (callback) callback(true);
                }
                else
                {
                    LOG_WARN("Echec de la liaison de compte pour l'ID {} (compte non trouve)", accountId);
                    if (callback) callback(false);
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("SqliteAccountRepository::BindAccount erreur (pseudo potentiellement deja pris) : {}", e.what());
                if (callback) callback(false);
            }
        });
    }

    // Lie un compte a un fournisseur externe (Google, Apple...)
    void SqliteAccountRepository::BindSocialAccount(int accountId, const std::string& provider, const std::string& providerId, std::function<void(bool success)> callback)
    {
        m_dbManager->EnqueueJob([accountId, provider, providerId, callback](SQLite::Database& db)
        {
            try
            {
                SQLite::Statement query(db, "INSERT INTO account_bindings (account_id, auth_provider, auth_provider_id) VALUES (?, ?, ?)");
                query.bind(1, accountId);
                query.bind(2, provider);
                query.bind(3, providerId);

                query.exec();
                
                LOG_INFO("Compte {} lie avec succes au fournisseur '{}' (ID: {})", accountId, provider, providerId);
                if (callback) callback(true);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("SqliteAccountRepository::BindSocialAccount erreur (liaison potentiellement existante) : {}", e.what());
                if (callback) callback(false);
            }
        });
    }

    // Cherche un compte par fournisseur (Google, Apple) et son ID
    void SqliteAccountRepository::GetAccountBySocialId(const std::string& provider, const std::string& providerId, std::function<void(std::optional<Account>)> callback)
    {
        m_dbManager->EnqueueJob([provider, providerId, callback](SQLite::Database& db)
        {
            try
            {
                SQLite::Statement query(db, 
                    "SELECT a.id, a.username, a.password_hash, a.device_id, a.created_at, a.last_login_at "
                    "FROM accounts a "
                    "JOIN account_bindings b ON a.id = b.account_id "
                    "WHERE b.auth_provider = ? AND b.auth_provider_id = ?");
                
                query.bind(1, provider);
                query.bind(2, providerId);

                if (query.executeStep())
                {
                    Account acc;
                    acc.id = query.getColumn(0).getInt();
                    acc.username = query.getColumn(1).getString();
                    acc.passwordHash = query.getColumn(2).isNull() ? "" : query.getColumn(2).getString();
                    acc.deviceId = query.getColumn(3).isNull() ? "" : query.getColumn(3).getString();
                    acc.createdAt = query.getColumn(4).getString();
                    acc.lastLoginAt = query.getColumn(5).isNull() ? "" : query.getColumn(5).getString();
                    
                    if (callback) callback(acc);
                }
                else
                {
                    if (callback) callback(std::nullopt);
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("SqliteAccountRepository::GetAccountBySocialId erreur: {}", e.what());
                if (callback) callback(std::nullopt);
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
