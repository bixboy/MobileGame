#pragma once
#include <unordered_map>
#include <optional>
#include <functional>
#include "enet.h"
#include "core/Types.h"


namespace MMO::Network
{
    // Donnees de session d'un joueur connecte
    struct PlayerSession
    {
        ENetPeer* peer = nullptr;
        PlayerID playerID = INVALID_PLAYER;
        EntityID entityID = INVALID_ENTITY;
        bool isAuthenticated = false;
        int kingdomId = -1;  // -1 = pas encore dans un royaume
    };

    // Gere le cycle de vie des connexions joueurs (connect → login → disconnect)
    class SessionManager
    {
    public:
        using DisconnectCallback = std::function<void(const PlayerSession&)>;

        SessionManager() = default;

        // Nouvelle connexion — cree une session non-authentifiee
        void OnConnect(ENetPeer* peer);

        // Deconnexion — supprime la session et retourne ses donnees
        std::optional<PlayerSession> OnDisconnect(ENetPeer* peer);

        // Login reussi — associe un PlayerID et un EntityID a la session
        void OnLogin(ENetPeer* peer, PlayerID playerID, EntityID entityID);

        // Resout un PeerID en ENetPeer* valide (safe pour les callbacks async)
        ENetPeer* FindPeer(uint32_t peerID) const;

        // Verifie si un peer est toujours connecte
        bool IsConnected(uint32_t peerID) const;

        // Recupere la session d'un peer (nullptr si introuvable)
        const PlayerSession* GetSession(ENetPeer* peer) const;

        // Definit le callback appele a chaque deconnexion
        void SetDisconnectCallback(DisconnectCallback callback);

        // Rejoindre un royaume — associe le kingdomId et l'entite a la session
        void OnJoinKingdom(ENetPeer* peer, int kingdomId, EntityID entityID);

        // Retourne toutes les sessions dans un royaume donne
        std::vector<const PlayerSession*> GetSessionsByKingdom(int kingdomId) const;

    private:
        std::unordered_map<uint32_t, PlayerSession> m_sessions;
        DisconnectCallback m_onDisconnect;
    };
}
