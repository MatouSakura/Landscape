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

struct LightConstants final
{
    float4   SunDirection;
    float4   SunColor;
    float4   AmbientColor;
    float4x4 ShadowViewProj[4];
    float4   CascadeSplits;
    float4   ShadowParams;
};

class FrameResources final
{
public:
    void Initialize(IRenderDevice* pDevice);
    void UpdateCameraConstants(IDeviceContext* pContext, const RenderView& View);
    void UpdateLightConstants(IDeviceContext* pContext, const LightConstants& Constants);

    IBuffer* GetCameraConstantsBuffer() const { return m_pCameraConstantsCB.RawPtr(); }
    IBuffer* GetLightConstantsBuffer() const { return m_pLightConstantsCB.RawPtr(); }

private:
    RefCntAutoPtr<IBuffer> m_pCameraConstantsCB;
    RefCntAutoPtr<IBuffer> m_pLightConstantsCB;
};

} // namespace Diligent
