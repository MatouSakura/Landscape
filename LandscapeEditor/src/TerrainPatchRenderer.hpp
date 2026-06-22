#pragma once

#include "Buffer.h"
#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"
#include "TerrainHeightField.hpp"

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
    void Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources, ITextureView* pShadowMapSRV);
    void RenderShadow(IDeviceContext* pContext, IBuffer* pShadowConstants);

    Uint32 GetTriangleCount() const { return m_IndexCount / 3u; }
    Uint32 GetCellCount() const { return m_HeightField.GetCellCount(); }
    Uint32 GetSampleCountPerAxis() const { return m_HeightField.GetSampleCountPerAxis(); }
    float  GetMinHeight() const { return m_HeightField.GetStats().MinHeight; }
    float  GetMaxHeight() const { return m_HeightField.GetStats().MaxHeight; }
    float  GetAverageHeight() const { return m_HeightField.GetStats().AverageHeight; }

private:
    TerrainHeightField                    m_HeightField;
    RefCntAutoPtr<IBuffer>                m_pVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_pIndexBuffer;
    RefCntAutoPtr<IPipelineState>         m_pTerrainPSO;
    RefCntAutoPtr<IPipelineState>         m_pShadowPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pTerrainSRB;
    RefCntAutoPtr<IShaderResourceBinding> m_pShadowSRB;
    Uint32                                m_IndexCount = 0;
};

} // namespace Diligent
