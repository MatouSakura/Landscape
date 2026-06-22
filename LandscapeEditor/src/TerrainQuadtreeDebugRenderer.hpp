#pragma once

#include "Buffer.h"
#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"

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

class TerrainQuadtreeDebugRenderer final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache, Uint32 MaxLineVertexCount);
    void Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources, const TerrainQuadtree& Quadtree, const TerrainQuadtreeSelection& Selection);

    Uint32 GetLastLineVertexCount() const { return m_LastLineVertexCount; }

private:
    RefCntAutoPtr<IBuffer>                m_pVertexBuffer;
    RefCntAutoPtr<IPipelineState>         m_pPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
    Uint32                                m_MaxLineVertexCount  = 0;
    Uint32                                m_LastLineVertexCount = 0;
};

} // namespace Diligent
