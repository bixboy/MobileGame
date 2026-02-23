#include "network/handlers/KingdomSelectHandler.h"
#include "network/PacketBuilder.h"
#include "world/KingdomWorld.h"
#include "ecs/PlayerComponents.h"
#include "Kingdom_generated.h"
#include "Resources_generated.h"
#include "utils/Logger.h"


namespace MMO::Network
{
    // --- Helpers locaux ---

    static void SendKingdomList(ENetPeer* peer,
        const std::unordered_map<int, std::unique_ptr<MMO::Core::KingdomWorld>>& kingdoms,
        const SessionManager& sessionManager)
    {
        PacketBuilder::SendResponse(peer, Opcode_S2C_KingdomList,
            [&kingdoms, &sessionManager](flatbuffers::FlatBufferBuilder& fbb)
            {
                std::vector<flatbuffers::Offset<KingdomEntry>> entries;
                for (const auto& [id, world] : kingdoms)
                {
                    auto sessions = sessionManager.GetSessionsByKingdom(id);
                    auto nameOffset = fbb.CreateString(world->GetName());

                    KingdomEntryBuilder builder(fbb);
                    builder.add_id(id);
                    builder.add_name(nameOffset);
                    builder.add_player_count(static_cast<int>(sessions.size()));
                    builder.add_max_players(1000);
                    builder.add_status(1); // Online
                    entries.push_back(builder.Finish());
                }

                auto vec = fbb.CreateVector(entries);
                KingdomListBuilder listBuilder(fbb);
                listBuilder.add_kingdoms(vec);
                fbb.Finish(listBuilder.Finish());
            });
    }

    static void SendPlayerData(ENetPeer* peer, const Database::Account& account, const Database::PlayerData& data)
    {
        PacketBuilder::SendResponse(peer, Opcode_S2C_PlayerData,
            [&account, &data](flatbuffers::FlatBufferBuilder& fbb)
            {
                auto nameOffset = fbb.CreateString(account.username);
                PlayerDataBuilder builder(fbb);
                builder.add_account_id(account.id);
                builder.add_username(nameOffset);
                builder.add_pos_x(data.posX);
                builder.add_pos_y(data.posY);
                builder.add_food(data.food);
                builder.add_wood(data.wood);
                builder.add_stone(data.stone);
                builder.add_gold(data.gold);
                fbb.Finish(builder.Finish());
            });
    }

    static entt::entity CreatePlayerEntity(entt::registry& registry, SessionManager& sessionManager,
        ENetPeer* peer, const Database::Account& account, const Database::PlayerData& data)
    {
        auto entity = registry.create();

        registry.emplace<ECS::PlayerInfoComponent>(entity,
            ECS::PlayerInfoComponent{ peer->connectID, account.id, account.username });
        
        registry.emplace<ECS::PositionComponent>(entity,
            ECS::PositionComponent{ data.posX, data.posY });
        
        registry.emplace<ECS::ResourcesComponent>(entity,
            ECS::ResourcesComponent{ data.food, data.wood, data.stone, data.gold });

        return entity;
    }

    // --- Enregistrement des handlers ---

    void RegisterKingdomSelectHandler(PacketDispatcher& dispatcher, SessionManager& sessionManager,
        std::unordered_map<int, std::unique_ptr<MMO::Core::KingdomWorld>>& kingdoms,
        std::shared_ptr<Database::IAccountRepository> accountRepo,
        std::shared_ptr<Database::IPlayerRepository> playerRepo,
        std::function<void(std::function<void()>)> runOnMainThread)
    {
        // C2S_RequestKingdoms → S2C_KingdomList
        dispatcher.RegisterHandler(Opcode_C2S_RequestKingdoms,
            [&kingdoms, &sessionManager](ENetPeer* peer, const flatbuffers::Vector<uint8_t>* /*payload*/)
            {
                auto* session = sessionManager.GetSession(peer);
                if (!session || !session->isAuthenticated)
                {
                    LOG_WARN("RequestKingdoms: peer non authentifie (PeerID: {})", peer->connectID);
                    return;
                }

                LOG_INFO("Envoi de la liste des royaumes ({} royaumes) au joueur {}",
                    kingdoms.size(), session->playerID);

                SendKingdomList(peer, kingdoms, sessionManager);
            });

        // C2S_SelectKingdom → charge le profil → cree l'entite → S2C_PlayerData
        dispatcher.RegisterHandler(Opcode_C2S_SelectKingdom,
            [&kingdoms, &sessionManager, accountRepo, playerRepo, runOnMainThread]
            (ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload)
            {
                auto req = flatbuffers::GetRoot<SelectKingdom>(payload->data());
                if (!req) return;

                auto* session = sessionManager.GetSession(peer);
                if (!session || !session->isAuthenticated)
                {
                    LOG_WARN("SelectKingdom: peer non authentifie (PeerID: {})", peer->connectID);
                    return;
                }

                // Deja dans un royaume ?
                if (session->kingdomId >= 0)
                {
                    LOG_WARN("SelectKingdom: joueur {} deja dans le royaume {}",
                        session->playerID, session->kingdomId);
                    return;
                }

                int kingdomId = req->kingdom_id();

                // Verifier que le royaume existe
                auto it = kingdoms.find(kingdomId);
                if (it == kingdoms.end())
                {
                    LOG_WARN("SelectKingdom: royaume {} introuvable", kingdomId);
                    return;
                }

                int accountId = static_cast<int>(session->playerID);
                uint32_t peerID = peer->connectID;

                LOG_INFO("Joueur {} selectionne le royaume '{}' (ID: {})",
                    accountId, it->second->GetName(), kingdomId);

                // Charger le compte depuis la DB
                accountRepo->GetById(accountId, [playerRepo, runOnMainThread, &sessionManager, &kingdoms, peerID, accountId, kingdomId]
                    (std::optional<Database::Account> account)
                {
                    if (!account)
                    {
                        LOG_ERROR("SelectKingdom: compte {} introuvable", accountId);
                        return;
                    }

                    // Charger les donnees joueur pour ce royaume
                    playerRepo->GetByAccountAndKingdom(accountId, kingdomId, [playerRepo, runOnMainThread, &sessionManager, &kingdoms, peerID, account, kingdomId]
                        (std::optional<Database::PlayerData> playerData)
                    {
                        if (!playerData)
                        {
                            // Creer le profil automatiquement
                            LOG_INFO("SelectKingdom: creation auto du profil joueur (AccountID: {}, KingdomID: {})", account->id, kingdomId);
                            playerRepo->Create(account->id, kingdomId, [runOnMainThread, &sessionManager, &kingdoms, peerID, account, kingdomId]
                                (std::optional<Database::PlayerData> newData)
                            {
                                if (!newData)
                                {
                                    LOG_ERROR("SelectKingdom: echec creation profil (AccountID: {})", account->id);
                                    return;
                                }
                                runOnMainThread([&sessionManager, &kingdoms, peerID, account, newData, kingdomId]()
                                {
                                    ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                    if (!safePeer) return;

                                    auto kIt = kingdoms.find(kingdomId);
                                    if (kIt == kingdoms.end()) return;

                                    auto entity = CreatePlayerEntity(kIt->second->GetRegistry(), sessionManager, safePeer, *account, *newData);
                                    sessionManager.OnJoinKingdom(safePeer, kingdomId, entity);
                                    kIt->second->GetSpatialGrid().Insert(entity, newData->posX, newData->posY);

                                    SendPlayerData(safePeer, *account, *newData);
                                    LOG_INFO("Joueur {} rejoint le royaume '{}' (entite creee)",
                                        account->username, kIt->second->GetName());
                                });
                            });
                            return;
                        }

                        runOnMainThread([&sessionManager, &kingdoms, peerID, account, playerData, kingdomId]()
                        {
                            ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                            if (!safePeer) return;

                            auto kIt = kingdoms.find(kingdomId);
                            if (kIt == kingdoms.end()) return;

                            auto entity = CreatePlayerEntity(kIt->second->GetRegistry(), sessionManager, safePeer, *account, *playerData);
                            sessionManager.OnJoinKingdom(safePeer, kingdomId, entity);
                            kIt->second->GetSpatialGrid().Insert(entity, playerData->posX, playerData->posY);

                            SendPlayerData(safePeer, *account, *playerData);
                            LOG_INFO("Joueur {} rejoint le royaume '{}' (entite creee)",
                                account->username, kIt->second->GetName());
                        });
                    });
                });
            });
    }
}
