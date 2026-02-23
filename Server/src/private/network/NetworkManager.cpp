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
        // Initialisation de la librairie ENet
        if (enet_initialize() != 0) 
        {
            LOG_ERROR("Une erreur est survenue lors de l'initialisation de ENet.");
            return false;
        }

        // Creation du serveur sur le port configure
        ENetAddress address;
        enet_address_set_ip(&address, "0.0.0.0");
        address.port = config.port;

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

    // Traite les evenements ENet (connexion, reception, deconnexion)
    void NetworkManager::ProcessEvents() 
    {
        if (!m_host)
            return;

        ENetEvent event;
        while (enet_host_service(m_host, &event, 0) > 0) 
        {
            switch (event.type) 
            {
                case ENET_EVENT_TYPE_CONNECT:
                    HandleConnect(event);
                    break;

                case ENET_EVENT_TYPE_RECEIVE:
                    HandleReceive(event);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    HandleDisconnect(event);
                    break;
                
                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                    HandleDisconnect(event);
                    break;

                case ENET_EVENT_TYPE_NONE:
                    break;
            }
        }
    }

    // Nouvelle connexion - cree une session via le SessionManager
    void NetworkManager::HandleConnect(const ENetEvent& event) 
    {
        m_sessionManager.OnConnect(event.peer);
    }

    // Paquet recu - deserialise et dispatch vers le bon handler
    void NetworkManager::HandleReceive(const ENetEvent& event) 
    {
        m_dispatcher.Dispatch(event.peer, event.packet->data, event.packet->dataLength);
        enet_packet_destroy(event.packet);
    }

    // Deconnexion - supprime la session et notifie le GameLoop
    void NetworkManager::HandleDisconnect(const ENetEvent& event) 
    {
        m_sessionManager.OnDisconnect(event.peer);
    }

    
    // Envoie un paquet a un client specifique
    void NetworkManager::SendPacket(ENetPeer* peer, std::span<const uint8_t> data, bool reliable) 
    {
        if (!peer)
            return;
        
        uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;
        ENetPacket* packet = enet_packet_create(data.data(), data.size(), flags);
        
        enet_peer_send(peer, 0, packet);
    }

    // Envoie un paquet a tous les clients connectes
    void NetworkManager::BroadcastPacket(std::span<const uint8_t> data, bool reliable) 
    {
        if (!m_host)
            return;
        
        uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;
        ENetPacket* packet = enet_packet_create(data.data(), data.size(), flags);
        
        enet_host_broadcast(m_host, 0, packet);
    }
}
