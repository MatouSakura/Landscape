#pragma once

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
    TransparentQuad
};

struct RenderItem final
{
    RenderQueueType Queue = RenderQueueType::Debug;
    RenderItemKind  Kind  = RenderItemKind::DebugGrid;
    float           SortDepth = 0.0f;
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
