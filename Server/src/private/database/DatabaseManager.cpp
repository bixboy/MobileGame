#include "database/DatabaseManager.h"
#include "utils/Logger.h"


namespace MMO::Database 
{
    DatabaseManager::DatabaseManager() : m_isRunning(false) 
    {
    }

    DatabaseManager::~DatabaseManager() 
    {
        Shutdown();
    }

    bool DatabaseManager::Initialize(const std::string& dbPath) 
    {
        try 
        {
            // Ouverture (ou creation) du fichier de base de donnees
            m_db = std::make_unique<SQLite::Database>(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
            LOG_INFO("SQLite database initialized at: {}", dbPath);

            InitializeSchema();

            // Demarrage du thread dedie aux operations DB
            m_isRunning = true;
            m_workerThread = std::thread(&DatabaseManager::WorkerThreadMain, this);
            
            return true;
        } 
        catch (const std::exception& e) 
        {
            LOG_ERROR("Erreur lors de l'initialisation de la base de donnees: {}", e.what());
            return false;
        }
    }

    void DatabaseManager::Shutdown() 
    {
        if (m_isRunning) 
        {
            LOG_INFO("Arret du DatabaseManager... Attente de la fin des operations.");
            
            // Le job de reveil doit etre pousse AVANT de couper m_isRunning
            // sinon EnqueueJob le rejette et le worker reste bloque sur WaitAndPop
            m_jobQueue.Push([](SQLite::Database&) {});
            m_isRunning = false;
            
            if (m_workerThread.joinable()) 
                m_workerThread.join();
            
            LOG_INFO("DatabaseManager arrete avec succes.");
        }
    }

    void DatabaseManager::EnqueueJob(DatabaseJob job) 
    {
        if (m_isRunning) 
        {
            m_jobQueue.Push(std::move(job));
        }
    }

    // Boucle du worker : attend et execute les jobs un par un
    void DatabaseManager::WorkerThreadMain() 
    {
        LOG_INFO("Database Worker Thread demarre.");
        
        while (m_isRunning) 
        {
            auto job = m_jobQueue.WaitAndPop();
            
            if (!m_isRunning)
                break;
                
            try 
            {
                if (m_db && job) 
                {
                    job(*m_db);
                }
            } 
            catch (const std::exception& e) 
            {
                LOG_ERROR("Erreur dans le Database Worker Thread: {}", e.what());
            }
        }
    }

    // Creation du schema (tables, pragmas)
    void DatabaseManager::InitializeSchema() 
    {
        try 
        {
            m_db->exec("PRAGMA journal_mode=WAL;");
            m_db->exec("PRAGMA synchronous=NORMAL;");

            // Table des comptes joueurs
            m_db->exec(R"(
                CREATE TABLE IF NOT EXISTS accounts (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    username TEXT UNIQUE NOT NULL,
                    password_hash TEXT,
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                    last_login_at DATETIME
                )
            )");

            // Table des donnees joueur (position, ressources) — une entree par (account, kingdom)
            m_db->exec(R"(
                CREATE TABLE IF NOT EXISTS player_data (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    account_id INTEGER NOT NULL,
                    kingdom_id INTEGER NOT NULL,
                    pos_x REAL DEFAULT 0.0,
                    pos_y REAL DEFAULT 0.0,
                    food INTEGER DEFAULT 500,
                    wood INTEGER DEFAULT 500,
                    stone INTEGER DEFAULT 200,
                    gold INTEGER DEFAULT 100,
                    FOREIGN KEY (account_id) REFERENCES accounts(id),
                    UNIQUE(account_id, kingdom_id)
                )
            )");
            
            LOG_INFO("Database schema verifie.");
        } 
        catch (const std::exception& e) 
        {
            LOG_ERROR("Erreur lors de l'initialisation du schema: {}", e.what());
            throw;
        }
    }
}
