#pragma once

#include "BasicMath.hpp"
#include "RefCntAutoPtr.hpp"
#include "TextureView.h"

namespace Diligent
{

struct IBuffer;
struct IDeviceContext;
struct IRenderDevice;
struct ITexture;
class TerrainPatchRenderer;
struct LightConstants;

class ShadowRenderer final
{
public:
    static constexpr Uint32 CascadeCount = 4;

    void Initialize(IRenderDevice* pDevice, bool UseGLProjection);
    void FillLightConstants(LightConstants& Constants) const;
    void Render(IDeviceContext* pContext, TerrainPatchRenderer& TerrainPatchRenderer);

    ITextureView* GetCascadeSRV(Uint32 Cascade) const { return m_Cascades[Cascade].SRV.RawPtr(); }
    Uint32 GetShadowMapSize() const { return m_ShadowMapSize; }

private:
    struct CascadeResources
    {
        RefCntAutoPtr<ITexture>     Texture;
        RefCntAutoPtr<ITextureView> DSV;
        RefCntAutoPtr<ITextureView> SRV;
        float4x4                    ViewProj = float4x4::Identity();
    };

    void BuildCascadeMatrices(bool UseGLProjection);

private:
    CascadeResources      m_Cascades[CascadeCount];
    RefCntAutoPtr<IBuffer> m_pShadowConstantsCB;
    Uint32                m_ShadowMapSize = 1024;
    float3                m_SunDirection  = normalize(float3{0.35f, -1.0f, 0.25f});
};

} // namespace Diligent
