#pragma once
#include <random>


namespace MMO::Math
{
    class Random
    {
    public:
        [[nodiscard]] static int GetInt(int min, int max)
        {
            std::uniform_int_distribution<int> dist(min, max);
            return dist(GetEngine());
        }

        [[nodiscard]] static float GetFloat(float min, float max)
        {
            std::uniform_real_distribution<float> dist(min, max);
            return dist(GetEngine());
        }

    private:
        static std::mt19937& GetEngine()
        {
            thread_local std::mt19937 engine(std::random_device{}());
            return engine;
        }
    };
}