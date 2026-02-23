#pragma once
#include "enet.h"
#include <span>
#include "core/Config.h"
#include "network/PacketDispatcher.h"
#include "network/SessionManager.h"


namespace MMO::Network 
{
    class NetworkManager 
    {
    public:
        NetworkManager();
        ~NetworkManager();

        // Initialise ENet et cree le serveur sur le port configure
        bool Initialize(const ServerConfig& config);

        // Arrete le serveur et libere les ressources ENet
        void Shutdown();

        // Traite tous les evenements ENet en attente (non-bloquant)
        void ProcessEvents();
        
        // Envoie un paquet a un client specifique
        void SendPacket(ENetPeer* peer, std::span<const uint8_t> data, bool reliable);
        
        // Envoie un paquet a tous les clients connectes
        void BroadcastPacket(std::span<const uint8_t> data, bool reliable);
        
        PacketDispatcher& GetDispatcher() { return m_dispatcher; }
        SessionManager& GetSessionManager() { return m_sessionManager; }

    private:
        void HandleConnect(const ENetEvent& event);
        void HandleReceive(const ENetEvent& event);
        void HandleDisconnect(const ENetEvent& event);

        ENetHost* m_host;                   // Serveur ENet
        PacketDispatcher m_dispatcher;      // Routage des paquets
        SessionManager m_sessionManager;    // Gestion des sessions joueurs
    };
}
