#pragma once

#include "BasicTypes.h"

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
    TerrainPatch
};

struct RenderItem final
{
    RenderQueueType Queue = RenderQueueType::Debug;
    RenderItemKind  Kind  = RenderItemKind::DebugGrid;
};

class RenderQueue final
{
public:
    void Clear()
    {
        m_OpaqueItems.clear();
        m_DebugItems.clear();
    }

    void AddTerrainPatch()
    {
        m_OpaqueItems.push_back(RenderItem{RenderQueueType::Opaque, RenderItemKind::TerrainPatch});
    }

    void AddDebugGrid()
    {
        m_DebugItems.push_back(RenderItem{RenderQueueType::Debug, RenderItemKind::DebugGrid});
    }

    const std::vector<RenderItem>& GetOpaqueItems() const
    {
        return m_OpaqueItems;
    }

    const std::vector<RenderItem>& GetDebugItems() const
    {
        return m_DebugItems;
    }

    Uint32 GetQueueCount(RenderQueueType Queue) const
    {
        if (Queue == RenderQueueType::Opaque)
            return static_cast<Uint32>(m_OpaqueItems.size());
        if (Queue == RenderQueueType::Debug)
            return static_cast<Uint32>(m_DebugItems.size());
        return 0u;
    }

private:
    std::vector<RenderItem> m_OpaqueItems;
    std::vector<RenderItem> m_DebugItems;
};

} // namespace Diligent
