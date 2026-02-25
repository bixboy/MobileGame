#pragma once
#include <string>
#include <functional>
#include <optional>


namespace MMO::Database 
{
    // Modele de donnees d'un compte joueur
    struct Account 
    {
        int id;
        std::string username;
        std::string passwordHash;
        std::string deviceId;
        std::string createdAt;
        std::string lastLoginAt;
    };
    
    // Interface pour l'acces aux comptes (permet de changer de SGBD)
    class IAccountRepository 
    {
    public:
        virtual ~IAccountRepository() = default;
        
        // Recherche un compte par nom d'utilisateur (async)
        virtual void GetAccountByUsername(const std::string& username, std::function<void(std::optional<Account>)> callback) = 0;
        
        // Recherche un compte par ID (async)
        virtual void GetById(int accountId, std::function<void(std::optional<Account>)> callback) = 0;
        
        // Recherche un compte invité par son Device ID (async)
        virtual void GetAccountByDeviceId(const std::string& deviceId, std::function<void(std::optional<Account>)> callback) = 0;
        
        // Cree un nouveau compte classique (async)
        virtual void CreateAccount(const std::string& username, const std::string& passwordHash, std::function<void(bool success, std::optional<Account>)> callback) = 0;
        
        // Cree un compte invité (async)
        virtual void CreateGuestAccount(const std::string& deviceId, const std::string& username, std::function<void(bool success, std::optional<Account>)> callback) = 0;

        // Lie un compte invite a des identifiants classiques (async)
        virtual void BindAccount(int accountId, const std::string& username, const std::string& passwordHash, std::function<void(bool success)> callback) = 0;

        // Lie un compte a un fournisseur externe (Google, Apple...) (async)
        virtual void BindSocialAccount(int accountId, const std::string& provider, const std::string& providerId, std::function<void(bool success)> callback) = 0;

        // Cherche un compte par fournisseur (Google, Apple) et son ID (async)
        virtual void GetAccountBySocialId(const std::string& provider, const std::string& providerId, std::function<void(std::optional<Account>)> callback) = 0;

        // Met a jour la date de derniere connexion
        virtual void UpdateLastLogin(int accountId) = 0;
    };
}
