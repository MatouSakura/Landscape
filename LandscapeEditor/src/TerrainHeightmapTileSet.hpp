#pragma once

#include "BasicMath.hpp"
#include "BasicTypes.h"

#include <vector>

namespace Diligent
{

struct TerrainHeightmapTileSetDesc final
{
    Uint32 TileCountX             = 1;
    Uint32 TileCountZ             = 1;
    Uint32 TileSampleCountPerAxis = 65;
    float  Extent                 = 20.0f;
};

struct TerrainHeightmapTile final
{
    Uint32 TileIndex              = 0;
    Uint32 TileX                  = 0;
    Uint32 TileZ                  = 0;
    Uint32 FirstSampleX           = 0;
    Uint32 FirstSampleZ           = 0;
    Uint32 SampleCountPerAxis     = 0;
    Uint32 CellCount              = 0;
    float2 MinXZ                  = {};
    float2 MaxXZ                  = {};
    float2 CenterXZ               = {};
    float  WorldSizeX             = 0.0f;
    float  WorldSizeZ             = 0.0f;
};

struct TerrainHeightmapTileSetStats final
{
    Uint32 TileCountX             = 1;
    Uint32 TileCountZ             = 1;
    Uint32 TileCount              = 1;
    Uint32 TileSampleCountPerAxis = 65;
    Uint32 TileCellCount          = 64;
    Uint32 TotalCellCountX        = 64;
    Uint32 TotalCellCountZ        = 64;
    float  TileWorldSizeX         = 40.0f;
    float  TileWorldSizeZ         = 40.0f;
    float  Extent                 = 20.0f;
};

class TerrainHeightmapTileSet final
{
public:
    void Build(const TerrainHeightmapTileSetDesc& Desc);

    const TerrainHeightmapTileSetDesc&  GetDesc() const { return m_Desc; }
    const std::vector<TerrainHeightmapTile>& GetTiles() const { return m_Tiles; }
    const TerrainHeightmapTile* FindTile(Uint32 TileX, Uint32 TileZ) const;
    const TerrainHeightmapTileSetStats& GetStats() const { return m_Stats; }
    const char* GetLayoutName() const;

private:
    TerrainHeightmapTileSetDesc  m_Desc;
    TerrainHeightmapTileSetStats m_Stats;
    std::vector<TerrainHeightmapTile> m_Tiles;
};

} // namespace Diligent
