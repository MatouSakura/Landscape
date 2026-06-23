#pragma once

#include "BasicTypes.h"
#include "TerrainLODStitching.hpp"
#include "TerrainQuadtree.hpp"

#include <vector>

namespace Diligent
{

enum class TerrainLODStitchEdgeMask : Uint32
{
    None  = 0,
    West  = 1u << 0u,
    East  = 1u << 1u,
    South = 1u << 2u,
    North = 1u << 3u
};

TerrainLODStitchEdgeMask operator|(TerrainLODStitchEdgeMask Lhs, TerrainLODStitchEdgeMask Rhs);
TerrainLODStitchEdgeMask& operator|=(TerrainLODStitchEdgeMask& Lhs, TerrainLODStitchEdgeMask Rhs);
bool HasTerrainLODStitchEdge(TerrainLODStitchEdgeMask Mask, TerrainLODStitchEdgeMask Edge);

struct TerrainLODStitchedDrawRegion final
{
    Uint32                    NodeIndex          = InvalidNodeIndex;
    TerrainLODStitchEdgeMask  EdgeMask           = TerrainLODStitchEdgeMask::None;
    Uint32                    WestStitchRatio    = 1;
    Uint32                    EastStitchRatio    = 1;
    Uint32                    SouthStitchRatio   = 1;
    Uint32                    NorthStitchRatio   = 1;
    Uint32                    FirstIndexLocation = 0;
    Uint32                    NumIndices         = 0;
    Uint32                    MainNumIndices     = 0;
};

struct TerrainLODIndexStitchingStats final
{
    Uint32 StitchedNodeCount  = 0;
    Uint32 StitchedEdgeCount  = 0;
    Uint32 GeneratedIndexCount = 0;
    Uint32 MaxStitchRatio     = 1;
};

class TerrainLODIndexStitching final
{
public:
    void Build(const std::vector<TerrainQuadtreeNode>& Nodes,
               const TerrainQuadtreeSelection& Selection,
               const TerrainLODStitching& Stitching);

    const TerrainLODStitchedDrawRegion* FindRegion(Uint32 NodeIndex) const;
    TerrainLODStitchedDrawRegion* FindRegion(Uint32 NodeIndex);
    const std::vector<TerrainLODStitchedDrawRegion>& GetRegions() const { return m_Regions; }
    std::vector<TerrainLODStitchedDrawRegion>& GetRegions() { return m_Regions; }
    const TerrainLODIndexStitchingStats& GetStats() const { return m_Stats; }

    void SetGeneratedIndexStats(Uint32 GeneratedIndexCount);

private:
    TerrainLODStitchedDrawRegion& FindOrAddRegion(Uint32 NodeIndex);

private:
    std::vector<TerrainLODStitchedDrawRegion> m_Regions;
    std::vector<Uint32>                       m_NodeToRegionIndex;
    TerrainLODIndexStitchingStats             m_Stats;
};

} // namespace Diligent
