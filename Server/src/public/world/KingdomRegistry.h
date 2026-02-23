#pragma once
#include <string>
#include <vector>
#include <unordered_map>


namespace MMO::Core
{
    struct KingdomInfo
    {
        int id = -1;
        std::string name;
        std::string ip;
        uint16_t port = 0;
        int maxPlayers = 1000;
        int playerCount = 0;    // Dynamique, mis a jour par les kingdoms
        uint8_t status = 0;     // 0=offline, 1=online, 2=full, 3=maintenance
    };

    // Registre des royaumes — charge depuis JSON, rechargeable a chaud
    class KingdomRegistry
    {
    public:
        // Charge la liste depuis un fichier JSON
        bool LoadFromFile(const std::string& path);

        // Recharge sans redemarrer
        bool Reload();

        // Acces O(1) par ID
        const KingdomInfo* GetById(int id) const;

        // Liste complete
        const std::vector<KingdomInfo>& GetAll() const { return m_kingdoms; }

    private:
        std::vector<KingdomInfo> m_kingdoms;
        std::unordered_map<int, size_t> m_idToIndex;  // ID → index dans m_kingdoms
        std::string m_filePath;
    };
}
