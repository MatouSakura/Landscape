#pragma once

#include "BasicMath.hpp"
#include "BasicTypes.h"

#include <algorithm>
#include <vector>

namespace Diligent
{

enum class RenderQueueType
{
    Opaque,
    Masked,
    Transparent,
    Debug
};

enum class RenderItemKind
{
    DebugGrid,
    TerrainPatch,
    TerrainLeaf,
    TransparentQuad
};

struct TerrainDrawRegion final
{
    Uint32 NodeIndex  = 0;
    Uint32 Level      = 0;
    float2 MinXZ      = {};
    float2 MaxXZ      = {};
    Uint32 FirstCellX = 0;
    Uint32 FirstCellZ = 0;
    Uint32 CellCountX = 0;
    Uint32 CellCountZ = 0;
    Uint32 LODSampleStep = 1;
    Uint32 MeshCellCountX = 0;
    Uint32 MeshCellCountZ = 0;
    Uint32 MeshSampleCountX = 0;
    Uint32 MeshSampleCountZ = 0;
    Uint32 BaseVertex = 0;
    Uint32 FirstIndexLocation = 0;
    Uint32 MainNumIndices = 0;
    Uint32 SkirtIndexCount = 0;
    Uint32 NumIndices = 0;
};

struct RenderItem final
{
    RenderQueueType  Queue = RenderQueueType::Debug;
    RenderItemKind   Kind  = RenderItemKind::DebugGrid;
    float            SortDepth = 0.0f;
    TerrainDrawRegion TerrainRegion = {};
};

class RenderQueue final
{
public:
    void Clear()
    {
        m_OpaqueItems.clear();
        m_TransparentItems.clear();
        m_DebugItems.clear();
    }

    void AddTerrainPatch()
    {
        m_OpaqueItems.push_back(RenderItem{RenderQueueType::Opaque, RenderItemKind::TerrainPatch});
    }

    void AddTerrainLeaf(const TerrainDrawRegion& Region)
    {
        RenderItem Item;
        Item.Queue         = RenderQueueType::Opaque;
        Item.Kind          = RenderItemKind::TerrainLeaf;
        Item.TerrainRegion = Region;
        m_OpaqueItems.push_back(Item);
    }

    void AddTransparentQuad(float SortDepth)
    {
        m_TransparentItems.push_back(RenderItem{RenderQueueType::Transparent, RenderItemKind::TransparentQuad, SortDepth});
    }

    void AddDebugGrid()
    {
        m_DebugItems.push_back(RenderItem{RenderQueueType::Debug, RenderItemKind::DebugGrid});
    }

    void SortTransparentBackToFront()
    {
        std::sort(m_TransparentItems.begin(), m_TransparentItems.end(), [](const RenderItem& Lhs, const RenderItem& Rhs) {
            return Lhs.SortDepth > Rhs.SortDepth;
        });
    }

    const std::vector<RenderItem>& GetOpaqueItems() const
    {
        return m_OpaqueItems;
    }

    const std::vector<RenderItem>& GetTransparentItems() const
    {
        return m_TransparentItems;
    }

    const std::vector<RenderItem>& GetDebugItems() const
    {
        return m_DebugItems;
    }

    Uint32 GetQueueCount(RenderQueueType Queue) const
    {
        if (Queue == RenderQueueType::Opaque)
            return static_cast<Uint32>(m_OpaqueItems.size());
        if (Queue == RenderQueueType::Transparent)
            return static_cast<Uint32>(m_TransparentItems.size());
        if (Queue == RenderQueueType::Debug)
            return static_cast<Uint32>(m_DebugItems.size());
        return 0u;
    }

private:
    std::vector<RenderItem> m_OpaqueItems;
    std::vector<RenderItem> m_TransparentItems;
    std::vector<RenderItem> m_DebugItems;
};

} // namespace Diligent
