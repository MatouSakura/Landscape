#pragma once

#include "BasicMath.hpp"
#include "BasicTypes.h"
#include "TerrainQuadtree.hpp"

#include <vector>

namespace Diligent
{

enum class TerrainLODSeamSide
{
    West,
    East,
    South,
    North
};

struct TerrainLODSeamEdge final
{
    Uint32             FineNodeIndex   = InvalidNodeIndex;
    Uint32             CoarseNodeIndex = InvalidNodeIndex;
    Uint32             FineLevel       = 0;
    Uint32             CoarseLevel     = 0;
    TerrainLODSeamSide FineSide        = TerrainLODSeamSide::West;
    float2             StartXZ         = {};
    float2             EndXZ           = {};
    Uint32             LODDelta        = 0;
    Uint32             StitchRatio     = 1;
    float              Length          = 0.0f;
};

struct TerrainLODStitchingStats final
{
    Uint32 SeamEdgeCount  = 0;
    Uint32 MaxLODDelta    = 0;
    Uint32 MaxStitchRatio = 1;
    float  TotalSeamLength = 0.0f;
};

class TerrainLODStitching final
{
public:
    void Build(const std::vector<TerrainQuadtreeNode>& Nodes, const TerrainQuadtreeSelection& Selection);

    const std::vector<TerrainLODSeamEdge>& GetSeams() const { return m_Seams; }
    const TerrainLODStitchingStats& GetStats() const { return m_Stats; }

private:
    std::vector<TerrainLODSeamEdge> m_Seams;
    TerrainLODStitchingStats        m_Stats;
};

} // namespace Diligent
