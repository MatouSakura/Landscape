#pragma once

#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

struct IDeviceContext;
struct IRenderDevice;
struct IShaderResourceBinding;
struct ISwapChain;
class PSOCache;
struct RenderView;

class SkyRenderer final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache);
    void Render(IDeviceContext* pContext, const RenderView& View);

    Uint32 GetPassCount() const { return m_PassCount; }

private:
    RefCntAutoPtr<IPipelineState>         m_pSkyPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pSkySRB;
    Uint32                                m_PassCount = 0;
};

} // namespace Diligent
