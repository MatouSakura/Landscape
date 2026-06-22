#pragma once

#include "Buffer.h"
#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"
#include "RenderQueue.hpp"
#include "TerrainHeightField.hpp"
#include "TerrainQuadtree.hpp"

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

class TerrainPatchRenderer final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache);
    TerrainDrawRegion BuildDrawRegion(const TerrainQuadtreeNode& Node) const;
    void BeginFrameStats();
    void Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources, const TerrainDrawRegion& Region, ITextureView* pShadowMapSRV);
    void RenderShadow(IDeviceContext* pContext, IBuffer* pShadowConstants, const TerrainDrawRegion& Region);

    Uint32 GetTriangleCount() const { return m_IndexCount / 3u; }
    Uint32 GetLastRenderedTriangleCount() const { return m_LastRenderedCellCount * 2u; }
    Uint32 GetLastRenderedCellCount() const { return m_LastRenderedCellCount; }
    Uint32 GetLastForwardDrawCallCount() const { return m_LastForwardDrawCallCount; }
    Uint32 GetLastShadowDrawCallCount() const { return m_LastShadowDrawCallCount; }
    Uint32 GetCellCount() const { return m_HeightField.GetCellCount(); }
    Uint32 GetSampleCountPerAxis() const { return m_HeightField.GetSampleCountPerAxis(); }
    float  GetMinHeight() const { return m_HeightField.GetStats().MinHeight; }
    float  GetMaxHeight() const { return m_HeightField.GetStats().MaxHeight; }
    float  GetAverageHeight() const { return m_HeightField.GetStats().AverageHeight; }

private:
    Uint32 DrawRegionIndexed(IDeviceContext* pContext, const TerrainDrawRegion& Region) const;

private:
    TerrainHeightField                    m_HeightField;
    RefCntAutoPtr<IBuffer>                m_pVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_pIndexBuffer;
    RefCntAutoPtr<IPipelineState>         m_pTerrainPSO;
    RefCntAutoPtr<IPipelineState>         m_pShadowPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pTerrainSRB;
    RefCntAutoPtr<IShaderResourceBinding> m_pShadowSRB;
    Uint32                                m_IndexCount = 0;
    Uint32                                m_LastRenderedCellCount = 0;
    Uint32                                m_LastForwardDrawCallCount = 0;
    Uint32                                m_LastShadowDrawCallCount = 0;
    float                                 m_TerrainExtent = 20.0f;
};

} // namespace Diligent
