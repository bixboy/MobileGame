#include "world/SpatialGrid.h"
#include <cmath>


namespace MMO::Core
{
    SpatialGrid::SpatialGrid(float cellSize)
        : m_cellSize(cellSize)
        , m_inverseCellSize(1.0f / cellSize)
    {
    }

    int SpatialGrid::ToCellX(float x) const
    {
        return static_cast<int>(std::floor(x * m_inverseCellSize));
    }

    int SpatialGrid::ToCellY(float y) const
    {
        return static_cast<int>(std::floor(y * m_inverseCellSize));
    }

    int64_t SpatialGrid::CellKey(int cx, int cy) const
    {
        // Combine 2 coordonnees 32-bit en une clef 64-bit unique
        return (static_cast<int64_t>(cx) << 32) | static_cast<uint32_t>(cy);
    }

    void SpatialGrid::Insert(entt::entity entity, float x, float y)
    {
        int64_t key = CellKey(ToCellX(x), ToCellY(y));
        m_cells[key].insert(entity);
        m_entityToCell[entity] = key;
    }

    void SpatialGrid::Remove(entt::entity entity)
    {
        auto it = m_entityToCell.find(entity);
        if (it == m_entityToCell.end())
            return;

        int64_t key = it->second;
        auto cellIt = m_cells.find(key);
        if (cellIt != m_cells.end())
        {
            cellIt->second.erase(entity);
            if (cellIt->second.empty())
                m_cells.erase(cellIt);
        }

        m_entityToCell.erase(it);
    }

    void SpatialGrid::Move(entt::entity entity, float newX, float newY)
    {
        int64_t newKey = CellKey(ToCellX(newX), ToCellY(newY));

        auto it = m_entityToCell.find(entity);
        if (it == m_entityToCell.end())
        {
            // Pas encore dans la grille — insertion directe
            Insert(entity, newX, newY);
            return;
        }

        int64_t oldKey = it->second;
        if (oldKey == newKey)
            return; // Meme cellule — rien a faire

        // Supprime de l'ancienne cellule
        auto cellIt = m_cells.find(oldKey);
        if (cellIt != m_cells.end())
        {
            cellIt->second.erase(entity);
            if (cellIt->second.empty())
                m_cells.erase(cellIt);
        }

        // Insere dans la nouvelle cellule
        m_cells[newKey].insert(entity);
        it->second = newKey;
    }

    void SpatialGrid::QueryNeighbors(float x, float y, std::vector<entt::entity>& out) const
    {
        int cx = ToCellX(x);
        int cy = ToCellY(y);

        // Parcourt les 9 cellules autour (3x3)
        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                int64_t key = CellKey(cx + dx, cy + dy);
                auto it = m_cells.find(key);
                if (it != m_cells.end())
                {
                    for (entt::entity e : it->second)
                    {
                        out.push_back(e);
                    }
                }
            }
        }
    }

    void SpatialGrid::Clear()
    {
        m_cells.clear();
        m_entityToCell.clear();
    }
}
