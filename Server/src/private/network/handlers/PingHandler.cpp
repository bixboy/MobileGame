#include "network/handlers/PingHandler.h"
#include "network/PacketBuilder.h"
#include "Core_generated.h"
#include "utils/Logger.h"
#include <chrono>

namespace MMO::Network
{
    void RegisterPingHandler(PacketDispatcher& dispatcher, entt::registry& /*registry*/)
    {
        dispatcher.RegisterHandler(Opcode_C2S_Ping,
            [](ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload)
            {
                auto ping = flatbuffers::GetRoot<Ping>(payload->data());
                if (!ping) return;

                int64_t clientTs = ping->timestamp();
                int64_t serverTs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();

                // Repondre avec un Pong unreliable (haute frequence)
                PacketBuilder::SendResponse(peer, Opcode_S2C_Pong,
                    [clientTs, serverTs](flatbuffers::FlatBufferBuilder& fbb)
                    {
                        PongBuilder pongBuilder(fbb);
                        pongBuilder.add_client_timestamp(clientTs);
                        pongBuilder.add_server_timestamp(serverTs);
                        fbb.Finish(pongBuilder.Finish());
                    }, /*reliable=*/false);
            });
    }
}
