#pragma once
#include <chrono>


namespace MMO::Time 
{
    class Stopwatch 
    {
    public:
        Stopwatch()
        {
            Reset();
        }

        void Reset() 
        {
            m_startTime = std::chrono::high_resolution_clock::now();
        }

        [[nodiscard]] float ElapsedMilliseconds() const 
        {
            auto endTime = std::chrono::high_resolution_clock::now();
            return std::chrono::duration<float, std::milli>(endTime - m_startTime).count();
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
    };

}