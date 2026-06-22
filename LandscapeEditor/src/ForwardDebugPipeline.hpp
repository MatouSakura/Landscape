#pragma once

#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

struct IDeviceContext;
struct IRenderDevice;
struct IShaderResourceBinding;
struct ISwapChain;
class FrameResources;
struct RenderView;

class ForwardDebugPipeline final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain);
    void Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources);

private:
    RefCntAutoPtr<IPipelineState> m_pGridPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pGridSRB;
};

} // namespace Diligent
