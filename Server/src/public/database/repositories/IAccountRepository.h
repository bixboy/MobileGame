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
        
        // Cree un nouveau compte avec mot de passe hashe (async)
        virtual void CreateAccount(const std::string& username, const std::string& passwordHash, std::function<void(bool success, std::optional<Account>)> callback) = 0;
        
        // Met a jour la date de derniere connexion
        virtual void UpdateLastLogin(int accountId) = 0;
    };
}
