#include "ShadowRenderer.hpp"

#include "Buffer.h"
#include "DeviceContext.h"
#include "FrameResources.hpp"
#include "MapHelper.hpp"
#include "RenderDevice.h"
#include "RenderQueue.hpp"
#include "TerrainPatchRenderer.hpp"
#include "Texture.h"

namespace Diligent
{

namespace
{

struct ShadowPassConstants
{
    float4x4 ShadowViewProj;
};

float4x4 BuildLightView(const float3& Direction)
{
    const float3 Eye     = -Direction * 45.0f;
    const float3 Ahead   = normalize(Direction);
    const float3 UpSeed  = float3{0.0f, 1.0f, 0.0f};
    const float3 Right   = normalize(cross(UpSeed, Ahead));
    const float3 Up      = cross(Ahead, Right);

    return float4x4{
        Right.x, Up.x, Ahead.x, 0.0f,
        Right.y, Up.y, Ahead.y, 0.0f,
        Right.z, Up.z, Ahead.z, 0.0f,
        -dot(Eye, Right), -dot(Eye, Up), -dot(Eye, Ahead), 1.0f};
}

} // namespace

void ShadowRenderer::Initialize(IRenderDevice* pDevice, bool UseGLProjection)
{
    BuildCascadeMatrices(UseGLProjection);

    for (Uint32 Cascade = 0; Cascade < CascadeCount; ++Cascade)
    {
        TextureDesc Desc;
        Desc.Name      = "LandscapeEditor CSM shadow cascade";
        Desc.Type      = RESOURCE_DIM_TEX_2D;
        Desc.Width     = m_ShadowMapSize;
        Desc.Height    = m_ShadowMapSize;
        Desc.Format    = TEX_FORMAT_D16_UNORM;
        Desc.BindFlags = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;

        pDevice->CreateTexture(Desc, nullptr, &m_Cascades[Cascade].Texture);
        m_Cascades[Cascade].DSV = m_Cascades[Cascade].Texture->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        m_Cascades[Cascade].SRV = m_Cascades[Cascade].Texture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }

    BufferDesc CBDesc;
    CBDesc.Name           = "LandscapeEditor shadow pass constants";
    CBDesc.Size           = sizeof(ShadowPassConstants);
    CBDesc.Usage          = USAGE_DYNAMIC;
    CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(CBDesc, nullptr, &m_pShadowConstantsCB);
}

void ShadowRenderer::BuildCascadeMatrices(bool UseGLProjection)
{
    const float4x4 LightView = BuildLightView(m_SunDirection);

    const float CascadeWidths[CascadeCount] = {24.0f, 36.0f, 52.0f, 72.0f};
    for (Uint32 Cascade = 0; Cascade < CascadeCount; ++Cascade)
    {
        const float4x4 LightProj = float4x4::Ortho(CascadeWidths[Cascade], CascadeWidths[Cascade], 0.1f, 120.0f, UseGLProjection);
        m_Cascades[Cascade].ViewProj = LightView * LightProj;
    }
}

void ShadowRenderer::FillLightConstants(LightConstants& Constants) const
{
    Constants.SunDirection = float4{m_SunDirection.x, m_SunDirection.y, m_SunDirection.z, 0.0f};
    Constants.SunColor     = float4{1.0f, 0.95f, 0.82f, 1.0f};
    Constants.AmbientColor = float4{0.18f, 0.22f, 0.24f, 1.0f};
    Constants.CascadeSplits = float4{20.0f, 45.0f, 90.0f, 160.0f};
    Constants.ShadowParams  = float4{static_cast<float>(CascadeCount), static_cast<float>(m_ShadowMapSize), 0.0015f, 0.0f};

    for (Uint32 Cascade = 0; Cascade < CascadeCount; ++Cascade)
        Constants.ShadowViewProj[Cascade] = m_Cascades[Cascade].ViewProj;
}

void ShadowRenderer::Render(IDeviceContext* pContext, TerrainPatchRenderer& TerrainPatchRenderer, const std::vector<RenderItem>& TerrainItems)
{
    for (Uint32 Cascade = 0; Cascade < CascadeCount; ++Cascade)
    {
        {
            MapHelper<ShadowPassConstants> Constants{pContext, m_pShadowConstantsCB, MAP_WRITE, MAP_FLAG_DISCARD};
            Constants->ShadowViewProj = m_Cascades[Cascade].ViewProj;
        }

        ITextureView* pDSV = m_Cascades[Cascade].DSV;
        pContext->SetRenderTargets(0, nullptr, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        for (const RenderItem& Item : TerrainItems)
        {
            if (Item.Kind == RenderItemKind::TerrainLeaf)
                TerrainPatchRenderer.RenderShadow(pContext, m_pShadowConstantsCB, Item.TerrainRegion);
        }
    }
}

} // namespace Diligent
