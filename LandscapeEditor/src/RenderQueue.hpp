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
    DebugGrid
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
        m_DebugItems.clear();
    }

    void AddDebugGrid()
    {
        m_DebugItems.push_back(RenderItem{RenderQueueType::Debug, RenderItemKind::DebugGrid});
    }

    const std::vector<RenderItem>& GetDebugItems() const
    {
        return m_DebugItems;
    }

    Uint32 GetQueueCount(RenderQueueType Queue) const
    {
        return Queue == RenderQueueType::Debug ? static_cast<Uint32>(m_DebugItems.size()) : 0u;
    }

private:
    std::vector<RenderItem> m_DebugItems;
};

} // namespace Diligent
