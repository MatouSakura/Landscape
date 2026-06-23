#pragma once

#include "Buffer.h"
#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"
#include "TerrainLODStitching.hpp"

namespace Diligent
{

struct IDeviceContext;
struct IRenderDevice;
struct IShaderResourceBinding;
struct ISwapChain;
class FrameResources;
class PSOCache;
class TerrainQuadtree;
struct TerrainQuadtreeSelection;
struct RenderView;

struct TerrainQuadtreeDebugStats final
{
    Uint32 LeafBoundLineCount    = 0;
    Uint32 SkirtEdgeCount        = 0;
    Uint32 LODTransitionEdgeCount = 0;
    Uint32 LineVertexCount       = 0;
};

class TerrainQuadtreeDebugRenderer final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache, Uint32 MaxLineVertexCount);
    void Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources, const TerrainQuadtree& Quadtree, const TerrainQuadtreeSelection& Selection, const TerrainLODStitching& Stitching);

    void SetShowSkirtEdges(bool Show) { m_ShowSkirtEdges = Show; }
    void SetShowLODTransitionEdges(bool Show) { m_ShowLODTransitionEdges = Show; }
    Uint32 GetLastLineVertexCount() const { return m_LastLineVertexCount; }
    const TerrainQuadtreeDebugStats& GetStats() const { return m_Stats; }

private:
    RefCntAutoPtr<IBuffer>                m_pVertexBuffer;
    RefCntAutoPtr<IPipelineState>         m_pPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
    Uint32                                m_MaxLineVertexCount  = 0;
    Uint32                                m_LastLineVertexCount = 0;
    TerrainQuadtreeDebugStats             m_Stats;
    bool                                  m_ShowSkirtEdges = true;
    bool                                  m_ShowLODTransitionEdges = true;
};

} // namespace Diligent
