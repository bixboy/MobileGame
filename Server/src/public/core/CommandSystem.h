#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <sstream>
#include <vector>
#include "database/ConcurrentQueue.h"


namespace MMO::Core
{
    // Systeme de commandes console pour le serveur
    // Lit stdin dans un thread dedie et execute les commandes sur le thread principal
    class CommandSystem
    {
    public:
        using CommandFunc = std::function<void(const std::vector<std::string>& args)>;

        CommandSystem() : m_isRunning(false) {}
        ~CommandSystem() { Stop(); }

        // Demarre la lecture de stdin dans un thread dedie
        void Start();

        // Arrete le thread de lecture
        void Stop();

        // Enregistre une commande avec son handler
        void Register(const std::string& name, const std::string& description, CommandFunc handler);

        // Traite les commandes en attente (appele depuis le thread principal)
        void ProcessPending();

    private:
        // Boucle de lecture stdin (thread dedie)
        void ReaderThread();

        // Affiche la liste des commandes disponibles
        void PrintHelp();

        // Decoupe une ligne en tokens
        static std::vector<std::string> Tokenize(const std::string& line);

        struct CommandEntry
        {
            std::string description;
            CommandFunc handler;
        };

        std::unordered_map<std::string, CommandEntry> m_commands;
        ConcurrentQueue<std::string> m_pendingCommands;
        std::thread m_readerThread;
        std::atomic<bool> m_isRunning;
    };
}
