#include "TerrainLODStitching.hpp"

#include <algorithm>
#include <cmath>

namespace Diligent
{

namespace
{

static constexpr float EdgeEpsilon = 0.0001f;

bool NearlyEqual(float Lhs, float Rhs)
{
    return std::abs(Lhs - Rhs) <= EdgeEpsilon;
}

bool RangesOverlap(float LhsMin, float LhsMax, float RhsMin, float RhsMax, float& OverlapMin, float& OverlapMax)
{
    OverlapMin = std::max(LhsMin, RhsMin);
    OverlapMax = std::min(LhsMax, RhsMax);
    return OverlapMax - OverlapMin > EdgeEpsilon;
}

float Distance(const float2& Start, const float2& End)
{
    const float2 Delta = End - Start;
    return std::sqrt(Delta.x * Delta.x + Delta.y * Delta.y);
}

bool TryBuildSeamEdge(const TerrainQuadtreeNode& Lhs,
                      const TerrainQuadtreeNode& Rhs,
                      TerrainLODSeamEdge&        OutSeam)
{
    if (Lhs.Level == Rhs.Level)
        return false;

    float2 StartXZ;
    float2 EndXZ;
    TerrainLODSeamSide LhsSide = TerrainLODSeamSide::West;
    TerrainLODSeamSide RhsSide = TerrainLODSeamSide::West;
    float OverlapMin = 0.0f;
    float OverlapMax = 0.0f;

    if (NearlyEqual(Lhs.MaxXZ.x, Rhs.MinXZ.x) && RangesOverlap(Lhs.MinXZ.y, Lhs.MaxXZ.y, Rhs.MinXZ.y, Rhs.MaxXZ.y, OverlapMin, OverlapMax))
    {
        StartXZ = float2{Lhs.MaxXZ.x, OverlapMin};
        EndXZ   = float2{Lhs.MaxXZ.x, OverlapMax};
        LhsSide = TerrainLODSeamSide::East;
        RhsSide = TerrainLODSeamSide::West;
    }
    else if (NearlyEqual(Lhs.MinXZ.x, Rhs.MaxXZ.x) && RangesOverlap(Lhs.MinXZ.y, Lhs.MaxXZ.y, Rhs.MinXZ.y, Rhs.MaxXZ.y, OverlapMin, OverlapMax))
    {
        StartXZ = float2{Lhs.MinXZ.x, OverlapMin};
        EndXZ   = float2{Lhs.MinXZ.x, OverlapMax};
        LhsSide = TerrainLODSeamSide::West;
        RhsSide = TerrainLODSeamSide::East;
    }
    else if (NearlyEqual(Lhs.MaxXZ.y, Rhs.MinXZ.y) && RangesOverlap(Lhs.MinXZ.x, Lhs.MaxXZ.x, Rhs.MinXZ.x, Rhs.MaxXZ.x, OverlapMin, OverlapMax))
    {
        StartXZ = float2{OverlapMin, Lhs.MaxXZ.y};
        EndXZ   = float2{OverlapMax, Lhs.MaxXZ.y};
        LhsSide = TerrainLODSeamSide::North;
        RhsSide = TerrainLODSeamSide::South;
    }
    else if (NearlyEqual(Lhs.MinXZ.y, Rhs.MaxXZ.y) && RangesOverlap(Lhs.MinXZ.x, Lhs.MaxXZ.x, Rhs.MinXZ.x, Rhs.MaxXZ.x, OverlapMin, OverlapMax))
    {
        StartXZ = float2{OverlapMin, Lhs.MinXZ.y};
        EndXZ   = float2{OverlapMax, Lhs.MinXZ.y};
        LhsSide = TerrainLODSeamSide::South;
        RhsSide = TerrainLODSeamSide::North;
    }
    else
    {
        return false;
    }

    const TerrainQuadtreeNode& FineNode   = Lhs.Level > Rhs.Level ? Lhs : Rhs;
    const TerrainQuadtreeNode& CoarseNode = Lhs.Level > Rhs.Level ? Rhs : Lhs;
    const TerrainLODSeamSide   FineSide   = Lhs.Level > Rhs.Level ? LhsSide : RhsSide;

    OutSeam.FineNodeIndex   = FineNode.NodeIndex;
    OutSeam.CoarseNodeIndex = CoarseNode.NodeIndex;
    OutSeam.FineLevel       = FineNode.Level;
    OutSeam.CoarseLevel     = CoarseNode.Level;
    OutSeam.FineSide        = FineSide;
    OutSeam.StartXZ         = StartXZ;
    OutSeam.EndXZ           = EndXZ;
    OutSeam.LODDelta        = FineNode.Level - CoarseNode.Level;
    OutSeam.StitchRatio     = 1u << OutSeam.LODDelta;
    OutSeam.Length          = Distance(StartXZ, EndXZ);
    return true;
}

} // namespace

void TerrainLODStitching::Build(const std::vector<TerrainQuadtreeNode>& Nodes, const TerrainQuadtreeSelection& Selection)
{
    m_Seams.clear();
    m_Stats = {};
    m_Stats.MaxStitchRatio = 1;

    const std::vector<Uint32>& SelectedNodeIndices = Selection.SelectedNodeIndices;
    for (size_t LhsIndex = 0; LhsIndex < SelectedNodeIndices.size(); ++LhsIndex)
    {
        const Uint32 LhsNodeIndex = SelectedNodeIndices[LhsIndex];
        if (LhsNodeIndex >= Nodes.size())
            continue;

        for (size_t RhsIndex = LhsIndex + 1u; RhsIndex < SelectedNodeIndices.size(); ++RhsIndex)
        {
            const Uint32 RhsNodeIndex = SelectedNodeIndices[RhsIndex];
            if (RhsNodeIndex >= Nodes.size())
                continue;

            TerrainLODSeamEdge Seam;
            if (!TryBuildSeamEdge(Nodes[LhsNodeIndex], Nodes[RhsNodeIndex], Seam))
                continue;

            m_Seams.push_back(Seam);
            m_Stats.MaxLODDelta = std::max(m_Stats.MaxLODDelta, Seam.LODDelta);
            m_Stats.MaxStitchRatio = std::max(m_Stats.MaxStitchRatio, Seam.StitchRatio);
            m_Stats.TotalSeamLength += Seam.Length;
        }
    }

    m_Stats.SeamEdgeCount = static_cast<Uint32>(m_Seams.size());
}

} // namespace Diligent
