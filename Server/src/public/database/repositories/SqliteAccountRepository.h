#pragma once
#include "database/repositories/IAccountRepository.h"
#include "database/DatabaseManager.h"
#include <memory>


namespace MMO::Database 
{
    // Implementation SQLite du repository de comptes
    class SqliteAccountRepository : public IAccountRepository 
    {
    public:
        explicit SqliteAccountRepository(std::shared_ptr<DatabaseManager> dbManager);
        ~SqliteAccountRepository() override = default;

        void GetAccountByUsername(const std::string& username, std::function<void(std::optional<Account>)> callback) override;
        void GetById(int accountId, std::function<void(std::optional<Account>)> callback) override;
        void GetAccountByDeviceId(const std::string& deviceId, std::function<void(std::optional<Account>)> callback) override;
        void CreateAccount(const std::string& username, const std::string& passwordHash, std::function<void(bool success, std::optional<Account>)> callback) override;
        void CreateGuestAccount(const std::string& deviceId, const std::string& username, std::function<void(bool success, std::optional<Account>)> callback) override;
        void BindAccount(int accountId, const std::string& username, const std::string& passwordHash, std::function<void(bool success)> callback) override;
        void BindSocialAccount(int accountId, const std::string& provider, const std::string& providerId, std::function<void(bool success)> callback) override;
        void GetAccountBySocialId(const std::string& provider, const std::string& providerId, std::function<void(std::optional<Account>)> callback) override;
        void UpdateLastLogin(int accountId) override;

    private:
        std::shared_ptr<DatabaseManager> m_dbManager;
    };
}
