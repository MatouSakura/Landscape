#pragma once

#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

struct IDeviceContext;
struct IRenderDevice;
struct ISwapChain;

class ForwardDebugPipeline final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain);
    void Render(IDeviceContext* pContext);

private:
    RefCntAutoPtr<IPipelineState> m_pTrianglePSO;
};

} // namespace Diligent
