#pragma once

#include "ForwardDebugPipeline.hpp"
#include "PostProcessRenderer.hpp"
#include "PSOCache.hpp"
#include "RenderQueue.hpp"
#include "ShadowRenderer.hpp"
#include "SkyRenderer.hpp"
#include "TerrainLODIndexStitching.hpp"
#include "TerrainLODStitching.hpp"
#include "TerrainPatchRenderer.hpp"
#include "TerrainQuadtree.hpp"
#include "TerrainQuadtreeDebugRenderer.hpp"
#include "TransparentRenderer.hpp"

namespace Diligent
{

struct IDeviceContext;
struct IRenderDevice;
struct ISwapChain;
class FrameResources;
struct RenderView;

struct ForwardRendererStats final
{
    Uint32 OpaqueItemCount    = 0;
    Uint32 TerrainTriangleCount = 0;
    Uint32 TerrainRenderItemCount = 0;
    Uint32 TerrainRenderedCellCount = 0;
    Uint32 TerrainRenderedMeshCellCount = 0;
    Uint32 TerrainForwardDrawCallCount = 0;
    Uint32 TerrainShadowDrawCallCount = 0;
    Uint32 TerrainTileMeshCount = 0;
    Uint32 TerrainPackedVertexCount = 0;
    Uint32 TerrainPackedIndexCount = 0;
    bool   TerrainSkirtsEnabled = true;
    float  TerrainSkirtDepth = 0.0f;
    Uint32 TerrainSkirtVertexCount = 0;
    Uint32 TerrainSkirtIndexCount = 0;
    Uint32 TerrainMinLODSampleStep = 1;
    Uint32 TerrainMaxLODSampleStep = 1;
    Uint32 TerrainCellCount   = 0;
    Uint32 TerrainSampleCountPerAxis = 0;
    float  TerrainMinHeight   = 0.0f;
    float  TerrainMaxHeight   = 0.0f;
    float  TerrainAverageHeight = 0.0f;
    Uint32 ShadowCascadeCount = 0;
    Uint32 ShadowMapSize      = 0;
    Uint32 SkyPassCount       = 0;
    Uint32 TransparentItemCount = 0;
    Uint32 TransparentPassCount = 0;
    Uint32 DebugItemCount     = 0;
    Uint32 TerrainQuadtreeNodeCount = 0;
    Uint32 TerrainQuadtreeSelectedLeafCount = 0;
    bool   TerrainFrustumCullingEnabled = true;
    Uint32 TerrainQuadtreeCandidateLeafCount = 0;
    Uint32 TerrainQuadtreeVisibleLeafCount = 0;
    Uint32 TerrainFrustumCulledLeafCount = 0;
    float  TerrainLODDistanceScale = 2.2f;
    Uint32 TerrainMaxSelectableLODLevel = 0;
    Uint32 TerrainQuadtreeMinSelectedLevel = 0;
    Uint32 TerrainQuadtreeMaxDepth = 0;
    Uint32 TerrainQuadtreeMaxSelectedLevel = 0;
    float  TerrainRootComponentWorldSize = 0.0f;
    float  TerrainFineComponentWorldSize = 0.0f;
    float  TerrainSelectedMinComponentWorldSize = 0.0f;
    float  TerrainSelectedMaxComponentWorldSize = 0.0f;
    Uint32 TerrainDebugLeafBoundLineCount = 0;
    Uint32 TerrainDebugSkirtEdgeCount = 0;
    Uint32 TerrainDebugLODTransitionEdgeCount = 0;
    Uint32 TerrainDebugLineVertexCount = 0;
    Uint32 TerrainLODStitchingSeamEdgeCount = 0;
    Uint32 TerrainLODStitchingMaxDelta = 0;
    Uint32 TerrainLODStitchingMaxRatio = 1;
    float  TerrainLODStitchingTotalLength = 0.0f;
    bool   TerrainLODIndexStitchingEnabled = true;
    Uint32 TerrainLODIndexStitchingNodeCount = 0;
    Uint32 TerrainLODIndexStitchingEdgeCount = 0;
    Uint32 TerrainLODIndexStitchingCornerCount = 0;
    Uint32 TerrainLODIndexStitchingIndexCount = 0;
    Uint32 TerrainLODIndexStitchingCornerIndexCount = 0;
    Uint32 TerrainLODIndexStitchingMaxRatio = 1;
    Uint32 PostProcessPassCount = 0;
    size_t PSOCount           = 0;
    size_t PSOCreationCount = 0;
};

class ForwardRenderer final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain);
    void Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources);

    const ForwardRendererStats& GetStats() const { return m_Stats; }
    void SetShowQuadtreeOverlay(bool Show) { m_ShowQuadtreeOverlay = Show; }
    bool GetShowQuadtreeOverlay() const { return m_ShowQuadtreeOverlay; }
    void SetShowSkirtEdgeOverlay(bool Show) { m_TerrainQuadtreeDebugRenderer.SetShowSkirtEdges(Show); }
    void SetShowLODTransitionOverlay(bool Show) { m_TerrainQuadtreeDebugRenderer.SetShowLODTransitionEdges(Show); }
    void SetTerrainSkirtsEnabled(bool Enable) { m_TerrainPatchRenderer.SetEnableSkirts(Enable); }
    bool GetTerrainSkirtsEnabled() const { return m_TerrainPatchRenderer.GetEnableSkirts(); }
    void SetTerrainFrustumCullingEnabled(bool Enable) { m_EnableTerrainFrustumCulling = Enable; }
    bool GetTerrainFrustumCullingEnabled() const { return m_EnableTerrainFrustumCulling; }
    void SetTerrainLODDistanceScale(float Scale);
    float GetTerrainLODDistanceScale() const { return m_TerrainQuadtree.GetLODPolicy().SplitDistanceScale; }
    void SetTerrainMaxSelectedLODLevel(Uint32 Level);
    Uint32 GetTerrainMaxSelectedLODLevel() const { return m_TerrainQuadtree.GetLODPolicy().MaxSelectedLevel; }
    void SetTerrainLODIndexStitchingEnabled(bool Enable) { m_EnableTerrainLODIndexStitching = Enable; }
    bool GetTerrainLODIndexStitchingEnabled() const { return m_EnableTerrainLODIndexStitching; }

private:
    ForwardDebugPipeline  m_ForwardDebugPipeline;
    TerrainPatchRenderer  m_TerrainPatchRenderer;
    TerrainQuadtree       m_TerrainQuadtree;
    TerrainQuadtreeSelection m_TerrainQuadtreeSelection;
    TerrainQuadtreeSelection m_VisibleTerrainQuadtreeSelection;
    TerrainLODStitching   m_TerrainLODStitching;
    TerrainLODIndexStitching m_TerrainLODIndexStitching;
    TerrainQuadtreeDebugRenderer m_TerrainQuadtreeDebugRenderer;
    ShadowRenderer        m_ShadowRenderer;
    SkyRenderer           m_SkyRenderer;
    TransparentRenderer   m_TransparentRenderer;
    PostProcessRenderer   m_PostProcessRenderer;
    RenderQueue           m_RenderQueue;
    PSOCache              m_PSOCache;
    ForwardRendererStats  m_Stats;
    ISwapChain*           m_pSwapChain = nullptr;
    bool                  m_ShowQuadtreeOverlay = true;
    bool                  m_EnableTerrainFrustumCulling = true;
    bool                  m_EnableTerrainLODIndexStitching = true;
};

} // namespace Diligent
