#pragma once
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <entt/entt.hpp>
#include <cstdint>


namespace MMO::Core
{
    // Grille spatiale pour les requetes d'Area of Interest (AOI)
    // Insert/Remove/Move en O(1), Query en O(k) ou k = entites dans les cellules voisines
    class SpatialGrid
    {
    public:
        explicit SpatialGrid(float cellSize = 100.0f);

        // Insere une entite a la position donnee
        void Insert(entt::entity entity, float x, float y);

        // Supprime une entite de la grille
        void Remove(entt::entity entity);

        // Deplace une entite — re-hash seulement si elle change de cellule
        void Move(entt::entity entity, float newX, float newY);

        // Retourne toutes les entites dans les cellules voisines (3x3 autour)
        void QueryNeighbors(float x, float y, std::vector<entt::entity>& out) const;

        // Vide la grille completement
        void Clear();

    private:
        // Hash 2D → clef unique pour une cellule
        int64_t CellKey(int cx, int cy) const;

        // Coordonnees monde → indices de cellule
        int ToCellX(float x) const;
        int ToCellY(float y) const;

        float m_cellSize;
        float m_inverseCellSize; // Pre-calcule pour eviter la division a chaque frame

        // Cellule → ensemble d'entites
        std::unordered_map<int64_t, std::unordered_set<entt::entity>> m_cells;

        // Entite → sa cellule actuelle (pour suppression O(1))
        std::unordered_map<entt::entity, int64_t> m_entityToCell;
    };
}
