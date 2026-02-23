#pragma once
#include <chrono>


namespace MMO::Time 
{
    // Chronometre haute precision pour mesurer les durees
    class Stopwatch 
    {
    public:
        Stopwatch()
        {
            Reset();
        }

        // Reinitialise le chronometre
        void Reset() 
        {
            m_startTime = std::chrono::steady_clock::now();
        }

        // Retourne le temps ecoule en millisecondes
        [[nodiscard]] float ElapsedMilliseconds() const 
        {
            auto endTime = std::chrono::steady_clock::now();
            return std::chrono::duration<float, std::milli>(endTime - m_startTime).count();
        }

    private:
        std::chrono::time_point<std::chrono::steady_clock> m_startTime;
    };
}