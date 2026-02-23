#include "world/KingdomRegistry.h"
#include "utils/Logger.h"
#include <fstream>
#include <nlohmann/json.hpp>


namespace MMO::Core
{
    bool KingdomRegistry::LoadFromFile(const std::string& path)
    {
        m_filePath = path;
        return Reload();
    }

    bool KingdomRegistry::Reload()
    {
        std::ifstream file(m_filePath);
        if (!file.is_open())
        {
            LOG_ERROR("Impossible d'ouvrir le fichier kingdoms: {}", m_filePath);
            return false;
        }

        try
        {
            nlohmann::json json;
            file >> json;

            std::vector<KingdomInfo> newKingdoms;
            std::unordered_map<int, size_t> newIndex;

            for (const auto& entry : json)
            {
                KingdomInfo info;
                info.id         = entry.at("id").get<int>();
                info.name       = entry.at("name").get<std::string>();
                info.ip         = entry.at("ip").get<std::string>();
                info.port       = entry.at("port").get<uint16_t>();
                info.maxPlayers = entry.value("maxPlayers", 1000);
                info.status     = 1; // Online par defaut

                newIndex[info.id] = newKingdoms.size();
                newKingdoms.push_back(std::move(info));
            }

            m_kingdoms = std::move(newKingdoms);
            m_idToIndex = std::move(newIndex);

            LOG_INFO("KingdomRegistry charge: {} royaume(s) depuis '{}'", m_kingdoms.size(), m_filePath);
            return true;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Erreur de parsing du fichier kingdoms: {}", e.what());
            return false;
        }
    }

    const KingdomInfo* KingdomRegistry::GetById(int id) const
    {
        auto it = m_idToIndex.find(id);
        if (it != m_idToIndex.end())
            return &m_kingdoms[it->second];
        return nullptr;
    }
}
