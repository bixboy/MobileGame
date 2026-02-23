#pragma once
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <SQLiteCpp/SQLiteCpp.h>
#include "database/ConcurrentQueue.h"


namespace MMO::Database 
{
    // Type d'un job a executer sur le thread de la base de donnees
    using DatabaseJob = std::function<void(SQLite::Database&)>;
    
    // Gere la connexion SQLite et execute les jobs sur un thread dedie
    class DatabaseManager 
    {
    public:
        DatabaseManager();
        ~DatabaseManager();

        // Ouvre la base de donnees et demarre le worker thread
        bool Initialize(const std::string& dbPath);

        // Arrete le worker thread et ferme la connexion
        void Shutdown();

        // Ajoute un job a executer de maniere asynchrone
        void EnqueueJob(DatabaseJob job);

        // Cree les tables si elles n'existent pas encore
        void InitializeSchema();

    private:
        // Boucle du thread de travail (consomme les jobs)
        void WorkerThreadMain();

        std::unique_ptr<SQLite::Database> m_db;
        std::thread m_workerThread;
        std::atomic<bool> m_isRunning;
        MMO::Core::ConcurrentQueue<DatabaseJob> m_jobQueue;
    };
}
