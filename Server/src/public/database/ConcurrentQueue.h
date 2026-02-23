#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace MMO::Core 
{
    // File d'attente thread-safe (producteur/consommateur)
    template <typename T>
    class ConcurrentQueue 
    {
    public:
        ConcurrentQueue() = default;
        ~ConcurrentQueue() = default;

        // Non copiable
        ConcurrentQueue(const ConcurrentQueue&) = delete;
        ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

        // Ajoute un element et reveille un thread en attente
        void Push(T item) 
        {
            {
                std::scoped_lock lock(m_mutex);
                m_queue.push(std::move(item));
            }
            m_condVar.notify_one();
        }

        // Tente de recuperer un element (non-bloquant)
        std::optional<T> TryPop() 
        {
            std::scoped_lock lock(m_mutex);
            if (m_queue.empty()) 
            {
                return std::nullopt;
            }

            T item = std::move(m_queue.front());
            m_queue.pop();
            return item;
        }

        // Recupere un element (bloquant jusqu'a disponibilite)
        T WaitAndPop() 
        {
            std::unique_lock lock(m_mutex);
            m_condVar.wait(lock, [this]() { return !m_queue.empty(); });

            T item = std::move(m_queue.front());
            m_queue.pop();
            return item;
        }

        // Verifie si la file est vide
        bool IsEmpty() const 
        {
            std::scoped_lock lock(m_mutex);
            return m_queue.empty();
        }

    private:
        std::queue<T> m_queue;
        mutable std::mutex m_mutex;
        std::condition_variable m_condVar;
    };
}
