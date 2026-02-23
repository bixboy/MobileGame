#include "network/handlers/LoginHandler.h"
#include "network/PacketBuilder.h"
#include "Auth_generated.h"
#include "utils/Logger.h"
#include "utils/PasswordHasher.h"
#include <chrono>
#include <unordered_map>

namespace
{
    // Rate limiting : max 5 tentatives par IP sur 60 secondes
    struct LoginAttempt
    {
        int count = 0;
        std::chrono::steady_clock::time_point windowStart = std::chrono::steady_clock::now();
    };

    std::unordered_map<uint32_t, LoginAttempt> s_loginAttempts;
    constexpr int MAX_ATTEMPTS = 5;
    constexpr int WINDOW_SECONDS = 60;
    constexpr int MIN_PASSWORD_LENGTH = 4;
    constexpr int MAX_USERNAME_LENGTH = 32;
}

namespace MMO::Network
{
    void RegisterLoginHandler(PacketDispatcher& dispatcher, SessionManager& sessionManager,
        std::shared_ptr<Database::IAccountRepository> accountRepo,
        std::function<void(std::function<void()>)> runOnMainThread)
    {
        dispatcher.RegisterHandler(Opcode_C2S_Login,
            [accountRepo, runOnMainThread, &sessionManager](ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload)
            {
                auto loginReq = flatbuffers::GetRoot<Login>(payload->data());
                if (!loginReq || !loginReq->username()) return;

                std::string username = loginReq->username()->str();
                std::string password = loginReq->password() ? loginReq->password()->str() : "";

                // --- Rate Limiting ---
                char ipBuffer[64] = {};
                enet_peer_get_ip(peer, ipBuffer, sizeof(ipBuffer));
                uint32_t peerIP = std::hash<std::string>{}(std::string(ipBuffer));
                auto now = std::chrono::steady_clock::now();
                auto& attempt = s_loginAttempts[peerIP];
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - attempt.windowStart).count();

                if (elapsed > WINDOW_SECONDS)
                {
                    attempt.count = 0;
                    attempt.windowStart = now;
                }

                if (attempt.count >= MAX_ATTEMPTS)
                {
                    LOG_WARN("Rate limit atteint pour IP {:08x}. Login rejete.", peerIP);
                    PacketBuilder::SendResponse(peer, Opcode_S2C_LoginResult,
                        [](flatbuffers::FlatBufferBuilder& fbb)
                        {
                            auto msgOffset = fbb.CreateString("Trop de tentatives. Reessayez dans 1 minute.");
                            LoginResultBuilder rb(fbb);
                            rb.add_success(false);
                            rb.add_account_id(-1);
                            rb.add_message(msgOffset);
                            fbb.Finish(rb.Finish());
                        });
                    return;
                }
                attempt.count++;

                // --- Validation ---
                if (username.empty() || username.length() > MAX_USERNAME_LENGTH)
                {
                    PacketBuilder::SendResponse(peer, Opcode_S2C_LoginResult,
                        [](flatbuffers::FlatBufferBuilder& fbb)
                        {
                            auto msgOffset = fbb.CreateString("Nom d'utilisateur invalide (1-32 caracteres).");
                            LoginResultBuilder rb(fbb);
                            rb.add_success(false);
                            rb.add_account_id(-1);
                            rb.add_message(msgOffset);
                            fbb.Finish(rb.Finish());
                        });
                    return;
                }

                if (password.length() < MIN_PASSWORD_LENGTH)
                {
                    PacketBuilder::SendResponse(peer, Opcode_S2C_LoginResult,
                        [](flatbuffers::FlatBufferBuilder& fbb)
                        {
                            auto msgOffset = fbb.CreateString("Mot de passe trop court (4 caracteres minimum).");
                            LoginResultBuilder rb(fbb);
                            rb.add_success(false);
                            rb.add_account_id(-1);
                            rb.add_message(msgOffset);
                            fbb.Finish(rb.Finish());
                        });
                    return;
                }

                LOG_INFO("Requete de Login recu pour: {}", username);

                uint32_t peerID = peer->connectID;

                // Recherche du compte en base
                accountRepo->GetAccountByUsername(username,
                    [accountRepo, runOnMainThread, &sessionManager, peerID, username, password](std::optional<Database::Account> account)
                    {
                        if (account.has_value())
                        {
                            // Verification du mot de passe
                            if (!Utils::PasswordHasher::Verify(password, account->passwordHash))
                            {
                                LOG_WARN("Mot de passe incorrect pour: {}", username);
                                runOnMainThread([&sessionManager, peerID]()
                                {
                                    ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                    if (!safePeer) return;

                                    PacketBuilder::SendResponse(safePeer, Opcode_S2C_LoginResult,
                                        [](flatbuffers::FlatBufferBuilder& fbb)
                                        {
                                            auto msgOffset = fbb.CreateString("Mot de passe incorrect.");
                                            LoginResultBuilder resultBuilder(fbb);
                                            resultBuilder.add_success(false);
                                            resultBuilder.add_account_id(-1);
                                            resultBuilder.add_message(msgOffset);
                                            fbb.Finish(resultBuilder.Finish());
                                        });
                                });
                                return;
                            }

                            LOG_INFO("Compte existant trouve et valide pour {}.", username);
                            accountRepo->UpdateLastLogin(account->id);

                            runOnMainThread([&sessionManager, peerID, account]()
                            {
                                ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                if (!safePeer) return;

                                // Le Master Server n'a pas d'entite ECS. 
                                // On passe entt::null (cast implicite) pour l'entite, mais l'authentification est validee.
                                sessionManager.OnLogin(safePeer, static_cast<PlayerID>(account->id), entt::null);

                                PacketBuilder::SendResponse(safePeer, Opcode_S2C_LoginResult,
                                    [&account](flatbuffers::FlatBufferBuilder& fbb)
                                    {
                                        auto msgOffset = fbb.CreateString("Bienvenue de retour !");
                                        LoginResultBuilder resultBuilder(fbb);
                                        resultBuilder.add_success(true);
                                        resultBuilder.add_account_id(account->id);
                                        resultBuilder.add_message(msgOffset);
                                        fbb.Finish(resultBuilder.Finish());
                                    });
                            });
                        }
                        else
                        {
                            // Creation automatique
                            LOG_INFO("Compte introuvable pour {}. Creation du compte...", username);
                            accountRepo->CreateAccount(username, password,
                                [runOnMainThread, &sessionManager, peerID](bool success, std::optional<Database::Account> newAcc)
                                {
                                    if (!success || !newAcc.has_value())
                                    {
                                        runOnMainThread([&sessionManager, peerID]()
                                        {
                                            ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                            if (!safePeer) return;

                                            PacketBuilder::SendResponse(safePeer, Opcode_S2C_LoginResult,
                                                [](flatbuffers::FlatBufferBuilder& fbb)
                                                {
                                                    auto msgOffset = fbb.CreateString("Echec creation du compte.");
                                                    LoginResultBuilder resultBuilder(fbb);
                                                    resultBuilder.add_success(false);
                                                    resultBuilder.add_account_id(-1);
                                                    resultBuilder.add_message(msgOffset);
                                                    fbb.Finish(resultBuilder.Finish());
                                                });
                                        });
                                        return;
                                    }

                                    runOnMainThread([&sessionManager, peerID, newAcc]()
                                    {
                                        ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                        if (!safePeer) return;

                                        sessionManager.OnLogin(safePeer, static_cast<PlayerID>(newAcc->id), entt::null);

                                        PacketBuilder::SendResponse(safePeer, Opcode_S2C_LoginResult,
                                            [&newAcc](flatbuffers::FlatBufferBuilder& fbb)
                                            {
                                                auto msgOffset = fbb.CreateString("Compte cree avec succes !");
                                                LoginResultBuilder resultBuilder(fbb);
                                                resultBuilder.add_success(true);
                                                resultBuilder.add_account_id(newAcc->id);
                                                resultBuilder.add_message(msgOffset);
                                                fbb.Finish(resultBuilder.Finish());
                                            });
                                    });
                                });
                        }
                    });
            });
    }
}
