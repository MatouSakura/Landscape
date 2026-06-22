#include "ForwardRenderer.hpp"

#include "DeviceContext.h"
#include "FrameResources.hpp"
#include "RenderView.hpp"
#include "SwapChain.h"

namespace Diligent
{

void ForwardRenderer::Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    m_pSwapChain = pSwapChain;
    m_ShadowRenderer.Initialize(pDevice, pDevice->GetDeviceInfo().NDC.MinZ == -1);
    m_TerrainPatchRenderer.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_ForwardDebugPipeline.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_Stats.PSOCount         = m_PSOCache.GetPSOCount();
    m_Stats.PSOCreationCount = m_PSOCache.GetCreationCount();
    m_Stats.TerrainTriangleCount = m_TerrainPatchRenderer.GetTriangleCount();
    m_Stats.ShadowCascadeCount = ShadowRenderer::CascadeCount;
    m_Stats.ShadowMapSize      = m_ShadowRenderer.GetShadowMapSize();
}

void ForwardRenderer::Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources)
{
    m_RenderQueue.Clear();
    m_RenderQueue.AddTerrainPatch();
    m_RenderQueue.AddDebugGrid();

    LightConstants LightData{};
    m_ShadowRenderer.FillLightConstants(LightData);
    FrameResources.UpdateLightConstants(pContext, LightData);

    m_ShadowRenderer.Render(pContext, m_TerrainPatchRenderer);

    ITextureView* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    ITextureView* pDSV = m_pSwapChain->GetDepthBufferDSV();
    pContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    for (const RenderItem& Item : m_RenderQueue.GetOpaqueItems())
    {
        if (Item.Kind == RenderItemKind::TerrainPatch)
            m_TerrainPatchRenderer.Render(pContext, View, FrameResources, m_ShadowRenderer.GetCascadeSRV(0));
    }

    for (const RenderItem& Item : m_RenderQueue.GetDebugItems())
    {
        if (Item.Kind == RenderItemKind::DebugGrid)
            m_ForwardDebugPipeline.Render(pContext, View, FrameResources);
    }

    m_Stats.OpaqueItemCount  = m_RenderQueue.GetQueueCount(RenderQueueType::Opaque);
    m_Stats.TerrainTriangleCount = m_TerrainPatchRenderer.GetTriangleCount();
    m_Stats.ShadowCascadeCount = ShadowRenderer::CascadeCount;
    m_Stats.ShadowMapSize      = m_ShadowRenderer.GetShadowMapSize();
    m_Stats.DebugItemCount   = m_RenderQueue.GetQueueCount(RenderQueueType::Debug);
    m_Stats.PSOCount         = m_PSOCache.GetPSOCount();
    m_Stats.PSOCreationCount = m_PSOCache.GetCreationCount();
}

} // namespace Diligent
