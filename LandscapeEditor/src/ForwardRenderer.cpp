#include "ForwardRenderer.hpp"

#include "AdvancedMath.hpp"
#include "DeviceContext.h"
#include "FrameResources.hpp"
#include "RenderView.hpp"
#include "SwapChain.h"

#include <algorithm>

namespace Diligent
{

namespace
{

BoundBox BuildTerrainNodeBounds(const TerrainQuadtreeNode& Node, float MinHeight, float MaxHeight, float SkirtDepth)
{
    return BoundBox{
        float3{Node.MinXZ.x, MinHeight - SkirtDepth, Node.MinXZ.y},
        float3{Node.MaxXZ.x, MaxHeight, Node.MaxXZ.y}};
}

void BuildVisibleTerrainSelection(const RenderView& View,
                                  const TerrainPatchRenderer& TerrainPatchRenderer,
                                  const std::vector<TerrainQuadtreeNode>& Nodes,
                                  const TerrainQuadtreeSelection& CandidateSelection,
                                  bool EnableFrustumCulling,
                                  TerrainQuadtreeSelection& OutVisibleSelection)
{
    OutVisibleSelection.SelectedNodeIndices.clear();
    OutVisibleSelection.MinSelectedLevel = 0;
    OutVisibleSelection.MaxSelectedLevel = 0;

    ViewFrustum Frustum;
    ExtractViewFrustumPlanesFromMatrix(View.ViewProj, Frustum, View.IsGL);

    const float MinHeight = TerrainPatchRenderer.GetMinHeight();
    const float MaxHeight = TerrainPatchRenderer.GetMaxHeight();
    const float SkirtDepth = TerrainPatchRenderer.GetEnableSkirts() ? TerrainPatchRenderer.GetSkirtDepth() : 0.0f;

    for (Uint32 NodeIndex : CandidateSelection.SelectedNodeIndices)
    {
        if (NodeIndex >= Nodes.size())
            continue;

        const TerrainQuadtreeNode& Node = Nodes[NodeIndex];
        const BoundBox Bounds = BuildTerrainNodeBounds(Node, MinHeight, MaxHeight, SkirtDepth);
        const bool IsVisible = !EnableFrustumCulling || GetBoxVisibility(Frustum, Bounds) != BoxVisibility::Invisible;
        if (IsVisible)
        {
            OutVisibleSelection.SelectedNodeIndices.push_back(NodeIndex);
            OutVisibleSelection.MinSelectedLevel = OutVisibleSelection.SelectedNodeIndices.size() == 1u ? Node.Level : std::min(OutVisibleSelection.MinSelectedLevel, Node.Level);
            OutVisibleSelection.MaxSelectedLevel = std::max(OutVisibleSelection.MaxSelectedLevel, Node.Level);
        }
    }
}

void UpdateTerrainComponentLODStats(const TerrainQuadtree& Quadtree,
                                    const TerrainQuadtreeSelection& Selection,
                                    ForwardRendererStats& Stats)
{
    const TerrainComponentLODPolicy& Policy = Quadtree.GetLODPolicy();
    Stats.TerrainLODDistanceScale = Policy.SplitDistanceScale;
    Stats.TerrainMaxSelectableLODLevel = Quadtree.GetMaxSelectableLevel();
    Stats.TerrainRootComponentWorldSize = Quadtree.GetComponentWorldSize(0);
    Stats.TerrainFineComponentWorldSize = Quadtree.GetComponentWorldSize(Quadtree.GetMaxDepth());

    if (Selection.SelectedNodeIndices.empty())
    {
        Stats.TerrainQuadtreeMinSelectedLevel = 0;
        Stats.TerrainSelectedMinComponentWorldSize = 0.0f;
        Stats.TerrainSelectedMaxComponentWorldSize = 0.0f;
        return;
    }

    Stats.TerrainQuadtreeMinSelectedLevel = Selection.MinSelectedLevel;
    Stats.TerrainSelectedMinComponentWorldSize = Quadtree.GetComponentWorldSize(Selection.MaxSelectedLevel);
    Stats.TerrainSelectedMaxComponentWorldSize = Quadtree.GetComponentWorldSize(Selection.MinSelectedLevel);
}

void UpdateTerrainLODStitchingStats(const TerrainLODStitching& Stitching, ForwardRendererStats& Stats)
{
    const TerrainLODStitchingStats& StitchingStats = Stitching.GetStats();
    Stats.TerrainLODStitchingSeamEdgeCount = StitchingStats.SeamEdgeCount;
    Stats.TerrainLODStitchingMaxDelta      = StitchingStats.MaxLODDelta;
    Stats.TerrainLODStitchingMaxRatio      = StitchingStats.MaxStitchRatio;
    Stats.TerrainLODStitchingTotalLength   = StitchingStats.TotalSeamLength;
}

void UpdateTerrainLODIndexStitchingStats(const TerrainLODIndexStitching& Stitching, ForwardRendererStats& Stats)
{
    const TerrainLODIndexStitchingStats& StitchingStats = Stitching.GetStats();
    Stats.TerrainLODIndexStitchingNodeCount = StitchingStats.StitchedNodeCount;
    Stats.TerrainLODIndexStitchingEdgeCount = StitchingStats.StitchedEdgeCount;
    Stats.TerrainLODIndexStitchingCornerCount = StitchingStats.StitchedCornerCount;
    Stats.TerrainLODIndexStitchingIndexCount = StitchingStats.GeneratedIndexCount;
    Stats.TerrainLODIndexStitchingCornerIndexCount = StitchingStats.CornerPatchIndexCount;
    Stats.TerrainLODIndexStitchingMaxRatio = StitchingStats.MaxStitchRatio;
}

void UpdateTerrainHeightmapTileStats(const TerrainPatchRenderer& TerrainPatchRenderer, ForwardRendererStats& Stats)
{
    Stats.TerrainHeightmapLayoutName = TerrainPatchRenderer.GetHeightmapLayoutName();
    Stats.TerrainHeightmapTileCountX = TerrainPatchRenderer.GetHeightmapTileCountX();
    Stats.TerrainHeightmapTileCountZ = TerrainPatchRenderer.GetHeightmapTileCountZ();
    Stats.TerrainHeightmapTileCount = TerrainPatchRenderer.GetHeightmapTileCount();
    Stats.TerrainHeightmapTileSampleCountPerAxis = TerrainPatchRenderer.GetHeightmapTileSampleCountPerAxis();
    Stats.TerrainHeightmapTileCellCount = TerrainPatchRenderer.GetHeightmapTileCellCount();
    Stats.TerrainHeightmapPackageCellCountX = TerrainPatchRenderer.GetHeightmapPackageCellCountX();
    Stats.TerrainHeightmapPackageCellCountZ = TerrainPatchRenderer.GetHeightmapPackageCellCountZ();
    Stats.TerrainHeightmapTileWorldSizeX = TerrainPatchRenderer.GetHeightmapTileWorldSizeX();
    Stats.TerrainHeightmapTileWorldSizeZ = TerrainPatchRenderer.GetHeightmapTileWorldSizeZ();
}

void ApplyLODIndexStitching(TerrainDrawRegion& Region, const TerrainLODIndexStitching& Stitching)
{
    const TerrainLODStitchedDrawRegion* StitchedRegion = Stitching.FindRegion(Region.NodeIndex);
    if (StitchedRegion == nullptr || StitchedRegion->NumIndices == 0)
        return;

    Region.UseStitchedIndexBuffer = true;
    Region.StitchedFirstIndexLocation = StitchedRegion->FirstIndexLocation;
    Region.StitchedMainNumIndices = StitchedRegion->MainNumIndices;
    Region.StitchedNumIndices = StitchedRegion->NumIndices;
}

} // namespace

void ForwardRenderer::SetTerrainLODDistanceScale(float Scale)
{
    TerrainComponentLODPolicy Policy = m_TerrainQuadtree.GetLODPolicy();
    Policy.SplitDistanceScale = Scale;
    m_TerrainQuadtree.SetLODPolicy(Policy);
}

void ForwardRenderer::SetTerrainMaxSelectedLODLevel(Uint32 Level)
{
    TerrainComponentLODPolicy Policy = m_TerrainQuadtree.GetLODPolicy();
    Policy.MaxSelectedLevel = Level;
    m_TerrainQuadtree.SetLODPolicy(Policy);
}

void ForwardRenderer::SetTerrainHeightmapRawR16(const std::string& Path, Uint32 SampleCountPerAxis, float HeightScale)
{
    m_TerrainPatchRendererDesc.HeightmapRawR16Path = Path;
    m_TerrainPatchRendererDesc.HeightmapSampleCountPerAxis = SampleCountPerAxis;
    m_TerrainPatchRendererDesc.HeightField.HeightScale = HeightScale;
}

void ForwardRenderer::SetTerrainHeightmapRawR16Tiles(const std::string& Pattern, Uint32 TileCountX, Uint32 TileCountZ, Uint32 TileSampleCountPerAxis, float HeightScale)
{
    m_TerrainPatchRendererDesc.HeightmapRawR16TilesPattern = Pattern;
    m_TerrainPatchRendererDesc.HeightmapTileCountX = std::max(TileCountX, 1u);
    m_TerrainPatchRendererDesc.HeightmapTileCountZ = std::max(TileCountZ, 1u);
    m_TerrainPatchRendererDesc.HeightmapSampleCountPerAxis = TileSampleCountPerAxis;
    m_TerrainPatchRendererDesc.HeightField.HeightScale = HeightScale;
}

void ForwardRenderer::SetTerrainHeightmapTileGrid(Uint32 TileCountX, Uint32 TileCountZ)
{
    m_TerrainPatchRendererDesc.HeightmapTileCountX = std::max(TileCountX, 1u);
    m_TerrainPatchRendererDesc.HeightmapTileCountZ = std::max(TileCountZ, 1u);
}

void ForwardRenderer::Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    m_pSwapChain = pSwapChain;
    m_ShadowRenderer.Initialize(pDevice, pDevice->GetDeviceInfo().NDC.MinZ == -1);
    m_PostProcessRenderer.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_SkyRenderer.Initialize(pDevice, pSwapChain, m_PSOCache);
    TerrainQuadtreeDesc QuadtreeDesc;
    m_TerrainQuadtree.Build(QuadtreeDesc);
    m_TerrainPatchRenderer.Initialize(pDevice, pSwapChain, m_PSOCache, m_TerrainQuadtree.GetNodes(), m_TerrainPatchRendererDesc);
    m_TerrainQuadtreeDebugRenderer.Initialize(pDevice, pSwapChain, m_PSOCache, m_TerrainQuadtree.GetSelectedLeafCapacity() * 32u);
    m_TransparentRenderer.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_ForwardDebugPipeline.Initialize(pDevice, pSwapChain, m_PSOCache);
    m_Stats.PSOCount         = m_PSOCache.GetPSOCount();
    m_Stats.PSOCreationCount = m_PSOCache.GetCreationCount();
    m_Stats.TerrainTriangleCount = m_TerrainPatchRenderer.GetTriangleCount();
    m_Stats.TerrainRenderItemCount = 0;
    m_Stats.TerrainRenderedCellCount = 0;
    m_Stats.TerrainRenderedMeshCellCount = 0;
    m_Stats.TerrainForwardDrawCallCount = 0;
    m_Stats.TerrainShadowDrawCallCount = 0;
    m_Stats.TerrainTileMeshCount = m_TerrainPatchRenderer.GetTileMeshCount();
    m_Stats.TerrainPackedVertexCount = m_TerrainPatchRenderer.GetPackedTileVertexCount();
    m_Stats.TerrainPackedIndexCount = m_TerrainPatchRenderer.GetPackedTileIndexCount();
    m_Stats.TerrainSkirtVertexCount = m_TerrainPatchRenderer.GetPackedTileSkirtVertexCount();
    m_Stats.TerrainSkirtIndexCount = m_TerrainPatchRenderer.GetPackedTileSkirtIndexCount();
    m_Stats.TerrainSkirtDepth = m_TerrainPatchRenderer.GetSkirtDepth();
    m_Stats.TerrainSkirtsEnabled = m_TerrainPatchRenderer.GetEnableSkirts();
    m_Stats.TerrainMinLODSampleStep = m_TerrainPatchRenderer.GetMinLODSampleStep();
    m_Stats.TerrainMaxLODSampleStep = m_TerrainPatchRenderer.GetMaxLODSampleStep();
    m_Stats.TerrainCellCount = m_TerrainPatchRenderer.GetCellCount();
    m_Stats.TerrainCellCountX = m_TerrainPatchRenderer.GetCellCountX();
    m_Stats.TerrainCellCountZ = m_TerrainPatchRenderer.GetCellCountZ();
    m_Stats.TerrainSampleCountPerAxis = m_TerrainPatchRenderer.GetSampleCountPerAxis();
    m_Stats.TerrainSampleCountX = m_TerrainPatchRenderer.GetSampleCountX();
    m_Stats.TerrainSampleCountZ = m_TerrainPatchRenderer.GetSampleCountZ();
    m_Stats.TerrainMinHeight = m_TerrainPatchRenderer.GetMinHeight();
    m_Stats.TerrainMaxHeight = m_TerrainPatchRenderer.GetMaxHeight();
    m_Stats.TerrainAverageHeight = m_TerrainPatchRenderer.GetAverageHeight();
    m_Stats.TerrainHeightmapLoaded = m_TerrainPatchRenderer.IsHeightmapLoaded();
    m_Stats.TerrainHeightSourceName = m_TerrainPatchRenderer.GetHeightSourceName();
    UpdateTerrainHeightmapTileStats(m_TerrainPatchRenderer, m_Stats);
    m_Stats.TerrainQuadtreeNodeCount = static_cast<Uint32>(m_TerrainQuadtree.GetNodes().size());
    m_Stats.TerrainQuadtreeSelectedLeafCount = static_cast<Uint32>(m_TerrainQuadtreeSelection.SelectedNodeIndices.size());
    m_Stats.TerrainFrustumCullingEnabled = m_EnableTerrainFrustumCulling;
    m_Stats.TerrainQuadtreeCandidateLeafCount = static_cast<Uint32>(m_TerrainQuadtreeSelection.SelectedNodeIndices.size());
    m_Stats.TerrainQuadtreeVisibleLeafCount = static_cast<Uint32>(m_VisibleTerrainQuadtreeSelection.SelectedNodeIndices.size());
    m_Stats.TerrainFrustumCulledLeafCount = 0;
    m_Stats.TerrainQuadtreeMaxDepth = m_TerrainQuadtree.GetMaxDepth();
    m_Stats.TerrainQuadtreeMaxSelectedLevel = m_TerrainQuadtreeSelection.MaxSelectedLevel;
    UpdateTerrainComponentLODStats(m_TerrainQuadtree, m_TerrainQuadtreeSelection, m_Stats);
    UpdateTerrainLODStitchingStats(m_TerrainLODStitching, m_Stats);
    UpdateTerrainLODIndexStitchingStats(m_TerrainLODIndexStitching, m_Stats);
    m_Stats.TerrainLODIndexStitchingEnabled = m_EnableTerrainLODIndexStitching;
    const auto& InitialDebugStats = m_TerrainQuadtreeDebugRenderer.GetStats();
    m_Stats.TerrainDebugLeafBoundLineCount = InitialDebugStats.LeafBoundLineCount;
    m_Stats.TerrainDebugSkirtEdgeCount = InitialDebugStats.SkirtEdgeCount;
    m_Stats.TerrainDebugLODTransitionEdgeCount = InitialDebugStats.LODTransitionEdgeCount;
    m_Stats.TerrainDebugLineVertexCount = InitialDebugStats.LineVertexCount;
    m_Stats.ShadowCascadeCount = ShadowRenderer::CascadeCount;
    m_Stats.ShadowMapSize      = m_ShadowRenderer.GetShadowMapSize();
    m_Stats.SkyPassCount       = m_SkyRenderer.GetPassCount();
    m_Stats.TransparentPassCount = m_TransparentRenderer.GetPassCount();
    m_Stats.PostProcessPassCount = m_PostProcessRenderer.GetPassCount();
}

void ForwardRenderer::Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources)
{
    m_TerrainQuadtree.Select(View.CameraPosition, m_TerrainQuadtreeSelection);
    const auto& QuadtreeNodes = m_TerrainQuadtree.GetNodes();
    BuildVisibleTerrainSelection(View,
                                 m_TerrainPatchRenderer,
                                 QuadtreeNodes,
                                 m_TerrainQuadtreeSelection,
                                 m_EnableTerrainFrustumCulling,
                                 m_VisibleTerrainQuadtreeSelection);
    m_TerrainLODStitching.Build(QuadtreeNodes, m_VisibleTerrainQuadtreeSelection);
    if (m_EnableTerrainLODIndexStitching)
    {
        m_TerrainLODIndexStitching.Build(QuadtreeNodes, m_VisibleTerrainQuadtreeSelection, m_TerrainLODStitching);
        m_TerrainPatchRenderer.PrepareLODIndexStitching(pContext, m_TerrainLODIndexStitching);
    }
    else
    {
        TerrainQuadtreeSelection EmptySelection;
        m_TerrainLODIndexStitching.Build(QuadtreeNodes, EmptySelection, m_TerrainLODStitching);
    }

    m_RenderQueue.Clear();
    for (Uint32 NodeIndex : m_VisibleTerrainQuadtreeSelection.SelectedNodeIndices)
    {
        if (NodeIndex < QuadtreeNodes.size())
        {
            TerrainDrawRegion Region = m_TerrainPatchRenderer.BuildDrawRegion(QuadtreeNodes[NodeIndex]);
            if (m_EnableTerrainLODIndexStitching)
                ApplyLODIndexStitching(Region, m_TerrainLODIndexStitching);
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
        m_TerrainQuadtreeDebugRenderer.Render(pContext, View, FrameResources, m_TerrainQuadtree, m_VisibleTerrainQuadtreeSelection, m_TerrainLODStitching);

    m_PostProcessRenderer.Render(pContext, m_pSwapChain);

    m_Stats.OpaqueItemCount  = m_RenderQueue.GetQueueCount(RenderQueueType::Opaque);
    m_Stats.TerrainTriangleCount = m_TerrainPatchRenderer.GetLastRenderedTriangleCount();
    m_Stats.TerrainRenderItemCount = m_RenderQueue.GetQueueCount(RenderQueueType::Opaque);
    m_Stats.TerrainRenderedCellCount = m_TerrainPatchRenderer.GetLastRenderedCellCount();
    m_Stats.TerrainRenderedMeshCellCount = m_TerrainPatchRenderer.GetLastRenderedMeshCellCount();
    m_Stats.TerrainForwardDrawCallCount = m_TerrainPatchRenderer.GetLastForwardDrawCallCount();
    m_Stats.TerrainShadowDrawCallCount = m_TerrainPatchRenderer.GetLastShadowDrawCallCount();
    m_Stats.TerrainTileMeshCount = m_TerrainPatchRenderer.GetTileMeshCount();
    m_Stats.TerrainPackedVertexCount = m_TerrainPatchRenderer.GetPackedTileVertexCount();
    m_Stats.TerrainPackedIndexCount = m_TerrainPatchRenderer.GetPackedTileIndexCount();
    m_Stats.TerrainSkirtVertexCount = m_TerrainPatchRenderer.GetPackedTileSkirtVertexCount();
    m_Stats.TerrainSkirtIndexCount = m_TerrainPatchRenderer.GetPackedTileSkirtIndexCount();
    m_Stats.TerrainSkirtDepth = m_TerrainPatchRenderer.GetSkirtDepth();
    m_Stats.TerrainSkirtsEnabled = m_TerrainPatchRenderer.GetEnableSkirts();
    m_Stats.TerrainMinLODSampleStep = m_TerrainPatchRenderer.GetMinLODSampleStep();
    m_Stats.TerrainMaxLODSampleStep = m_TerrainPatchRenderer.GetMaxLODSampleStep();
    m_Stats.TerrainCellCount = m_TerrainPatchRenderer.GetCellCount();
    m_Stats.TerrainCellCountX = m_TerrainPatchRenderer.GetCellCountX();
    m_Stats.TerrainCellCountZ = m_TerrainPatchRenderer.GetCellCountZ();
    m_Stats.TerrainSampleCountPerAxis = m_TerrainPatchRenderer.GetSampleCountPerAxis();
    m_Stats.TerrainSampleCountX = m_TerrainPatchRenderer.GetSampleCountX();
    m_Stats.TerrainSampleCountZ = m_TerrainPatchRenderer.GetSampleCountZ();
    m_Stats.TerrainMinHeight = m_TerrainPatchRenderer.GetMinHeight();
    m_Stats.TerrainMaxHeight = m_TerrainPatchRenderer.GetMaxHeight();
    m_Stats.TerrainAverageHeight = m_TerrainPatchRenderer.GetAverageHeight();
    m_Stats.TerrainHeightmapLoaded = m_TerrainPatchRenderer.IsHeightmapLoaded();
    m_Stats.TerrainHeightSourceName = m_TerrainPatchRenderer.GetHeightSourceName();
    UpdateTerrainHeightmapTileStats(m_TerrainPatchRenderer, m_Stats);
    m_Stats.TerrainQuadtreeNodeCount = static_cast<Uint32>(m_TerrainQuadtree.GetNodes().size());
    m_Stats.TerrainQuadtreeSelectedLeafCount = static_cast<Uint32>(m_TerrainQuadtreeSelection.SelectedNodeIndices.size());
    m_Stats.TerrainFrustumCullingEnabled = m_EnableTerrainFrustumCulling;
    m_Stats.TerrainQuadtreeCandidateLeafCount = static_cast<Uint32>(m_TerrainQuadtreeSelection.SelectedNodeIndices.size());
    m_Stats.TerrainQuadtreeVisibleLeafCount = static_cast<Uint32>(m_VisibleTerrainQuadtreeSelection.SelectedNodeIndices.size());
    m_Stats.TerrainFrustumCulledLeafCount = m_Stats.TerrainQuadtreeCandidateLeafCount >= m_Stats.TerrainQuadtreeVisibleLeafCount ?
        m_Stats.TerrainQuadtreeCandidateLeafCount - m_Stats.TerrainQuadtreeVisibleLeafCount :
        0u;
    m_Stats.TerrainQuadtreeMaxDepth = m_TerrainQuadtree.GetMaxDepth();
    m_Stats.TerrainQuadtreeMaxSelectedLevel = m_VisibleTerrainQuadtreeSelection.MaxSelectedLevel;
    UpdateTerrainComponentLODStats(m_TerrainQuadtree, m_VisibleTerrainQuadtreeSelection, m_Stats);
    UpdateTerrainLODStitchingStats(m_TerrainLODStitching, m_Stats);
    UpdateTerrainLODIndexStitchingStats(m_TerrainLODIndexStitching, m_Stats);
    m_Stats.TerrainLODIndexStitchingEnabled = m_EnableTerrainLODIndexStitching;
    const auto& DebugStats = m_TerrainQuadtreeDebugRenderer.GetStats();
    m_Stats.TerrainDebugLeafBoundLineCount = DebugStats.LeafBoundLineCount;
    m_Stats.TerrainDebugSkirtEdgeCount = DebugStats.SkirtEdgeCount;
    m_Stats.TerrainDebugLODTransitionEdgeCount = DebugStats.LODTransitionEdgeCount;
    m_Stats.TerrainDebugLineVertexCount = DebugStats.LineVertexCount;
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
