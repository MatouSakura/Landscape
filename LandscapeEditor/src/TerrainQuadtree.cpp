#include "TerrainQuadtree.hpp"

#include <algorithm>
#include <cmath>

namespace Diligent
{

namespace
{

TerrainComponentLODPolicy ClampLODPolicy(TerrainComponentLODPolicy Policy, Uint32 MaxDepth)
{
    Policy.SplitDistanceScale = std::clamp(Policy.SplitDistanceScale, 0.25f, 8.0f);
    Policy.MaxSelectedLevel   = std::clamp<Uint32>(Policy.MaxSelectedLevel, 1u, MaxDepth);
    return Policy;
}

float ComputeRadius(const float2& MinXZ, const float2& MaxXZ)
{
    const float2 Extents = (MaxXZ - MinXZ) * 0.5f;
    return std::sqrt(Extents.x * Extents.x + Extents.y * Extents.y);
}

} // namespace

void TerrainQuadtree::Build(const TerrainQuadtreeDesc& Desc)
{
    m_Desc          = Desc;
    m_Desc.MaxDepth = std::max<Uint32>(m_Desc.MaxDepth, 1u);
    m_Desc.Extent   = std::max(m_Desc.Extent, 1.0f);
    m_LODPolicy     = ClampLODPolicy(m_LODPolicy, m_Desc.MaxDepth);

    m_Nodes.clear();
    const float2 RootMin{-m_Desc.Extent, -m_Desc.Extent};
    const float2 RootMax{m_Desc.Extent, m_Desc.Extent};
    AddNode(InvalidNodeIndex, 0, RootMin, RootMax);
    SubdivideNode(0);
}

void TerrainQuadtree::SetLODPolicy(const TerrainComponentLODPolicy& Policy)
{
    m_LODPolicy = ClampLODPolicy(Policy, m_Desc.MaxDepth);
}

void TerrainQuadtree::Select(const float3& CameraPosition, TerrainQuadtreeSelection& OutSelection) const
{
    OutSelection.SelectedNodeIndices.clear();
    OutSelection.MinSelectedLevel = 0;
    OutSelection.MaxSelectedLevel = 0;

    if (!m_Nodes.empty())
        SelectNode(0, CameraPosition, OutSelection);
}

Uint32 TerrainQuadtree::GetSelectedLeafCapacity() const
{
    Uint32 Capacity = 1;
    for (Uint32 Level = 0; Level < m_Desc.MaxDepth; ++Level)
        Capacity *= 4u;
    return Capacity;
}

Uint32 TerrainQuadtree::GetMaxSelectableLevel() const
{
    return std::min(m_Desc.MaxDepth, m_LODPolicy.MaxSelectedLevel);
}

float TerrainQuadtree::ComputeSplitDistance(Uint32 Level) const
{
    const Uint32 SafeLevel = std::min(Level, m_Desc.MaxDepth);
    return m_Desc.Extent * m_LODPolicy.SplitDistanceScale / static_cast<float>(1u << SafeLevel);
}

float TerrainQuadtree::GetComponentWorldSize(Uint32 Level) const
{
    const Uint32 SafeLevel = std::min(Level, m_Desc.MaxDepth);
    return (2.0f * m_Desc.Extent) / static_cast<float>(1u << SafeLevel);
}

Uint32 TerrainQuadtree::AddNode(Uint32 ParentIndex, Uint32 Level, const float2& MinXZ, const float2& MaxXZ)
{
    const Uint32 NodeIndex = static_cast<Uint32>(m_Nodes.size());

    TerrainQuadtreeNode Node;
    Node.NodeIndex   = NodeIndex;
    Node.ParentIndex = ParentIndex;
    Node.Level       = Level;
    Node.MinXZ       = MinXZ;
    Node.MaxXZ       = MaxXZ;
    Node.CenterXZ    = (MinXZ + MaxXZ) * 0.5f;
    Node.Radius      = ComputeRadius(MinXZ, MaxXZ);
    m_Nodes.push_back(Node);

    return NodeIndex;
}

void TerrainQuadtree::SubdivideNode(Uint32 NodeIndex)
{
    TerrainQuadtreeNode& StoredNode = m_Nodes[NodeIndex];
    if (StoredNode.Level >= m_Desc.MaxDepth)
        return;

    StoredNode.FirstChildIndex = static_cast<Uint32>(m_Nodes.size());
    StoredNode.ChildMask       = 0xFu;

    const Uint32 FirstChildIndex = StoredNode.FirstChildIndex;
    const Uint32 ChildLevel      = StoredNode.Level + 1u;
    const float2 MinXZ          = StoredNode.MinXZ;
    const float2 MaxXZ          = StoredNode.MaxXZ;
    const float2 Center = StoredNode.CenterXZ;

    // Stable child order: SouthWest, SouthEast, NorthWest, NorthEast.
    const float2 SouthWestMin{MinXZ.x, MinXZ.y};
    const float2 SouthWestMax{Center.x, Center.y};
    const float2 SouthEastMin{Center.x, MinXZ.y};
    const float2 SouthEastMax{MaxXZ.x, Center.y};
    const float2 NorthWestMin{MinXZ.x, Center.y};
    const float2 NorthWestMax{Center.x, MaxXZ.y};
    const float2 NorthEastMin{Center.x, Center.y};
    const float2 NorthEastMax{MaxXZ.x, MaxXZ.y};

    AddNode(NodeIndex, ChildLevel, SouthWestMin, SouthWestMax);
    AddNode(NodeIndex, ChildLevel, SouthEastMin, SouthEastMax);
    AddNode(NodeIndex, ChildLevel, NorthWestMin, NorthWestMax);
    AddNode(NodeIndex, ChildLevel, NorthEastMin, NorthEastMax);

    for (Uint32 Child = 0; Child < 4u; ++Child)
        SubdivideNode(FirstChildIndex + Child);
}

void TerrainQuadtree::SelectNode(Uint32 NodeIndex, const float3& CameraPosition, TerrainQuadtreeSelection& OutSelection) const
{
    const TerrainQuadtreeNode& Node = m_Nodes[NodeIndex];

    const float DeltaX = CameraPosition.x - Node.CenterXZ.x;
    const float DeltaZ = CameraPosition.z - Node.CenterXZ.y;
    const float DistanceXZ = std::sqrt(DeltaX * DeltaX + DeltaZ * DeltaZ);
    const float SplitDistance = ComputeSplitDistance(Node.Level);

    if (Node.Level < GetMaxSelectableLevel() && Node.FirstChildIndex != InvalidNodeIndex && DistanceXZ < SplitDistance)
    {
        for (Uint32 Child = 0; Child < 4u; ++Child)
            SelectNode(Node.FirstChildIndex + Child, CameraPosition, OutSelection);
        return;
    }

    OutSelection.SelectedNodeIndices.push_back(NodeIndex);
    OutSelection.MinSelectedLevel = OutSelection.SelectedNodeIndices.size() == 1u ? Node.Level : std::min(OutSelection.MinSelectedLevel, Node.Level);
    OutSelection.MaxSelectedLevel = std::max(OutSelection.MaxSelectedLevel, Node.Level);
}

} // namespace Diligent
