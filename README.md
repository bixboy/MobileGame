# 🏰 MMO MobileGame

> Serveur C++ haute-performance + client Unity pour un jeu mobile RTS multijoueur
> inspiré de **Rise of Kingdoms**.

=============
### Sommaire
=============

| Section                                           | Description                     |
|---------------------------------------------------|---------------------------------|
| [📋 Architecture](#-architecture)                 | Structure du projet             |
| [🔧 Prérequis](#-prérequis)                       | Outils nécessaires              |
| [🚀 Démarrage rapide](#-démarrage-rapide)         | Compiler, configurer, lancer    |
| [🏗️ Architecture serveur](#-architecture-serveur) | Single-process, flow réseau, DB |
| [📡 Guide FlatBuffers](#-guide-flatbuffers)       | Ajouter un message réseau       |
| [🔌 Créer un Handler](#-créer-un-nouveau-handler) | Ajouter un handler de A à Z     |
| [🛡️ Sécurité](#-sécurité)                         | Mesures de protection           |
| [🎮 Commandes console](#-commandes-console)       | Commandes admin serveur         |
| [📦 Dépendances](#-dépendances)                   | Bibliothèques utilisées         |

---

====================
## 📋 Architecture
====================

```
MobileGame/
├── Server/              ← Serveur C++ (xmake)
│   ├── src/
│   │   ├── public/      ← Headers (.h)
│   │   │   ├── core/    ← Config, GameLoop, CommandSystem
│   │   │   ├── world/   ← KingdomWorld, SpatialGrid, IGameSystem
│   │   │   ├── ecs/     ← Composants ECS (PlayerComponents)
│   │   │   ├── database/← DatabaseManager, Repositories
│   │   │   ├── network/ ← NetworkManager, Handlers, SessionManager
│   │   │   ├── math/    ← Vector2, Random
│   │   │   └── utils/   ← Logger, PasswordHasher, Time
│   │   ├── private/     ← Implémentations (.cpp)
│   │   └── main.cpp     ← Point d'entrée
│   ├── proto/           ← FlatBuffers
│   │   ├── schemas/     ← Fichiers .fbs par domaine
│   │   ├── generated/   ← Code généré (gitignored)
│   │   └── GenerateProto.bat
│   ├── vendor/          ← Dépendances tierces
│   └── xmake.lua        ← Build system
│
└── MMO_MobileGame/      ← Projet Unity (client mobile)
    └── Assets/Scripts/
        ├── Network/     ← NetworkClient + FlatBuffers générés
        └── UI/          ← LoginUI, KingdomUI, ResourceUI
```

---
=================
## 🔧 Prérequis
=================

| Outil           | Version              |
|-----------------|----------------------|
| **xmake**       | ≥ 2.8                |
| **MSVC**        | Visual Studio 2022   |
| **Unity**       | 2022.3+ LTS          |
| **FlatBuffers** | `flatc` dans le PATH |
| **libsodium**   | Installé via xmake   |

---

========================
## 🚀 Démarrage rapide
========================

===========================
### 1. Compiler le serveur
===========================

```bash
cd Server
xmake build
```

===============================
### 2. Configurer les royaumes
===============================

Éditer `Server/kingdoms.json` :

```json
{
  "kingdoms": [
    { "id": 1, "name": "Avalon",  "maxPlayers": 1000 },
    { "id": 2, "name": "Midgard", "maxPlayers": 1000 }
  ]
}
```

=========================
### 3. Lancer le serveur
=========================

```bash
xmake run
```

Options disponibles :

| Argument            | Défaut          | Description |
|---------------------|-----------------|-------------|
| `--port`            | `7777`          | Port d'écoute ENet |
| `--db`              | `game.db`       | Chemin vers la base SQLite |
| `--kingdoms-config` | `kingdoms.json` | Fichier de configuration des royaumes |
| `--tick-rate`       | `20`            | Fréquence du tick serveur (Hz) |
| `--max-players`     | `100`           | Nombre max de connexions |


==============================
### 4. Lancer le client Unity
==============================

Ouvrir `MMO_MobileGame/` dans Unity, ouvrir la scène principale et appuyer sur **Play**.

---

============================
## 🏗️ Architecture serveur
============================

### Single-Process Multi-Kingdom

Le serveur fonctionne en **un seul processus**. Chaque royaume est une partition logique
avec sa propre ECS registry, sa grille spatiale, et ses systèmes de gameplay.

```
Client ──connexion unique──▶ Serveur :7777
                                ├── KingdomWorld #1 (Avalon)
                                │     ├── entt::registry
                                │     ├── SpatialGrid (AOI)
                                │     └── IGameSystem[]
                                │
                                └── KingdomWorld #2 (Midgard)
                                      ├── entt::registry
                                      ├── SpatialGrid (AOI)
                                      └── IGameSystem[]
```

**Points clés :**
- **Zéro reconnexion** — le client maintient une connexion unique du login au gameplay
- **Ressources par royaume** — clé composite `(account_id, kingdom_id)` en DB
- **Extensible** — ajouter du gameplay = implémenter `IGameSystem`

================
### Flow réseau
================

```
1. Connect → C2S_Login → S2C_LoginResult
2. C2S_RequestKingdoms → S2C_KingdomList
3. C2S_SelectKingdom → charge profil DB → crée entité ECS → S2C_PlayerData
4. Gameplay (C2S_ModifyResources → S2C_ResourceUpdate, etc.)
```

====================
### Base de données
====================

SQLite en mode WAL avec un **worker thread dédié**. Toutes les opérations DB sont
asynchrones (fire-and-forget pour les écritures, callback pour les lectures).

| Table         | Clé                        | Description                               |
|---------------|----------------------------|-------------------------------------------|
| `accounts`    | `id`                       | Comptes joueurs (username, password_hash) |
| `player_data` | `(account_id, kingdom_id)` | Profil par royaume (position, ressources) |


==================
### Sérialisation
==================

**FlatBuffers** avec une enveloppe universelle (`Opcode + payload`).

=========================
## 📡 Guide FlatBuffers
=========================

Les messages réseau sont définis dans des fichiers `.fbs` séparés par domaine dans `Server/proto/schemas/`.
Le script `GenerateProto.bat` compile tous les schémas et copie automatiquement les fichiers C# vers Unity.

| Fichier          | Contenu                                           |
|------------------|---------------------------------------------------|
| `Core.fbs`       | Opcode (enum central), Envelope, Ping/Pong        |
| `Auth.fbs`       | Login, LoginResult                                |
| `Kingdom.fbs`    | KingdomEntry, KingdomList, SelectKingdom, Request |
| `Resources.fbs`  | PlayerData, ResourceType, ModifyResources, Update |
| `Movement.fbs`   | MoveRequest, MovementSnapshot                     |

### Ajouter un nouveau message

===========================================================================
**Étape 1** — Ajouter l'opcode dans l'enum `Opcode` de `Core.fbs` :
===========================================================================

```c++
enum Opcode : ushort
{
    // ...existants...
    C2S_BuildRequest = 200,  // Client → Serveur
    S2C_BuildConfirm = 201,  // Serveur → Client
}
```

> Convention : `C2S_` = Client-to-Server, `S2C_` = Server-to-Client.
> Les plages sont groupées par système (1-999 général, 1000-1999 mouvement, 2000-2999 combat...).

==============================================================================
**Étape 2** — Créer un nouveau fichier `.fbs` (ou ajouter dans un existant) :
==============================================================================

Créer `schemas/Building.fbs` :

```c++
include "Core.fbs";

namespace MMO.Network;

table BuildRequest
{
    building_type: ubyte;
    pos_x: float;
    pos_y: float;
}

table BuildConfirm
{
    success: bool;
    building_id: int;
}
```

==================================
**Étape 3** — Régénérer le code :
==================================

```bash
cd Server/proto
.\GenerateProto.bat
```

Ce script :
1. Cherche `flatc.exe` dans le cache xmake
2. Compile tous les `schemas/*.fbs`
3. Génère les headers C++ (`*_generated.h`) dans `generated/`
4. **Copie automatiquement** les classes C# vers Unity (`Scripts/Network/Generated/`)

===================================
### Structure des fichiers générés
===================================

```
Server/proto/
├── GenerateProto.bat            ← Script de génération
├── schemas/                     ← Fichiers source .fbs
│   ├── Core.fbs                 ← Opcode, Envelope, Ping/Pong
│   ├── Auth.fbs                 ← Login, LoginResult
│   ├── Kingdom.fbs              ← KingdomEntry, SelectKingdom
│   ├── Resources.fbs            ← PlayerData, ModifyResources
│   └── Movement.fbs             ← MoveRequest, MovementSnapshot
└── generated/                   ← Fichiers générés (gitignored)
    ├── Core_generated.h         ← C++
    ├── Auth_generated.h
    ├── Kingdom_generated.h
    ├── Resources_generated.h
    ├── Movement_generated.h
    └── MMO/Network/*.cs         ← C# (auto-copié vers Unity)
```

---

================================
## 🔌 Créer un nouveau Handler

Un handler est une fonction qui traite un opcode spécifique. Voici comment en créer un
de A à Z (exemple : `C2S_BuildRequest`).

### Étape 1 — Créer le header
==============================

`src/public/network/handlers/BuildHandler.h` :

```cpp
#pragma once
#include "network/PacketDispatcher.h"
#include "network/SessionManager.h"
#include <memory>
#include <unordered_map>

namespace MMO::Core { class KingdomWorld; }

namespace MMO::Network
{
    void RegisterBuildHandler(PacketDispatcher& dispatcher,
        SessionManager& sessionManager,
        std::unordered_map<int, std::unique_ptr<MMO::Core::KingdomWorld>>& kingdoms);
}
```

=====================================
### Étape 2 — Créer l'implémentation
=====================================

`src/private/network/handlers/BuildHandler.cpp` :

```cpp
#include "network/handlers/BuildHandler.h"
#include "network/PacketBuilder.h"
#include "world/KingdomWorld.h"
#include "NetworkCore_generated.h"
#include "utils/Logger.h"

namespace MMO::Network
{
    void RegisterBuildHandler(PacketDispatcher& dispatcher, SessionManager& sessionManager,
        std::unordered_map<int, std::unique_ptr<MMO::Core::KingdomWorld>>& kingdoms)
    {
        dispatcher.RegisterHandler(Opcode_C2S_BuildRequest,
            [&sessionManager, &kingdoms](ENetPeer* peer,
                const flatbuffers::Vector<uint8_t>* payload)
            {
                // 1. Deserialiser le message
                auto req = flatbuffers::GetRoot<BuildRequest>(payload->data());
                if (!req)
                    return;

                // 2. Verifier l'authentification
                auto* session = sessionManager.GetSession(peer);
                if (!session || session->kingdomId < 0)
                    return;

                // 3. Acceder au bon royaume
                auto kIt = kingdoms.find(session->kingdomId);
                if (kIt == kingdoms.end())
                    return;

                auto& registry = kIt->second->GetRegistry();

                // 4. Logique metier...
                LOG_INFO("Build request: type={} pos=({}, {})",
                    req->building_type(), req->pos_x(), req->pos_y());

                // 5. Repondre au client
                PacketBuilder::SendResponse(peer, Opcode_S2C_BuildConfirm,
                    [](flatbuffers::FlatBufferBuilder& fbb)
                    {
                        BuildConfirmBuilder builder(fbb);
                        builder.add_success(true);
                        builder.add_building_id(42);
                        fbb.Finish(builder.Finish());
                    });
            });
    }
}
```

========================================
### Étape 3 — Enregistrer dans GameLoop
========================================

Dans `GameLoop.cpp`, méthode `RegisterHandlers()` :

```cpp
#include "network/handlers/BuildHandler.h"

// Dans RegisterHandlers() :
MMO::Network::RegisterBuildHandler(dispatcher, sessionManager, m_kingdoms);
```

========================================
### Étape 4 — Gérer côté client (Unity)
========================================

Dans `NetworkClient.cs`, ajouter dans le `switch` de `HandleReceive` :

```csharp
case Opcode.S2C_BuildConfirm:
    HandleBuildConfirm(envelope.GetPayloadDataArray());
    break;
```

=====================
### Checklist rapide
=====================

| # | Fichier                          | Action                          |
|---|----------------------------------|---------------------------------|
| 1 | `proto/NetworkCore.fbs`          | Ajouter opcode + table          |
| 2 | `proto/GenerateProto.bat`        | Exécuter pour régénérer         |
| 3 | `handlers/BuildHandler.h`        | Déclarer `RegisterBuildHandler` |
| 4 | `handlers/BuildHandler.cpp`      | Implémenter le handler          |
| 5 | `GameLoop.cpp`                   | Appeler `RegisterBuildHandler`  |
| 6 | `NetworkClient.cs`               | Traiter la réponse côté client  |

---

================
## 🛡️ Sécurité
================

- **Mots de passe** — hashés avec libsodium (Argon2id)
- **Rate limiting** — protection contre le brute-force login
- **Validation serveur** — delta de ressources limité (`±1000`), vérification d'authentification sur chaque handler
- **Aucun trust client** — toute la logique est côté serveur

---

========================
## 🎮 Commandes console
========================

| Commande            | Description                                      |
|---------------------|--------------------------------------------------|
| `help`              | Liste toutes les commandes                       |
| `stop`              | Arrête le serveur proprement                     |
| `deletedb all`      | Supprime toutes les DB et arrête le serveur      |
| `deletedb game.db`  | Supprime une DB spécifique et arrête le serveur  |

---

====================
## 📦 Dépendances
====================

| Bibliothèque                                         | Usage                            |
|------------------------------------------------------|----------------------------------|
| [ENet](https://github.com/nxrighthere/ENet-CSharp)   | Transport UDP fiable             |
| [FlatBuffers](https://google.github.io/flatbuffers/) | Sérialisation zero-copy          |
| [EnTT](https://github.com/skypjack/entt)             | Entity Component System          |
| [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp)  | Wrapper SQLite C++               |
| [libsodium](https://doc.libsodium.org/)              | Cryptographie (hash passwords)   |
| [nlohmann/json](https://github.com/nlohmann/json)    | Parser JSON (kingdoms.json)      |
| [spdlog](https://github.com/gabime/spdlog)           | Logging structuré                |

---

===============
## 📝 License
===============

Projet privé — tous droits réservés.
