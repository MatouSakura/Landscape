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
    m_PostProcessRenderer.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_SkyRenderer.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_TerrainPatchRenderer.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_TransparentRenderer.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_ForwardDebugPipeline.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_Stats.PSOCount         = m_PSOCache.GetPSOCount();
    m_Stats.PSOCreationCount = m_PSOCache.GetCreationCount();
    m_Stats.TerrainTriangleCount = m_TerrainPatchRenderer.GetTriangleCount();
    m_Stats.ShadowCascadeCount = ShadowRenderer::CascadeCount;
    m_Stats.ShadowMapSize      = m_ShadowRenderer.GetShadowMapSize();
    m_Stats.SkyPassCount       = m_SkyRenderer.GetPassCount();
    m_Stats.TransparentPassCount = m_TransparentRenderer.GetPassCount();
    m_Stats.PostProcessPassCount = m_PostProcessRenderer.GetPassCount();
}

void ForwardRenderer::Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources)
{
    m_RenderQueue.Clear();
    m_RenderQueue.AddTerrainPatch();
    m_RenderQueue.AddTransparentQuad(length(View.CameraPosition - TransparentRenderer::GetTestQuadCenter()));
    m_RenderQueue.SortTransparentBackToFront();
    m_RenderQueue.AddDebugGrid();

    LightConstants LightData{};
    m_ShadowRenderer.FillLightConstants(LightData);
    FrameResources.UpdateLightConstants(pContext, LightData);

    m_ShadowRenderer.Render(pContext, m_TerrainPatchRenderer);

    const float SceneClearColor[] = {0.08f, 0.09f, 0.10f, 1.0f};
    ITextureView* pRTV = m_PostProcessRenderer.PrepareSceneTarget(pContext, m_pSwapChain, SceneClearColor);
    ITextureView* pDSV = m_pSwapChain->GetDepthBufferDSV();
    pContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    for (const RenderItem& Item : m_RenderQueue.GetOpaqueItems())
    {
        if (Item.Kind == RenderItemKind::TerrainPatch)
            m_TerrainPatchRenderer.Render(pContext, View, FrameResources, m_ShadowRenderer.GetCascadeSRV(0));
    }

    m_SkyRenderer.Render(pContext, View);

    for (const RenderItem& Item : m_RenderQueue.GetTransparentItems())
    {
        if (Item.Kind == RenderItemKind::TransparentQuad)
            m_TransparentRenderer.Render(pContext, View, FrameResources);
    }

    for (const RenderItem& Item : m_RenderQueue.GetDebugItems())
    {
        if (Item.Kind == RenderItemKind::DebugGrid)
            m_ForwardDebugPipeline.Render(pContext, View, FrameResources);
    }

    m_PostProcessRenderer.Render(pContext, m_pSwapChain);

    m_Stats.OpaqueItemCount  = m_RenderQueue.GetQueueCount(RenderQueueType::Opaque);
    m_Stats.TerrainTriangleCount = m_TerrainPatchRenderer.GetTriangleCount();
    m_Stats.ShadowCascadeCount = ShadowRenderer::CascadeCount;
    m_Stats.ShadowMapSize      = m_ShadowRenderer.GetShadowMapSize();
    m_Stats.SkyPassCount       = m_SkyRenderer.GetPassCount();
    m_Stats.TransparentItemCount = m_RenderQueue.GetQueueCount(RenderQueueType::Transparent);
    m_Stats.TransparentPassCount = m_TransparentRenderer.GetPassCount();
    m_Stats.DebugItemCount   = m_RenderQueue.GetQueueCount(RenderQueueType::Debug);
    m_Stats.PostProcessPassCount = m_PostProcessRenderer.GetPassCount();
    m_Stats.PSOCount         = m_PSOCache.GetPSOCount();
    m_Stats.PSOCreationCount = m_PSOCache.GetCreationCount();
}

} // namespace Diligent
