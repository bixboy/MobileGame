#include "core/ServerCommands.h"
#include "utils/Logger.h"
#include <filesystem>


namespace MMO::Core
{
    void RegisterServerCommands(CommandSystem& commandSystem, CommandContext ctx)
    {
        // deletedb [nom] - Supprime la DB et arrete le serveur
        commandSystem.Register("deletedb", "Supprime la DB et arrete le serveur. Usage: deletedb [nom.db] ou deletedb all",
            [ctx](const std::vector<std::string>& args)
            {
                if (args.empty())
                {
                    LOG_WARN("Usage: deletedb <nom.db> ou deletedb all");
                    return;
                }

                // Arreter le serveur en premier — libere tous les handles DB proprement
                LOG_WARN("Arret du serveur avant suppression de la DB...");
                ctx.stopServer();

                // Apres l'arret, supprimer les fichiers DB
                auto deleteDbFile = [](const std::filesystem::path& path)
                {
                    std::error_code ec;
                    std::filesystem::remove(path, ec);
                    std::filesystem::remove(std::filesystem::path(path.string() + "-wal"), ec);
                    std::filesystem::remove(std::filesystem::path(path.string() + "-shm"), ec);
                };

                if (args[0] == "all")
                {
                    int count = 0;
                    for (const auto& entry : std::filesystem::directory_iterator("."))
                    {
                        if (entry.path().extension() == ".db")
                        {
                            LOG_INFO("Supprime: {}", entry.path().string());
                            deleteDbFile(entry.path());
                            count++;
                        }
                    }
                    LOG_INFO("{} fichier(s) .db supprime(s). Relancez le serveur.", count);
                }
                else
                {
                    std::string filename = args[0];
                    if (std::filesystem::exists(filename))
                    {
                        deleteDbFile(filename);
                        LOG_INFO("Supprime: {}. Relancez le serveur.", filename);
                    }
                    else
                    {
                        LOG_WARN("Fichier introuvable: {}", filename);
                    }
                }
            });

        // stop - Arrete le serveur proprement
        commandSystem.Register("stop", "Arrete le serveur proprement",
            [ctx](const std::vector<std::string>&)
            {
                LOG_INFO("Arret du serveur demande via commande...");
                ctx.stopServer();
            });
    }
}
