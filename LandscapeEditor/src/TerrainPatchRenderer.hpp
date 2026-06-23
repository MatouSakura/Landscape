#pragma once

#include "Buffer.h"
#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"
#include "RenderQueue.hpp"
#include "TerrainHeightField.hpp"
#include "TerrainQuadtree.hpp"

#include <vector>

namespace Diligent
{

struct IDeviceContext;
struct IRenderDevice;
struct IShaderResourceBinding;
struct ISwapChain;
struct ITextureView;
class FrameResources;
class PSOCache;
struct RenderView;

struct TerrainTileMeshRange final
{
    TerrainDrawRegion Region;
    Uint32            VertexCount = 0;
};

class TerrainPatchRenderer final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache, const std::vector<TerrainQuadtreeNode>& QuadtreeNodes);
    TerrainDrawRegion BuildDrawRegion(const TerrainQuadtreeNode& Node) const;
    void BeginFrameStats();
    void SetEnableSkirts(bool Enable) { m_EnableSkirts = Enable; }
    bool GetEnableSkirts() const { return m_EnableSkirts; }
    float GetSkirtDepth() const { return m_SkirtDepth; }
    void Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources, const TerrainDrawRegion& Region, ITextureView* pShadowMapSRV);
    void RenderShadow(IDeviceContext* pContext, IBuffer* pShadowConstants, const TerrainDrawRegion& Region);

    Uint32 GetTriangleCount() const { return m_IndexCount / 3u; }
    Uint32 GetLastRenderedTriangleCount() const { return m_LastRenderedIndexCount / 3u; }
    Uint32 GetLastRenderedCellCount() const { return m_LastRenderedCellCount; }
    Uint32 GetLastForwardDrawCallCount() const { return m_LastForwardDrawCallCount; }
    Uint32 GetLastShadowDrawCallCount() const { return m_LastShadowDrawCallCount; }
    Uint32 GetTileMeshCount() const { return static_cast<Uint32>(m_TileMeshRanges.size()); }
    Uint32 GetPackedTileVertexCount() const { return m_PackedTileVertexCount; }
    Uint32 GetPackedTileIndexCount() const { return m_PackedTileIndexCount; }
    Uint32 GetPackedTileSkirtVertexCount() const { return m_PackedTileSkirtVertexCount; }
    Uint32 GetPackedTileSkirtIndexCount() const { return m_PackedTileSkirtIndexCount; }
    Uint32 GetCellCount() const { return m_HeightField.GetCellCount(); }
    Uint32 GetSampleCountPerAxis() const { return m_HeightField.GetSampleCountPerAxis(); }
    float  GetMinHeight() const { return m_HeightField.GetStats().MinHeight; }
    float  GetMaxHeight() const { return m_HeightField.GetStats().MaxHeight; }
    float  GetAverageHeight() const { return m_HeightField.GetStats().AverageHeight; }

private:
    void BuildPackedTileMeshCache(IRenderDevice* pDevice, const std::vector<TerrainQuadtreeNode>& QuadtreeNodes);
    void DrawTileMeshIndexed(IDeviceContext* pContext, const TerrainDrawRegion& Region) const;

private:
    TerrainHeightField                    m_HeightField;
    std::vector<TerrainTileMeshRange>      m_TileMeshRanges;
    RefCntAutoPtr<IBuffer>                m_pVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_pIndexBuffer;
    RefCntAutoPtr<IPipelineState>         m_pTerrainPSO;
    RefCntAutoPtr<IPipelineState>         m_pShadowPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pTerrainSRB;
    RefCntAutoPtr<IShaderResourceBinding> m_pShadowSRB;
    Uint32                                m_IndexCount = 0;
    Uint32                                m_LastRenderedCellCount = 0;
    Uint32                                m_LastRenderedIndexCount = 0;
    Uint32                                m_LastForwardDrawCallCount = 0;
    Uint32                                m_LastShadowDrawCallCount = 0;
    Uint32                                m_PackedTileVertexCount = 0;
    Uint32                                m_PackedTileIndexCount = 0;
    Uint32                                m_PackedTileSkirtVertexCount = 0;
    Uint32                                m_PackedTileSkirtIndexCount = 0;
    float                                 m_TerrainExtent = 20.0f;
    float                                 m_SkirtDepth = 1.25f;
    bool                                  m_EnableSkirts = true;
};

} // namespace Diligent
