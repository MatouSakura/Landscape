#include "SkyRenderer.hpp"

#include "DeviceContext.h"
#include "PSOCache.hpp"
#include "RenderDevice.h"
#include "RenderView.hpp"
#include "Shader.h"
#include "ShaderResourceBinding.h"
#include "SwapChain.h"

namespace Diligent
{

namespace
{

static const char* SkyVS = R"(
struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 UV  : TEXCOORD0;
};

void main(in uint VertId : SV_VertexID, out PSInput PSIn)
{
    float2 Pos[3];
    Pos[0] = float2(-1.0, -1.0);
    Pos[1] = float2(-1.0,  3.0);
    Pos[2] = float2( 3.0, -1.0);

    PSIn.Pos = float4(Pos[VertId], 1.0, 1.0);
    PSIn.UV  = Pos[VertId] * 0.5 + 0.5;
}
)";

static const char* SkyPS = R"(
struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 UV  : TEXCOORD0;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut)
{
    float horizon = saturate(1.0 - PSIn.UV.y);
    float zenith  = saturate(PSIn.UV.y);
    float3 skyTop = float3(0.18, 0.34, 0.62);
    float3 skyLow = float3(0.48, 0.62, 0.74);
    float3 color  = lerp(skyLow, skyTop, pow(zenith, 0.75));
    color += float3(0.08, 0.06, 0.03) * pow(horizon, 4.0);
    PSOut.Color = float4(color, 1.0);
}
)";

RefCntAutoPtr<IPipelineState> CreateSkyPipelineState(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "LandscapeEditor procedural sky PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets                  = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                     = pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                         = pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology                 = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode           = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable      = True;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = False;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthFunc        = COMPARISON_FUNC_LESS_EQUAL;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage                  = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.EntryPoint                      = "main";
    ShaderCI.CompileFlags                    = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.Desc.Name       = "LandscapeEditor procedural sky VS";
        ShaderCI.Source          = SkyVS;
        pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "LandscapeEditor procedural sky PS";
        ShaderCI.Source          = SkyPS;
        pDevice->CreateShader(ShaderCI, &pPS);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    RefCntAutoPtr<IPipelineState> pSkyPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pSkyPSO);
    return pSkyPSO;
}

} // namespace

void SkyRenderer::Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache)
{
    m_pSkyPSO = PSOCache.GetOrCreate("ForwardSky.Procedural.Fullscreen", [&]() {
        return CreateSkyPipelineState(pDevice, pSwapChain);
    });
    m_pSkyPSO->CreateShaderResourceBinding(&m_pSkySRB, true);
}

void SkyRenderer::Render(IDeviceContext* pContext, const RenderView& View)
{
    (void)View;

    pContext->SetPipelineState(m_pSkyPSO);
    pContext->CommitShaderResources(m_pSkySRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 3;
    DrawAttrs.Flags       = DRAW_FLAG_VERIFY_ALL;
    pContext->Draw(DrawAttrs);

    ++m_PassCount;
}

} // namespace Diligent
