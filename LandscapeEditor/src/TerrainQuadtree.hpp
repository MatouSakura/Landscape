#pragma once

#include "BasicMath.hpp"
#include "BasicTypes.h"

#include <vector>

namespace Diligent
{

static constexpr Uint32 InvalidNodeIndex = 0xFFFFFFFFu;

struct TerrainQuadtreeDesc final
{
    Uint32 MaxDepth = 4;
    float  Extent   = 20.0f;
};

struct TerrainComponentLODPolicy final
{
    float  SplitDistanceScale = 2.2f;
    Uint32 MaxSelectedLevel   = 4;
};

struct TerrainQuadtreeNode final
{
    Uint32 NodeIndex      = 0;
    Uint32 ParentIndex    = InvalidNodeIndex;
    Uint32 FirstChildIndex = InvalidNodeIndex;
    Uint32 Level          = 0;
    Uint32 ChildMask      = 0;
    float2 MinXZ          = {};
    float2 MaxXZ          = {};
    float2 CenterXZ       = {};
    float  Radius         = 0.0f;
};

struct TerrainQuadtreeSelection final
{
    std::vector<Uint32> SelectedNodeIndices;
    Uint32              MinSelectedLevel = 0;
    Uint32              MaxSelectedLevel = 0;
};

class TerrainQuadtree final
{
public:
    void Build(const TerrainQuadtreeDesc& Desc);
    void SetLODPolicy(const TerrainComponentLODPolicy& Policy);
    void Select(const float3& CameraPosition, TerrainQuadtreeSelection& OutSelection) const;

    const std::vector<TerrainQuadtreeNode>& GetNodes() const { return m_Nodes; }
    Uint32 GetMaxDepth() const { return m_Desc.MaxDepth; }
    Uint32 GetSelectedLeafCapacity() const;
    const TerrainComponentLODPolicy& GetLODPolicy() const { return m_LODPolicy; }
    Uint32 GetMaxSelectableLevel() const;
    float  ComputeSplitDistance(Uint32 Level) const;
    float  GetComponentWorldSize(Uint32 Level) const;

private:
    Uint32 AddNode(Uint32 ParentIndex, Uint32 Level, const float2& MinXZ, const float2& MaxXZ);
    void   SubdivideNode(Uint32 NodeIndex);
    void   SelectNode(Uint32 NodeIndex, const float3& CameraPosition, TerrainQuadtreeSelection& OutSelection) const;

private:
    TerrainQuadtreeDesc             m_Desc;
    TerrainComponentLODPolicy       m_LODPolicy;
    std::vector<TerrainQuadtreeNode> m_Nodes;
};

} // namespace Diligent
