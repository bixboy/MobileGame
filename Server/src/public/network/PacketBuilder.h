#pragma once
#include "enet.h"
#include "NetworkCore_generated.h"


namespace MMO::Network
{
    // Construire et envoyer un paquet FlatBuffers
    class PacketBuilder
    {
    public:
        // Construit et envoie un paquet encapsule dans une Envelope
        template<typename BuilderFunc>
        static void SendResponse(ENetPeer* peer, Opcode opcode, BuilderFunc payloadBuilder, bool reliable = true)
        {
            if (!peer)
                return;

            // Construction du payload interne
            flatbuffers::FlatBufferBuilder payloadFbb;
            payloadBuilder(payloadFbb);

            // Encapsulation dans l'Envelope
            flatbuffers::FlatBufferBuilder envBuilder;
            auto payloadVector = envBuilder.CreateVector(payloadFbb.GetBufferPointer(), payloadFbb.GetSize());
            EnvelopeBuilder env(envBuilder);
            env.add_opcode(opcode);
            env.add_payload_data(payloadVector);
            envBuilder.Finish(env.Finish());

            // Envoi via ENet
            uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;
            ENetPacket* packet = enet_packet_create(envBuilder.GetBufferPointer(), envBuilder.GetSize(), flags);
            enet_peer_send(peer, 0, packet);
        }
    };
}
