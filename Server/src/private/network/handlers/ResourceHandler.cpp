#include "network/handlers/ResourceHandler.h"
#include "network/PacketBuilder.h"
#include "world/KingdomWorld.h"
#include "NetworkCore_generated.h"
#include "ecs/PlayerComponents.h"
#include "utils/Logger.h"


namespace MMO::Network
{
    // Envoie les ressources mises a jour au client
    static void SendResourceUpdate(ENetPeer* peer, const ECS::ResourcesComponent& res)
    {
        PacketBuilder::SendResponse(peer, Opcode_S2C_ResourceUpdate,
            [&res](flatbuffers::FlatBufferBuilder& fbb)
            {
                ResourceUpdateBuilder builder(fbb);
                builder.add_food(res.food);
                builder.add_wood(res.wood);
                builder.add_stone(res.stone);
                builder.add_gold(res.gold);
                fbb.Finish(builder.Finish());
            });
    }

    void RegisterResourceHandler(PacketDispatcher& dispatcher, SessionManager& sessionManager,
        std::unordered_map<int, std::unique_ptr<MMO::Core::KingdomWorld>>& kingdoms,
        std::shared_ptr<Database::IPlayerRepository> playerRepo)
    {
        // Clamping max pour empecher les exploits
        constexpr int MAX_DELTA = 1000;

        dispatcher.RegisterHandler(Opcode_C2S_ModifyResources,
            [playerRepo, &sessionManager, &kingdoms](ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload)
            {
                auto req = flatbuffers::GetRoot<ModifyResources>(payload->data());
                if (!req)
                    return;

                auto type = req->resource_type();
                int delta = std::clamp(req->delta(), -MAX_DELTA, MAX_DELTA);

                // Verification que le peer est dans un royaume
                auto* session = sessionManager.GetSession(peer);
                if (!session || session->kingdomId < 0 || session->entityID == MMO::INVALID_ENTITY)
                {
                    LOG_WARN("ModifyResources: peer non authentifie ou pas dans un royaume (PeerID: {})", peer->connectID);
                    return;
                }

                // Recuperer la registry du bon royaume
                auto kIt = kingdoms.find(session->kingdomId);
                if (kIt == kingdoms.end())
                    return;

                auto& registry = kIt->second->GetRegistry();
                auto entity = session->entityID;

                if (!registry.valid(entity) || !registry.all_of<ECS::ResourcesComponent, ECS::PlayerInfoComponent>(entity))
                    return;

                auto& res = registry.get<ECS::ResourcesComponent>(entity);
                auto& info = registry.get<ECS::PlayerInfoComponent>(entity);

                // Application du delta via enum (O(1), type-safe)
                switch (type)
                {
                    case ResourceType_Food:  res.food  = std::max(0, res.food + delta);  break;
                    case ResourceType_Wood:  res.wood  = std::max(0, res.wood + delta);  break;
                    case ResourceType_Stone: res.stone = std::max(0, res.stone + delta); break;
                    case ResourceType_Gold:  res.gold  = std::max(0, res.gold + delta);  break;
                    default:
                        LOG_WARN("ModifyResources: type inconnu ({})", static_cast<int>(type));
                        return;
                }

                LOG_INFO("Ressources modifiees pour {} : type={} {:+d} -> Food:{} Wood:{} Stone:{} Gold:{}",
                    info.username, EnumNameResourceType(type), delta, res.food, res.wood, res.stone, res.gold);

                // Sauvegarde async en DB (cle composite: account_id + kingdom_id)
                playerRepo->UpdateResources(info.accountID, session->kingdomId, res.food, res.wood, res.stone, res.gold);

                // Confirmation au client
                SendResourceUpdate(peer, res);
            });
    }
}
