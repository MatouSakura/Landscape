#include "TerrainHeightmapTileSet.hpp"

#include <algorithm>

namespace Diligent
{

void TerrainHeightmapTileSet::Build(const TerrainHeightmapTileSetDesc& Desc)
{
    m_Desc.TileCountX             = std::max(Desc.TileCountX, 1u);
    m_Desc.TileCountZ             = std::max(Desc.TileCountZ, 1u);
    m_Desc.TileSampleCountPerAxis = std::max(Desc.TileSampleCountPerAxis, 2u);
    m_Desc.Extent                 = std::max(Desc.Extent, 1.0f);

    const Uint32 TileCellCount = m_Desc.TileSampleCountPerAxis - 1u;
    const float  WorldMin      = -m_Desc.Extent;
    const float  PackageWorldSize = 2.0f * m_Desc.Extent;
    const float  TileWorldSizeX = PackageWorldSize / static_cast<float>(m_Desc.TileCountX);
    const float  TileWorldSizeZ = PackageWorldSize / static_cast<float>(m_Desc.TileCountZ);

    m_Stats.TileCountX             = m_Desc.TileCountX;
    m_Stats.TileCountZ             = m_Desc.TileCountZ;
    m_Stats.TileCount              = m_Desc.TileCountX * m_Desc.TileCountZ;
    m_Stats.TileSampleCountPerAxis = m_Desc.TileSampleCountPerAxis;
    m_Stats.TileCellCount          = TileCellCount;
    m_Stats.TotalCellCountX        = TileCellCount * m_Desc.TileCountX;
    m_Stats.TotalCellCountZ        = TileCellCount * m_Desc.TileCountZ;
    m_Stats.TileWorldSizeX         = TileWorldSizeX;
    m_Stats.TileWorldSizeZ         = TileWorldSizeZ;
    m_Stats.Extent                 = m_Desc.Extent;

    m_Tiles.clear();
    m_Tiles.reserve(m_Stats.TileCount);
    for (Uint32 TileZ = 0; TileZ < m_Desc.TileCountZ; ++TileZ)
    {
        for (Uint32 TileX = 0; TileX < m_Desc.TileCountX; ++TileX)
        {
            TerrainHeightmapTile Tile;
            Tile.TileIndex          = static_cast<Uint32>(m_Tiles.size());
            Tile.TileX              = TileX;
            Tile.TileZ              = TileZ;
            Tile.FirstSampleX       = TileX * TileCellCount;
            Tile.FirstSampleZ       = TileZ * TileCellCount;
            Tile.SampleCountPerAxis = m_Desc.TileSampleCountPerAxis;
            Tile.CellCount          = TileCellCount;

            Tile.MinXZ = float2{
                WorldMin + TileWorldSizeX * static_cast<float>(TileX),
                WorldMin + TileWorldSizeZ * static_cast<float>(TileZ)};
            Tile.MaxXZ = float2{
                WorldMin + TileWorldSizeX * static_cast<float>(TileX + 1u),
                WorldMin + TileWorldSizeZ * static_cast<float>(TileZ + 1u)};
            Tile.CenterXZ = (Tile.MinXZ + Tile.MaxXZ) * 0.5f;
            Tile.WorldSizeX = Tile.MaxXZ.x - Tile.MinXZ.x;
            Tile.WorldSizeZ = Tile.MaxXZ.y - Tile.MinXZ.y;

            m_Tiles.push_back(Tile);
        }
    }
}

const TerrainHeightmapTile* TerrainHeightmapTileSet::FindTile(Uint32 TileX, Uint32 TileZ) const
{
    if (TileX >= m_Desc.TileCountX || TileZ >= m_Desc.TileCountZ)
        return nullptr;

    const Uint32 TileIndex = TileZ * m_Desc.TileCountX + TileX;
    return TileIndex < m_Tiles.size() ? &m_Tiles[TileIndex] : nullptr;
}

const char* TerrainHeightmapTileSet::GetLayoutName() const
{
    return m_Stats.TileCount == 1u ? "single_patch" : "tiled_grid";
}

} // namespace Diligent
