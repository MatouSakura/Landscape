#pragma once

#include "ForwardDebugPipeline.hpp"
#include "PostProcessRenderer.hpp"
#include "PSOCache.hpp"
#include "RenderQueue.hpp"
#include "ShadowRenderer.hpp"
#include "SkyRenderer.hpp"
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
    Uint32 TerrainForwardDrawCallCount = 0;
    Uint32 TerrainShadowDrawCallCount = 0;
    Uint32 TerrainTileMeshCount = 0;
    Uint32 TerrainPackedVertexCount = 0;
    Uint32 TerrainPackedIndexCount = 0;
    bool   TerrainSkirtsEnabled = true;
    float  TerrainSkirtDepth = 0.0f;
    Uint32 TerrainSkirtVertexCount = 0;
    Uint32 TerrainSkirtIndexCount = 0;
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
    Uint32 TerrainQuadtreeMaxDepth = 0;
    Uint32 TerrainQuadtreeMaxSelectedLevel = 0;
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
    void SetTerrainSkirtsEnabled(bool Enable) { m_TerrainPatchRenderer.SetEnableSkirts(Enable); }
    bool GetTerrainSkirtsEnabled() const { return m_TerrainPatchRenderer.GetEnableSkirts(); }

private:
    ForwardDebugPipeline  m_ForwardDebugPipeline;
    TerrainPatchRenderer  m_TerrainPatchRenderer;
    TerrainQuadtree       m_TerrainQuadtree;
    TerrainQuadtreeSelection m_TerrainQuadtreeSelection;
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
};

} // namespace Diligent
