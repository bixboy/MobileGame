#include "network/PacketDispatcher.h"
#include "utils/Logger.h"

namespace MMO::Network 
{
    void PacketDispatcher::RegisterHandler(Opcode opcode, PacketHandlerFunc handler) 
    {
        if (m_handlers.contains(opcode)) 
        {
            LOG_WARN("Un Handler est deja enregistre pour l'Opcode: {}", static_cast<uint16_t>(opcode));
            return;
        }
        m_handlers[opcode] = std::move(handler);
    }

    void PacketDispatcher::Dispatch(ENetPeer* peer, const uint8_t* data, size_t size) 
    {
        // 1. Verification de securite FlatBuffers
        flatbuffers::Verifier verifier(data, size);
        if (!VerifyEnvelopeBuffer(verifier)) 
        {
            LOG_ERROR("Paquet Ignore : Structure FlatBuffer Invalide / Malformee");
            return;
        }

        // 2. Lecture de l'Envelope
        const Envelope* envelope = GetEnvelope(data);
        if (!envelope) return;

        Opcode opcode = envelope->opcode();

        // 3. Routage O(1) vers le Handler concerne
        auto it = m_handlers.find(opcode);
        if (it != m_handlers.end()) 
        {
            it->second(peer, envelope->payload_data());
        } 
        else 
        {
            LOG_WARN("Paquet Ignore : Aucun Handler enregistre pour l'Opcode {}", static_cast<uint16_t>(opcode));
        }
    }
}
