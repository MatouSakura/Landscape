#include "TerrainLODIndexStitching.hpp"

#include <algorithm>

namespace Diligent
{

namespace
{

TerrainLODStitchEdgeMask ToEdgeMask(TerrainLODSeamSide Side)
{
    switch (Side)
    {
        case TerrainLODSeamSide::West:  return TerrainLODStitchEdgeMask::West;
        case TerrainLODSeamSide::East:  return TerrainLODStitchEdgeMask::East;
        case TerrainLODSeamSide::South: return TerrainLODStitchEdgeMask::South;
        case TerrainLODSeamSide::North: return TerrainLODStitchEdgeMask::North;
    }
    return TerrainLODStitchEdgeMask::None;
}

void UpdateSideRatio(TerrainLODStitchedDrawRegion& Region, TerrainLODSeamSide Side, Uint32 StitchRatio)
{
    const Uint32 Ratio = std::max(StitchRatio, 1u);
    switch (Side)
    {
        case TerrainLODSeamSide::West:
            Region.WestStitchRatio = std::max(Region.WestStitchRatio, Ratio);
            break;
        case TerrainLODSeamSide::East:
            Region.EastStitchRatio = std::max(Region.EastStitchRatio, Ratio);
            break;
        case TerrainLODSeamSide::South:
            Region.SouthStitchRatio = std::max(Region.SouthStitchRatio, Ratio);
            break;
        case TerrainLODSeamSide::North:
            Region.NorthStitchRatio = std::max(Region.NorthStitchRatio, Ratio);
            break;
    }
}

Uint32 CountStitchedCorners(TerrainLODStitchEdgeMask EdgeMask)
{
    const bool StitchWest  = HasTerrainLODStitchEdge(EdgeMask, TerrainLODStitchEdgeMask::West);
    const bool StitchEast  = HasTerrainLODStitchEdge(EdgeMask, TerrainLODStitchEdgeMask::East);
    const bool StitchSouth = HasTerrainLODStitchEdge(EdgeMask, TerrainLODStitchEdgeMask::South);
    const bool StitchNorth = HasTerrainLODStitchEdge(EdgeMask, TerrainLODStitchEdgeMask::North);

    Uint32 CornerCount = 0;
    CornerCount += StitchWest && StitchSouth ? 1u : 0u;
    CornerCount += StitchWest && StitchNorth ? 1u : 0u;
    CornerCount += StitchEast && StitchSouth ? 1u : 0u;
    CornerCount += StitchEast && StitchNorth ? 1u : 0u;
    return CornerCount;
}

} // namespace

TerrainLODStitchEdgeMask operator|(TerrainLODStitchEdgeMask Lhs, TerrainLODStitchEdgeMask Rhs)
{
    return static_cast<TerrainLODStitchEdgeMask>(static_cast<Uint32>(Lhs) | static_cast<Uint32>(Rhs));
}

TerrainLODStitchEdgeMask& operator|=(TerrainLODStitchEdgeMask& Lhs, TerrainLODStitchEdgeMask Rhs)
{
    Lhs = Lhs | Rhs;
    return Lhs;
}

bool HasTerrainLODStitchEdge(TerrainLODStitchEdgeMask Mask, TerrainLODStitchEdgeMask Edge)
{
    return (static_cast<Uint32>(Mask) & static_cast<Uint32>(Edge)) != 0u;
}

void TerrainLODIndexStitching::Build(const std::vector<TerrainQuadtreeNode>& Nodes,
                                     const TerrainQuadtreeSelection& Selection,
                                     const TerrainLODStitching& Stitching)
{
    m_Regions.clear();
    m_NodeToRegionIndex.clear();
    m_NodeToRegionIndex.resize(Nodes.size(), InvalidNodeIndex);
    m_Stats = {};
    m_Stats.MaxStitchRatio = 1;
    m_Stats.GeneratedIndexCount = 0;

    std::vector<bool> SelectedNodes(Nodes.size(), false);
    for (Uint32 NodeIndex : Selection.SelectedNodeIndices)
    {
        if (NodeIndex < SelectedNodes.size())
            SelectedNodes[NodeIndex] = true;
    }

    for (const TerrainLODSeamEdge& Seam : Stitching.GetSeams())
    {
        const Uint32 FineNodeIndex = Seam.FineNodeIndex;
        if (FineNodeIndex >= Nodes.size() || !SelectedNodes[FineNodeIndex])
            continue;

        TerrainLODStitchedDrawRegion& Region = FindOrAddRegion(FineNodeIndex);
        Region.EdgeMask |= ToEdgeMask(Seam.FineSide);
        const TerrainLODSeamSide FineSide = Seam.FineSide;
        UpdateSideRatio(Region, FineSide, Seam.StitchRatio);
        ++m_Stats.StitchedEdgeCount;
        m_Stats.MaxStitchRatio = std::max(m_Stats.MaxStitchRatio, Seam.StitchRatio);
    }

    m_Stats.StitchedNodeCount = static_cast<Uint32>(m_Regions.size());
    for (const TerrainLODStitchedDrawRegion& Region : m_Regions)
        m_Stats.StitchedCornerCount += CountStitchedCorners(Region.EdgeMask);
}

const TerrainLODStitchedDrawRegion* TerrainLODIndexStitching::FindRegion(Uint32 NodeIndex) const
{
    if (NodeIndex >= m_NodeToRegionIndex.size())
        return nullptr;

    const Uint32 RegionIndex = m_NodeToRegionIndex[NodeIndex];
    return RegionIndex < m_Regions.size() ? &m_Regions[RegionIndex] : nullptr;
}

TerrainLODStitchedDrawRegion* TerrainLODIndexStitching::FindRegion(Uint32 NodeIndex)
{
    if (NodeIndex >= m_NodeToRegionIndex.size())
        return nullptr;

    const Uint32 RegionIndex = m_NodeToRegionIndex[NodeIndex];
    return RegionIndex < m_Regions.size() ? &m_Regions[RegionIndex] : nullptr;
}

void TerrainLODIndexStitching::SetGeneratedIndexStats(Uint32 GeneratedIndexCount, Uint32 CornerPatchIndexCount)
{
    m_Stats.GeneratedIndexCount = GeneratedIndexCount;
    m_Stats.CornerPatchIndexCount = CornerPatchIndexCount;
}

TerrainLODStitchedDrawRegion& TerrainLODIndexStitching::FindOrAddRegion(Uint32 NodeIndex)
{
    if (NodeIndex < m_NodeToRegionIndex.size())
    {
        const Uint32 RegionIndex = m_NodeToRegionIndex[NodeIndex];
        if (RegionIndex < m_Regions.size())
            return m_Regions[RegionIndex];
    }

    TerrainLODStitchedDrawRegion Region;
    Region.NodeIndex = NodeIndex;
    m_Regions.push_back(Region);

    if (NodeIndex < m_NodeToRegionIndex.size())
        m_NodeToRegionIndex[NodeIndex] = static_cast<Uint32>(m_Regions.size() - 1u);

    return m_Regions.back();
}

} // namespace Diligent
