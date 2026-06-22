#pragma once

#include "BasicMath.hpp"
#include "GraphicsTypes.h"

namespace Diligent
{

struct RenderView final
{
    float4x4 View       = float4x4::Identity();
    float4x4 Projection = float4x4::Identity();
    float4x4 ViewProj   = float4x4::Identity();

    float3 CameraPosition = float3{0.0f, 0.0f, 0.0f};
    float  NearPlane      = 0.1f;
    float  FarPlane       = 1000.0f;

    float2        ViewportSize = float2{1.0f, 1.0f};
    TEXTURE_FORMAT ColorFormat = TEX_FORMAT_UNKNOWN;
    TEXTURE_FORMAT DepthFormat = TEX_FORMAT_UNKNOWN;

    bool IsGL = false;
};

} // namespace Diligent
