#include "network/Handlers.h"
#include "NetworkCore_generated.h"
#include "utils/Logger.h"


namespace MMO::Network 
{
    void RegisterAllNetworkHandlers(PacketDispatcher& dispatcher, entt::registry& registry)
    {
        // --- Handler : Ping ---
        dispatcher.RegisterHandler(Opcode_C2S_Ping, 
            [&registry](ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload) 
            {
                auto ping = flatbuffers::GetRoot<Ping>(payload->data());
                
                LOG_INFO("Serveur : Ping recu avec le timestamp distant {}", ping->timestamp());
                
                // TODO: Renvoyer un S2C_Pong avec ENet
            });


        // --- Handler : Login ---
        // dispatcher.RegisterHandler(Opcode_C2S_Login, ...
        
        
        // --- Handler : Movement ---
        // dispatcher.RegisterHandler(Opcode_C2S_MoveRequest, ...
    }
}
