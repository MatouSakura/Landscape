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
class PSOCache;
struct RenderView;

class ForwardDebugPipeline final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache);
    void Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources);

private:
    RefCntAutoPtr<IPipelineState> m_pGridPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pGridSRB;
};

} // namespace Diligent
