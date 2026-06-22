#include "FrameResources.hpp"

#include "DeviceContext.h"
#include "MapHelper.hpp"
#include "RenderDevice.h"
#include "RenderView.hpp"

namespace Diligent
{

void FrameResources::Initialize(IRenderDevice* pDevice)
{
    BufferDesc CBDesc;
    CBDesc.Name           = "LandscapeEditor camera constants";
    CBDesc.Size           = sizeof(CameraConstants);
    CBDesc.Usage          = USAGE_DYNAMIC;
    CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(CBDesc, nullptr, &m_pCameraConstantsCB);
}

void FrameResources::UpdateCameraConstants(IDeviceContext* pContext, const RenderView& View)
{
    MapHelper<CameraConstants> Constants{pContext, m_pCameraConstantsCB, MAP_WRITE, MAP_FLAG_DISCARD};
    Constants->View                  = View.View;
    Constants->Projection            = View.Projection;
    Constants->ViewProj              = View.ViewProj;
    Constants->CameraPositionAndNear = float4{View.CameraPosition.x, View.CameraPosition.y, View.CameraPosition.z, View.NearPlane};
    Constants->ViewportSizeAndFar    = float4{View.ViewportSize.x, View.ViewportSize.y, View.FarPlane, 0.0f};
}

} // namespace Diligent
