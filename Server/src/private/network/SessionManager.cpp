#include "network/SessionManager.h"
#include "utils/Logger.h"

namespace MMO::Network
{
    void SessionManager::OnConnect(ENetPeer* peer)
    {
        if (!peer) return;

        // Creation d'une session vide pour le nouveau peer
        PlayerSession session;
        session.peer = peer;
        m_sessions[peer->connectID] = session;

        char ip[64];
        enet_address_get_ip(&peer->address, ip, sizeof(ip));
        LOG_INFO("Session creee pour le peer {} (IP: {}, Port: {})", peer->connectID, ip, peer->address.port);
    }

    std::optional<PlayerSession> SessionManager::OnDisconnect(ENetPeer* peer)
    {
        if (!peer) return std::nullopt;

        auto it = m_sessions.find(peer->connectID);
        if (it == m_sessions.end())
        {
            LOG_WARN("Deconnexion d'un peer sans session: {}", peer->connectID);
            return std::nullopt;
        }

        // Sauvegarde et suppression de la session
        PlayerSession session = it->second;
        m_sessions.erase(it);

        if (session.isAuthenticated)
        {
            LOG_INFO("Joueur deconnecte (PlayerID: {}, PeerID: {})", session.playerID, peer->connectID);
        }
        else
        {
            LOG_INFO("Client non-authentifie deconnecte (PeerID: {})", peer->connectID);
        }

        // Notification au GameLoop pour le nettoyage ECS
        if (m_onDisconnect)
        {
            m_onDisconnect(session);
        }

        return session;
    }

    void SessionManager::OnLogin(ENetPeer* peer, PlayerID playerID, EntityID entityID)
    {
        if (!peer) return;

        auto it = m_sessions.find(peer->connectID);
        if (it == m_sessions.end())
        {
            LOG_ERROR("OnLogin appele pour un peer sans session: {}", peer->connectID);
            return;
        }

        // Promotion de la session en authentifiee
        it->second.playerID = playerID;
        it->second.entityID = entityID;
        it->second.isAuthenticated = true;

        LOG_INFO("Session authentifiee (PeerID: {}, PlayerID: {})", peer->connectID, playerID);
    }

    ENetPeer* SessionManager::FindPeer(uint32_t peerID) const
    {
        auto it = m_sessions.find(peerID);
        if (it != m_sessions.end())
        {
            return it->second.peer;
        }
        return nullptr;
    }

    bool SessionManager::IsConnected(uint32_t peerID) const
    {
        return m_sessions.contains(peerID);
    }

    const PlayerSession* SessionManager::GetSession(ENetPeer* peer) const
    {
        if (!peer) return nullptr;

        auto it = m_sessions.find(peer->connectID);
        if (it != m_sessions.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    void SessionManager::SetDisconnectCallback(DisconnectCallback callback)
    {
        m_onDisconnect = std::move(callback);
    }

    void SessionManager::OnJoinKingdom(ENetPeer* peer, int kingdomId, EntityID entityID)
    {
        if (!peer) return;

        auto it = m_sessions.find(peer->connectID);
        if (it == m_sessions.end())
        {
            LOG_ERROR("OnJoinKingdom appele pour un peer sans session: {}", peer->connectID);
            return;
        }

        it->second.kingdomId = kingdomId;
        it->second.entityID = entityID;

        LOG_INFO("Session assignee au royaume {} (PeerID: {}, PlayerID: {})",
            kingdomId, peer->connectID, it->second.playerID);
    }

    std::vector<const PlayerSession*> SessionManager::GetSessionsByKingdom(int kingdomId) const
    {
        std::vector<const PlayerSession*> result;
        for (const auto& [id, session] : m_sessions)
        {
            if (session.kingdomId == kingdomId)
            {
                result.push_back(&session);
            }
        }
        return result;
    }
}
