#pragma once
#include "network/enet.h"
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

        // Enregistre un Handler
        void RegisterHandler(Opcode opcode, PacketHandlerFunc handler);

        // De-serialize et dispatch vers le bon Handler
        void Dispatch(ENetPeer* peer, const uint8_t* data, size_t size);

    private:
        std::unordered_map<Opcode, PacketHandlerFunc> m_handlers;
    };
}
