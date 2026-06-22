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
    TerrainQuadtreeDesc QuadtreeDesc;
    m_TerrainQuadtree.Build(QuadtreeDesc);
    m_TerrainQuadtreeDebugRenderer.Initialize(pDevice, pSwapChain, m_PSOCache, m_TerrainQuadtree.GetSelectedLeafCapacity() * 8u);
    m_TransparentRenderer.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_ForwardDebugPipeline.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_Stats.PSOCount         = m_PSOCache.GetPSOCount();
    m_Stats.PSOCreationCount = m_PSOCache.GetCreationCount();
    m_Stats.TerrainTriangleCount = m_TerrainPatchRenderer.GetTriangleCount();
    m_Stats.TerrainRenderItemCount = 0;
    m_Stats.TerrainRenderedCellCount = 0;
    m_Stats.TerrainForwardDrawCallCount = 0;
    m_Stats.TerrainShadowDrawCallCount = 0;
    m_Stats.TerrainCellCount = m_TerrainPatchRenderer.GetCellCount();
    m_Stats.TerrainSampleCountPerAxis = m_TerrainPatchRenderer.GetSampleCountPerAxis();
    m_Stats.TerrainMinHeight = m_TerrainPatchRenderer.GetMinHeight();
    m_Stats.TerrainMaxHeight = m_TerrainPatchRenderer.GetMaxHeight();
    m_Stats.TerrainAverageHeight = m_TerrainPatchRenderer.GetAverageHeight();
    m_Stats.TerrainQuadtreeNodeCount = static_cast<Uint32>(m_TerrainQuadtree.GetNodes().size());
    m_Stats.TerrainQuadtreeSelectedLeafCount = static_cast<Uint32>(m_TerrainQuadtreeSelection.SelectedNodeIndices.size());
    m_Stats.TerrainQuadtreeMaxDepth = m_TerrainQuadtree.GetMaxDepth();
    m_Stats.TerrainQuadtreeMaxSelectedLevel = m_TerrainQuadtreeSelection.MaxSelectedLevel;
    m_Stats.ShadowCascadeCount = ShadowRenderer::CascadeCount;
    m_Stats.ShadowMapSize      = m_ShadowRenderer.GetShadowMapSize();
    m_Stats.SkyPassCount       = m_SkyRenderer.GetPassCount();
    m_Stats.TransparentPassCount = m_TransparentRenderer.GetPassCount();
    m_Stats.PostProcessPassCount = m_PostProcessRenderer.GetPassCount();
}

void ForwardRenderer::Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources)
{
    m_TerrainQuadtree.Select(View.CameraPosition, m_TerrainQuadtreeSelection);

    m_RenderQueue.Clear();
    const auto& QuadtreeNodes = m_TerrainQuadtree.GetNodes();
    for (Uint32 NodeIndex : m_TerrainQuadtreeSelection.SelectedNodeIndices)
    {
        if (NodeIndex < QuadtreeNodes.size())
        {
            const TerrainDrawRegion Region = m_TerrainPatchRenderer.BuildDrawRegion(QuadtreeNodes[NodeIndex]);
            m_RenderQueue.AddTerrainLeaf(Region);
        }
    }
    m_RenderQueue.AddTransparentQuad(length(View.CameraPosition - TransparentRenderer::GetTestQuadCenter()));
    m_RenderQueue.SortTransparentBackToFront();
    m_RenderQueue.AddDebugGrid();
    m_TerrainPatchRenderer.BeginFrameStats();

    LightConstants LightData{};
    m_ShadowRenderer.FillLightConstants(LightData);
    FrameResources.UpdateLightConstants(pContext, LightData);

    m_ShadowRenderer.Render(pContext, m_TerrainPatchRenderer, m_RenderQueue.GetOpaqueItems());

    const float SceneClearColor[] = {0.08f, 0.09f, 0.10f, 1.0f};
    ITextureView* pRTV = m_PostProcessRenderer.PrepareSceneTarget(pContext, m_pSwapChain, SceneClearColor);
    ITextureView* pDSV = m_pSwapChain->GetDepthBufferDSV();
    pContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    if (View.IsGL)
        m_SkyRenderer.Render(pContext, View);

    for (const RenderItem& Item : m_RenderQueue.GetOpaqueItems())
    {
        if (Item.Kind == RenderItemKind::TerrainLeaf)
            m_TerrainPatchRenderer.Render(pContext, View, FrameResources, Item.TerrainRegion, m_ShadowRenderer.GetCascadeSRV(0));
    }

    if (!View.IsGL)
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

    if (m_ShowQuadtreeOverlay)
        m_TerrainQuadtreeDebugRenderer.Render(pContext, View, FrameResources, m_TerrainQuadtree, m_TerrainQuadtreeSelection);

    m_PostProcessRenderer.Render(pContext, m_pSwapChain);

    m_Stats.OpaqueItemCount  = m_RenderQueue.GetQueueCount(RenderQueueType::Opaque);
    m_Stats.TerrainTriangleCount = m_TerrainPatchRenderer.GetLastRenderedTriangleCount();
    m_Stats.TerrainRenderItemCount = m_RenderQueue.GetQueueCount(RenderQueueType::Opaque);
    m_Stats.TerrainRenderedCellCount = m_TerrainPatchRenderer.GetLastRenderedCellCount();
    m_Stats.TerrainForwardDrawCallCount = m_TerrainPatchRenderer.GetLastForwardDrawCallCount();
    m_Stats.TerrainShadowDrawCallCount = m_TerrainPatchRenderer.GetLastShadowDrawCallCount();
    m_Stats.TerrainCellCount = m_TerrainPatchRenderer.GetCellCount();
    m_Stats.TerrainSampleCountPerAxis = m_TerrainPatchRenderer.GetSampleCountPerAxis();
    m_Stats.TerrainMinHeight = m_TerrainPatchRenderer.GetMinHeight();
    m_Stats.TerrainMaxHeight = m_TerrainPatchRenderer.GetMaxHeight();
    m_Stats.TerrainAverageHeight = m_TerrainPatchRenderer.GetAverageHeight();
    m_Stats.TerrainQuadtreeNodeCount = static_cast<Uint32>(m_TerrainQuadtree.GetNodes().size());
    m_Stats.TerrainQuadtreeSelectedLeafCount = static_cast<Uint32>(m_TerrainQuadtreeSelection.SelectedNodeIndices.size());
    m_Stats.TerrainQuadtreeMaxDepth = m_TerrainQuadtree.GetMaxDepth();
    m_Stats.TerrainQuadtreeMaxSelectedLevel = m_TerrainQuadtreeSelection.MaxSelectedLevel;
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
