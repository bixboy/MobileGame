#include "network/handlers/LoginHandler.h"
#include "network/PacketBuilder.h"
#include "Core_generated.h"
#include "Auth_generated.h"
#include "utils/Logger.h"
#include "utils/PasswordHasher.h"
#include <chrono>
#include <unordered_map>
#include <regex>

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
    constexpr int MAX_USERNAME_LENGTH = 16;
    
    // Regex: Alphanumerique + underscores, 3 a 16 caracteres
    const std::regex USERNAME_REGEX("^[a-zA-Z0-9_]{3,16}$");

    bool CheckRateLimit(ENetPeer* peer)
    {
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
            return false;
            
        attempt.count++;
        return true;
    }

    void SendLoginError(ENetPeer* peer, const std::string& message)
    {
        MMO::Network::PacketBuilder::SendResponse(peer, MMO::Network::Opcode_S2C_LoginResult,
            [&message](flatbuffers::FlatBufferBuilder& fbb)
            {
                auto msgOffset = fbb.CreateString(message);
                auto tokenOffset = fbb.CreateString("");
                MMO::Network::LoginResultBuilder rb(fbb);
                rb.add_success(false);
                rb.add_account_id(-1);
                rb.add_message(msgOffset);
                rb.add_session_token(tokenOffset);
                fbb.Finish(rb.Finish());
            });
    }

    void SendLoginSuccess(ENetPeer* peer, int accountId, const std::string& message, const std::string& sessionToken)
    {
        MMO::Network::PacketBuilder::SendResponse(peer, MMO::Network::Opcode_S2C_LoginResult,
            [&message, accountId, &sessionToken](flatbuffers::FlatBufferBuilder& fbb)
            {
                auto msgOffset = fbb.CreateString(message);
                auto tokenOffset = fbb.CreateString(sessionToken);
                MMO::Network::LoginResultBuilder rb(fbb);
                rb.add_success(true);
                rb.add_account_id(accountId);
                rb.add_message(msgOffset);
                rb.add_session_token(tokenOffset);
                fbb.Finish(rb.Finish());
            });
    }

    void SendBindAccountResult(ENetPeer* peer, bool success, const std::string& message)
    {
        MMO::Network::PacketBuilder::SendResponse(peer, MMO::Network::Opcode_S2C_BindAccountResult,
            [&message, success](flatbuffers::FlatBufferBuilder& fbb)
            {
                auto msgOffset = fbb.CreateString(message);
                MMO::Network::BindAccountResultBuilder br(fbb);
                br.add_success(success);
                br.add_message(msgOffset);
                fbb.Finish(br.Finish());
            });
    }

    void SendBindSocialAccountResult(ENetPeer* peer, bool success, const std::string& message)
    {
        MMO::Network::PacketBuilder::SendResponse(peer, MMO::Network::Opcode_S2C_BindSocialAccountResult,
            [&message, success](flatbuffers::FlatBufferBuilder& fbb)
            {
                auto msgOffset = fbb.CreateString(message);
                MMO::Network::BindSocialAccountResultBuilder br(fbb);
                br.add_success(success);
                br.add_message(msgOffset);
                fbb.Finish(br.Finish());
            });
    }
}

namespace MMO::Network
{
    void RegisterLoginHandler(PacketDispatcher& dispatcher, SessionManager& sessionManager,
        std::shared_ptr<Database::IAccountRepository> accountRepo,
        std::function<void(std::function<void()>)> runOnMainThread)
    {
        // --------------------------------------------------------------------
        // C2S_Login (Classique Username / Password)
        // --------------------------------------------------------------------
        dispatcher.RegisterHandler(Opcode_C2S_Login,
            [accountRepo, runOnMainThread, &sessionManager](ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload)
            {
                auto loginReq = flatbuffers::GetRoot<Login>(payload->data());
                if (!loginReq || !loginReq->username()) return;

                std::string username = loginReq->username()->str();
                std::string password = loginReq->password() ? loginReq->password()->str() : "";

                if (!CheckRateLimit(peer))
                {
                    LOG_WARN("Rate limit atteint. Login rejete.");
                    SendLoginError(peer, "Trop de tentatives. Reessayez dans 1 minute.");
                    return;
                }

                if (!std::regex_match(username, USERNAME_REGEX))
                {
                    SendLoginError(peer, "Pseudo invalide (3-16 caracteres, lettres/chiffres/underscores uniquement).");
                    return;
                }

                if (password.length() < MIN_PASSWORD_LENGTH)
                {
                    SendLoginError(peer, "Mot de passe trop court (4 caracteres minimum).");
                    return;
                }

                LOG_INFO("Requete de Login recu pour: {}", username);
                uint32_t peerID = peer->connectID;

                accountRepo->GetAccountByUsername(username,
                    [accountRepo, runOnMainThread, &sessionManager, peerID, username, password](std::optional<Database::Account> account)
                    {
                        if (account.has_value())
                        {
                            if (!Utils::PasswordHasher::Verify(password, account->passwordHash))
                            {
                                LOG_WARN("Mot de passe incorrect pour: {}", username);
                                runOnMainThread([&sessionManager, peerID]()
                                {
                                    ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                    if (safePeer) SendLoginError(safePeer, "Mot de passe incorrect.");
                                });
                                return;
                            }

                            LOG_INFO("Compte existant trouve et valide pour {}.", username);
                            accountRepo->UpdateLastLogin(account->id);

                            runOnMainThread([&sessionManager, peerID, account]()
                            {
                                ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                if (!safePeer) return;

                                std::string token = sessionManager.OnLogin(safePeer, static_cast<PlayerID>(account->id), entt::null);
                                SendLoginSuccess(safePeer, account->id, "Bienvenue de retour !", token);
                            });
                        }
                        else
                        {
                            LOG_INFO("Compte introuvable pour {}. Creation du compte...", username);
                            accountRepo->CreateAccount(username, password,
                                [runOnMainThread, &sessionManager, peerID](bool success, std::optional<Database::Account> newAcc)
                                {
                                    runOnMainThread([&sessionManager, peerID, success, newAcc]()
                                    {
                                        ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                        if (!safePeer) return;

                                        if (!success || !newAcc.has_value())
                                        {
                                            SendLoginError(safePeer, "Echec creation du compte.");
                                            return;
                                        }

                                        std::string token = sessionManager.OnLogin(safePeer, static_cast<PlayerID>(newAcc->id), entt::null);
                                        SendLoginSuccess(safePeer, newAcc->id, "Compte cree avec succes !", token);
                                    });
                                });
                        }
                    });
            });

        // --------------------------------------------------------------------
        // C2S_GuestLogin (Connexion via DeviceID sans mot de passe)
        // --------------------------------------------------------------------
        dispatcher.RegisterHandler(Opcode_C2S_GuestLogin,
            [accountRepo, runOnMainThread, &sessionManager](ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload)
            {
                auto guestReq = flatbuffers::GetRoot<GuestLogin>(payload->data());
                if (!guestReq || !guestReq->device_id()) return;

                std::string deviceId = guestReq->device_id()->str();

                if (!CheckRateLimit(peer))
                {
                    SendLoginError(peer, "Trop de tentatives.");
                    return;
                }

                LOG_INFO("Requete de Guest Login recu pour DeviceID: {}", deviceId);
                uint32_t peerID = peer->connectID;

                accountRepo->GetAccountByDeviceId(deviceId,
                    [accountRepo, runOnMainThread, &sessionManager, peerID, deviceId](std::optional<Database::Account> account)
                    {
                        if (account.has_value())
                        {
                            LOG_INFO("Compte invite existant trouve pour DeviceID {}.", deviceId);
                            accountRepo->UpdateLastLogin(account->id);

                            runOnMainThread([&sessionManager, peerID, account]()
                            {
                                ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                if (!safePeer) return;

                                std::string token = sessionManager.OnLogin(safePeer, static_cast<PlayerID>(account->id), entt::null);
                                SendLoginSuccess(safePeer, account->id, "Connexion invite reussie !", token);
                            });
                        }
                        else
                        {
                            std::string guestName = "Guest_" + deviceId.substr(0, 8);
                            LOG_INFO("Creation d'un nouveau compte invite: {}", guestName);

                            accountRepo->CreateGuestAccount(deviceId, guestName,
                                [runOnMainThread, &sessionManager, peerID](bool success, std::optional<Database::Account> newAcc)
                                {
                                    runOnMainThread([&sessionManager, peerID, success, newAcc]()
                                    {
                                        ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                        if (!safePeer) return;

                                        if (!success || !newAcc.has_value())
                                        {
                                            SendLoginError(safePeer, "Impossible de creer le compte invite.");
                                            return;
                                        }

                                        std::string token = sessionManager.OnLogin(safePeer, static_cast<PlayerID>(newAcc->id), entt::null);
                                        SendLoginSuccess(safePeer, newAcc->id, "Bienvenue au nouveau joueur !", token);
                                    });
                                });
                        }
                    });
            });

        // --------------------------------------------------------------------
        // C2S_Reconnect (Reconnexion rapide via SessionToken)
        // --------------------------------------------------------------------
        dispatcher.RegisterHandler(Opcode_C2S_Reconnect,
            [accountRepo, runOnMainThread, &sessionManager](ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload)
            {
                auto reconnectReq = flatbuffers::GetRoot<Reconnect>(payload->data());
                if (!reconnectReq || !reconnectReq->session_token()) return;

                int accountId = reconnectReq->account_id();
                std::string token = reconnectReq->session_token()->str();

                if (!CheckRateLimit(peer))
                {
                    SendLoginError(peer, "Trop de tentatives.");
                    return;
                }

                // Validation direct en memoire (pas d'appel bdd)
                if (sessionManager.ValidateSessionToken(static_cast<PlayerID>(accountId), token))
                {
                    LOG_INFO("Reconnexion reussie pour AccountID {}", accountId);
                    accountRepo->UpdateLastLogin(accountId); // on update quand meme la date (optionnel)

                    // On passe par le main thread pour assigner la session
                    uint32_t peerID = peer->connectID;
                    runOnMainThread([&sessionManager, peerID, accountId]()
                    {
                        ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                        if (!safePeer) return;

                        // Regenere un nouveau token a chaque reconnexion pour la securite
                        std::string newToken = sessionManager.OnLogin(safePeer, static_cast<PlayerID>(accountId), entt::null);
                        SendLoginSuccess(safePeer, accountId, "Reconnexion reussie !", newToken);
                    });
                }
                else
                {
                    LOG_WARN("Session invalide ou expiree pour AccountID {}", accountId);
                    SendLoginError(peer, "Session invalide. Veuillez vous reconnecter.");
                }
            });

        // --------------------------------------------------------------------
        // C2S_BindAccount (Liaison d'un compte Invité vers Identifiants Classiques)
        // --------------------------------------------------------------------
        dispatcher.RegisterHandler(Opcode_C2S_BindAccount,
            [accountRepo, runOnMainThread, &sessionManager](ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload)
            {
                auto session = sessionManager.GetSession(peer);
                if (!session)
                {
                    SendBindAccountResult(peer, false, "Vous n'etes pas connecte.");
                    return;
                }

                auto bindReq = flatbuffers::GetRoot<BindAccount>(payload->data());
                if (!bindReq || !bindReq->username() || !bindReq->password()) return;

                std::string username = bindReq->username()->str();
                std::string password = bindReq->password()->str();

                if (!CheckRateLimit(peer))
                {
                    SendBindAccountResult(peer, false, "Trop de requetes. Veuillez patienter.");
                    return;
                }

                if (!std::regex_match(username, USERNAME_REGEX))
                {
                    SendBindAccountResult(peer, false, "Pseudo invalide (3-16 caracteres, alphanumerique).");
                    return;
                }

                if (password.length() < MIN_PASSWORD_LENGTH)
                {
                    SendBindAccountResult(peer, false, "Mot de passe trop court (4 caracteres minimum).");
                    return;
                }

                LOG_INFO("Requete de liaison de compte recu pour le pseudo: {}", username);
                uint32_t peerID = peer->connectID;
                int accountId = static_cast<int>(session->playerID);

                // 1. Verifier si le pseudo est deja pris
                accountRepo->GetAccountByUsername(username,
                    [accountRepo, runOnMainThread, peerID, accountId, username, password, &sessionManager](std::optional<Database::Account> account)
                    {
                        if (account.has_value())
                        {
                            runOnMainThread([peerID, &sessionManager]()
                            {
                                ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                if (safePeer) SendBindAccountResult(safePeer, false, "Ce pseudo est deja utilise.");
                            });
                            return;
                        }

                        // 2. Le pseudo est libre, on met a jour (bind)
                        std::string pwHash = Utils::PasswordHasher::Hash(password);
                        accountRepo->BindAccount(accountId, username, pwHash,
                            [runOnMainThread, peerID, username, &sessionManager](bool success)
                            {
                                runOnMainThread([peerID, &sessionManager, success, username]()
                                {
                                    ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                    if (!safePeer) return;

                                    if (success)
                                    {
                                        SendBindAccountResult(safePeer, true, "Compte '" + username + "' lie avec succes !");
                                        LOG_INFO("Liaison terminee. Le joueur sur le peer {} a lie son compte a '{}'.", safePeer->connectID, username);
                                    }
                                    else
                                    {
                                        SendBindAccountResult(safePeer, false, "Erreur serveur lors de la liaison de compte.");
                                    }
                                });
                            });
                    });
            });

        // --------------------------------------------------------------------
        // C2S_BindSocialAccount (Liaison d'un compte avec un fournisseur externe comme Google/Apple)
        // --------------------------------------------------------------------
        dispatcher.RegisterHandler(Opcode_C2S_BindSocialAccount,
            [accountRepo, runOnMainThread, &sessionManager](ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload)
            {
                auto session = sessionManager.GetSession(peer);
                if (!session)
                {
                    SendBindSocialAccountResult(peer, false, "Vous n'etes pas connecte.");
                    return;
                }

                auto bindReq = flatbuffers::GetRoot<BindSocialAccount>(payload->data());
                if (!bindReq || !bindReq->auth_provider() || !bindReq->provider_id()) return;

                std::string provider = bindReq->auth_provider()->str();
                std::string providerId = bindReq->provider_id()->str();
                // std::string idToken = bindReq->id_token() ? bindReq->id_token()->str() : ""; // Pour future validation

                if (!CheckRateLimit(peer))
                {
                    SendBindSocialAccountResult(peer, false, "Trop de requetes. Veuillez patienter.");
                    return;
                }

                if (provider.empty() || providerId.empty())
                {
                    SendBindSocialAccountResult(peer, false, "Informations de fournisseur invalides.");
                    return;
                }

                LOG_INFO("Requete de liaison Social recu, Fournisseur: {}, ID: {}", provider, providerId);
                uint32_t peerID = peer->connectID;
                int accountId = static_cast<int>(session->playerID);

                // 1. Verifier si ce social login n'est pas DEJA lie a un autre compte
                accountRepo->GetAccountBySocialId(provider, providerId,
                    [accountRepo, runOnMainThread, peerID, accountId, provider, providerId, &sessionManager](std::optional<Database::Account> account)
                    {
                        if (account.has_value())
                        {
                            runOnMainThread([peerID, provider, &sessionManager]()
                            {
                                ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                if (safePeer) SendBindSocialAccountResult(safePeer, false, "Ce compte " + provider + " est deja lie a un autre joueur.");
                            });
                            return;
                        }

                        // 2. Le social login est libre, on le lie au compte actuel
                        accountRepo->BindSocialAccount(accountId, provider, providerId,
                            [runOnMainThread, peerID, provider, &sessionManager](bool success)
                            {
                                runOnMainThread([peerID, &sessionManager, success, provider]()
                                {
                                    ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                                    if (!safePeer) return;

                                    if (success)
                                    {
                                        SendBindSocialAccountResult(safePeer, true, "Liaison " + provider + " reussie !");
                                        LOG_INFO("Liaison terminee. Le joueur sur le peer {} a lie son compte a {}.", safePeer->connectID, provider);
                                    }
                                    else
                                    {
                                        SendBindSocialAccountResult(safePeer, false, "Erreur serveur lors de la liaison " + provider + ".");
                                    }
                                });
                            });
                    });
            });

        // --------------------------------------------------------------------
        // C2S_SocialLogin (Connexion directe via un compte social)
        // --------------------------------------------------------------------
        dispatcher.RegisterHandler(Opcode_C2S_SocialLogin,
            [accountRepo, runOnMainThread, &sessionManager](ENetPeer* peer, const flatbuffers::Vector<uint8_t>* payload)
            {
                auto loginReq = flatbuffers::GetRoot<SocialLogin>(payload->data());
                if (!loginReq || !loginReq->auth_provider() || !loginReq->provider_id()) return;

                std::string provider = loginReq->auth_provider()->str();
                std::string providerId = loginReq->provider_id()->str();
                
                if (!CheckRateLimit(peer))
                {
                    SendLoginError(peer, "Trop de tentatives. Veuillez patienter.");
                    return;
                }

                LOG_INFO("Requete de Social Login recue ({} : {})", provider, providerId);
                uint32_t peerID = peer->connectID;

                // Rechercher le compte par ID social
                accountRepo->GetAccountBySocialId(provider, providerId,
                    [accountRepo, runOnMainThread, peerID, &sessionManager](std::optional<Database::Account> account)
                    {
                        runOnMainThread([accountRepo, peerID, account, &sessionManager]()
                        {
                            ENetPeer* safePeer = sessionManager.FindPeer(peerID);
                            if (!safePeer) return;

                            if (!account.has_value())
                            {
                                SendLoginError(safePeer, "Aucun compte n'est lie a ce login social.");
                                return;
                            }

                            // Compte trouve, on initie la session
                            std::string token = sessionManager.OnLogin(safePeer, static_cast<PlayerID>(account->id), entt::null);
                            accountRepo->UpdateLastLogin(account->id);
                            
                            SendLoginSuccess(safePeer, account->id, "Connexion social reussie !", token);
                            LOG_INFO("Joueur {} connecte via Social Login. ID: {}", account->username, account->id);
                        });
                    });
            });
    }
}
