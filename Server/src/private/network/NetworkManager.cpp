#include "network/NetworkManager.h"
#include "utils/Logger.h"

namespace MMO::Network 
{
    NetworkManager::NetworkManager() : m_host(nullptr) 
    {
    }

    NetworkManager::~NetworkManager() 
    {
        Shutdown();
    }

    bool NetworkManager::Initialize(const ServerConfig& config) 
    {
        if (enet_initialize() != 0) 
        {
            LOG_ERROR("Une erreur est survenue lors de l'initialisation de ENet.");
            return false;
        }

        ENetAddress address;
        enet_address_set_ip(&address, "0.0.0.0");
        address.port = config.port;

        // 6 args for ENet-CSharp
        m_host = enet_host_create(&address, config.maxPlayers, 2, 0, 0, 0);

        if (m_host == nullptr) 
        {
            LOG_ERROR("Une erreur est survenue lors de la creation du serveur ENet sur le port {}", config.port);
            return false;
        }

        LOG_INFO("Serveur ENet demarre sur le port {}", config.port);
        return true;
    }

    void NetworkManager::Shutdown() 
    {
        if (m_host != nullptr) 
        {
            enet_host_destroy(m_host);
            m_host = nullptr;
        }
        enet_deinitialize();
        LOG_INFO("Serveur ENet arrete.");
    }

    void NetworkManager::ProcessEvents() 
    {
        if (!m_host)
            return;

        ENetEvent event;
        while (enet_host_service(m_host, &event, 0) > 0) 
        {
            switch (event.type) 
            {
                case ENET_EVENT_TYPE_CONNECT: // Connect
                    LOG_INFO("DEBUG: EVENT_TYPE_CONNECT recu par le serveur !");
                    HandleConnect(event);
                    break;

                case ENET_EVENT_TYPE_RECEIVE: // Receive
                    LOG_INFO("DEBUG: EVENT_TYPE_RECEIVE recu par le serveur ({} bytes) !", event.packet->dataLength);
                    HandleReceive(event);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT: // Disconnect
                    LOG_INFO("DEBUG: EVENT_TYPE_DISCONNECT recu par le serveur !");
                    HandleDisconnect(event);
                    break;

                case ENET_EVENT_TYPE_NONE:
                    break;
            }
        }
    }

    // Connexion
    void NetworkManager::HandleConnect(const ENetEvent& event) 
    {
        char ip[64];
        enet_address_get_ip(&event.peer->address, ip, sizeof(ip));
        LOG_INFO("Un nouveau client s'est connecte (IP: {}, Port: {})", ip, event.peer->address.port);
    }

    // Packet Recue
    void NetworkManager::HandleReceive(const ENetEvent& event) 
    {
        m_dispatcher.Dispatch(event.peer, event.packet->data, event.packet->dataLength);
        enet_packet_destroy(event.packet);
    }

    // Deconnexion
    void NetworkManager::HandleDisconnect(const ENetEvent& event) 
    {
        LOG_INFO("Un client s'est deconnecte.");
    }

    
    // Envoie d'un packet
    void NetworkManager::SendPacket(ENetPeer* peer, std::span<const uint8_t> data, bool reliable) 
    {
        if (!peer)
            return;
        
        uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;
        ENetPacket* packet = enet_packet_create(data.data(), data.size(), flags);
        
        enet_peer_send(peer, 0, packet);
    }

    
    void NetworkManager::BroadcastPacket(std::span<const uint8_t> data, bool reliable) 
    {
        if (!m_host)
            return;
        
        uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;
        ENetPacket* packet = enet_packet_create(data.data(), data.size(), flags);
        
        enet_host_broadcast(m_host, 0, packet);
    }
}
