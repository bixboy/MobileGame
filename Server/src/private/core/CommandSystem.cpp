#include "core/CommandSystem.h"
#include "utils/Logger.h"
#include <iostream>


namespace MMO::Core
{
    void CommandSystem::Start()
    {
        if (m_isRunning)
            return;

        // Commande help par defaut
        Register("help", "Affiche la liste des commandes", [this](const std::vector<std::string>&)
        {
            PrintHelp();
        });

        m_isRunning = true;
        m_readerThread = std::thread(&CommandSystem::ReaderThread, this);

        LOG_INFO("Systeme de commandes demarre. Tapez 'help' pour la liste.");
    }

    void CommandSystem::Stop()
    {
        if (!m_isRunning)
            return;

        m_isRunning = false;

        // Le thread est bloque sur getline, il se terminera au prochain input ou a la fermeture de stdin
        if (m_readerThread.joinable())
            m_readerThread.detach();
    }

    void CommandSystem::Register(const std::string& name, const std::string& description, CommandFunc handler)
    {
        m_commands[name] = { description, std::move(handler) };
    }

    // Traite les commandes en attente sur le thread principal
    void CommandSystem::ProcessPending()
    {
        while (auto cmdOpt = m_pendingCommands.TryPop())
        {
            auto tokens = Tokenize(cmdOpt.value());
            if (tokens.empty())
                continue;

            std::string cmdName = tokens[0];
            std::vector<std::string> args(tokens.begin() + 1, tokens.end());

            auto it = m_commands.find(cmdName);
            if (it != m_commands.end())
            {
                it->second.handler(args);
            }
            else
            {
                LOG_WARN("Commande inconnue: '{}'. Tapez 'help' pour la liste.", cmdName);
            }
        }
    }

    // Thread dedie a la lecture de stdin
    void CommandSystem::ReaderThread()
    {
        std::string line;
        while (m_isRunning && std::getline(std::cin, line))
        {
            if (line.empty())
                continue;

            m_pendingCommands.Push(line);
        }
    }

    void CommandSystem::PrintHelp()
    {
        LOG_INFO("=== Commandes disponibles ===");
        for (const auto& [name, entry] : m_commands)
        {
            LOG_INFO("  {} - {}", name, entry.description);
        }
        LOG_INFO("=============================");
    }

    std::vector<std::string> CommandSystem::Tokenize(const std::string& line)
    {
        std::vector<std::string> tokens;
        std::istringstream stream(line);
        std::string token;

        while (stream >> token)
        {
            tokens.push_back(token);
        }

        return tokens;
    }
}
