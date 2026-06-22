#pragma once

#include "BasicMath.hpp"
#include "Buffer.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

struct IDeviceContext;
struct IRenderDevice;
struct RenderView;

struct CameraConstants final
{
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProj;
    float4   CameraPositionAndNear;
    float4   ViewportSizeAndFar;
};

class FrameResources final
{
public:
    void Initialize(IRenderDevice* pDevice);
    void UpdateCameraConstants(IDeviceContext* pContext, const RenderView& View);

    IBuffer* GetCameraConstantsBuffer() const { return m_pCameraConstantsCB.RawPtr(); }

private:
    RefCntAutoPtr<IBuffer> m_pCameraConstantsCB;
};

} // namespace Diligent
