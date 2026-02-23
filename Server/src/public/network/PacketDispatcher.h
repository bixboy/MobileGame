#pragma once
#include "enet.h"
#include <functional>
#include <unordered_map>
#include "NetworkCore_generated.h"


namespace MMO::Network 
{
    using PacketHandlerFunc = std::function<void(ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload)>;

    class PacketDispatcher 
    {
    public:
        PacketDispatcher() = default;

        // Enregistre un handler pour un opcode donne
        void RegisterHandler(Opcode opcode, PacketHandlerFunc handler);

        // Deserialise l'envelope et dispatch vers le handler concerne
        void Dispatch(ENetPeer* peer, const uint8_t* data, size_t size);

    private:
        // Table de routage Opcode → Handler
        std::unordered_map<Opcode, PacketHandlerFunc> m_handlers;
    };
}
