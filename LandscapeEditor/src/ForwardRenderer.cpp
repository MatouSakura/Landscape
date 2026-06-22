#include "ForwardRenderer.hpp"

#include "DeviceContext.h"
#include "FrameResources.hpp"
#include "RenderView.hpp"

namespace Diligent
{

void ForwardRenderer::Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    m_TerrainPatchRenderer.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_ForwardDebugPipeline.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_Stats.PSOCount         = m_PSOCache.GetPSOCount();
    m_Stats.PSOCreationCount = m_PSOCache.GetCreationCount();
    m_Stats.TerrainTriangleCount = m_TerrainPatchRenderer.GetTriangleCount();
}

void ForwardRenderer::Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources)
{
    m_RenderQueue.Clear();
    m_RenderQueue.AddTerrainPatch();
    m_RenderQueue.AddDebugGrid();

    for (const RenderItem& Item : m_RenderQueue.GetOpaqueItems())
    {
        if (Item.Kind == RenderItemKind::TerrainPatch)
            m_TerrainPatchRenderer.Render(pContext, View, FrameResources);
    }

    for (const RenderItem& Item : m_RenderQueue.GetDebugItems())
    {
        if (Item.Kind == RenderItemKind::DebugGrid)
            m_ForwardDebugPipeline.Render(pContext, View, FrameResources);
    }

    m_Stats.OpaqueItemCount  = m_RenderQueue.GetQueueCount(RenderQueueType::Opaque);
    m_Stats.TerrainTriangleCount = m_TerrainPatchRenderer.GetTriangleCount();
    m_Stats.DebugItemCount   = m_RenderQueue.GetQueueCount(RenderQueueType::Debug);
    m_Stats.PSOCount         = m_PSOCache.GetPSOCount();
    m_Stats.PSOCreationCount = m_PSOCache.GetCreationCount();
}

} // namespace Diligent
