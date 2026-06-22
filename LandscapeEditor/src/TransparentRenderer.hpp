#pragma once

#include "BasicMath.hpp"
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
struct RenderView;

class TransparentRenderer final
{
public:
    static float3 GetTestQuadCenter() { return float3{0.0f, 1.35f, -3.0f}; }

    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache);
    void Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources);

    Uint32 GetPassCount() const { return m_PassCount; }

private:
    RefCntAutoPtr<IPipelineState>         m_pTransparentPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pTransparentSRB;
    Uint32                                m_PassCount = 0;
};

} // namespace Diligent
