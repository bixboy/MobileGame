#pragma once
#include "network/enet.h"
#include <span>
#include "core/Config.h"
#include "network/PacketDispatcher.h"


namespace MMO::Network 
{
    class NetworkManager 
    {
    public:
        NetworkManager();
        ~NetworkManager();

        bool Initialize(const ServerConfig& config);
        void Shutdown();

        // Appele par le GameLoop a chaque tick pour lire les messages ENet
        void ProcessEvents();
        
        // Permet d'envoyer un FlatBuffer binaire a un pair precis
        void SendPacket(ENetPeer* peer, std::span<const uint8_t> data, bool reliable);
        
        // Permet de diffuser un FlatBuffer a tout le monde
        void BroadcastPacket(std::span<const uint8_t> data, bool reliable);
        
        // Permet d'enregistrer les fonctions a l'exterieur
        PacketDispatcher& GetDispatcher() { return m_dispatcher; }

    private:
        void HandleConnect(const ENetEvent& event);
        void HandleReceive(const ENetEvent& event);
        void HandleDisconnect(const ENetEvent& event);

        ENetHost* m_host;
        PacketDispatcher m_dispatcher;
    };
}
