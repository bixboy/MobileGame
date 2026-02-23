#pragma once
#include <random>


namespace MMO::Math
{
    // Generateur de nombres aleatoires (thread-safe via thread_local)
    class Random
    {
    public:
        // Retourne un entier aleatoire dans [min, max]
        [[nodiscard]] static int GetInt(int min, int max)
        {
            std::uniform_int_distribution<int> dist(min, max);
            return dist(GetEngine());
        }

        // Retourne un float aleatoire dans [min, max]
        [[nodiscard]] static float GetFloat(float min, float max)
        {
            std::uniform_real_distribution<float> dist(min, max);
            return dist(GetEngine());
        }

    private:
        // Moteur Mersenne Twister par thread (evite les locks)
        static std::mt19937& GetEngine()
        {
            thread_local std::mt19937 engine(std::random_device{}());
            return engine;
        }
    };
}