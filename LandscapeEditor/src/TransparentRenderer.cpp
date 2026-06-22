#include "TransparentRenderer.hpp"

#include "DeviceContext.h"
#include "Errors.hpp"
#include "FrameResources.hpp"
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

static const char* TransparentVS = R"(
cbuffer CameraConstants
{
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProj;
    float4   CameraPositionAndNear;
    float4   ViewportSizeAndFar;
};

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float2 UV    : TEXCOORD0;
    float4 Color : COLOR;
};

void main(in uint VertId : SV_VertexID, out PSInput PSIn)
{
    float3 Positions[6];
    Positions[0] = float3(-2.4, 0.10, -3.0);
    Positions[1] = float3( 2.4, 0.10, -3.0);
    Positions[2] = float3(-2.4, 2.60, -3.0);
    Positions[3] = float3(-2.4, 2.60, -3.0);
    Positions[4] = float3( 2.4, 0.10, -3.0);
    Positions[5] = float3( 2.4, 2.60, -3.0);

    float2 UVs[6];
    UVs[0] = float2(0.0, 1.0);
    UVs[1] = float2(1.0, 1.0);
    UVs[2] = float2(0.0, 0.0);
    UVs[3] = float2(0.0, 0.0);
    UVs[4] = float2(1.0, 1.0);
    UVs[5] = float2(1.0, 0.0);

    float2 uv = UVs[VertId];
    float edge = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));
    float border = 1.0 - smoothstep(0.0, 0.10, edge);

    PSIn.Pos   = mul(float4(Positions[VertId], 1.0), ViewProj);
    PSIn.UV    = uv;
    PSIn.Color = lerp(float4(0.10, 0.56, 0.82, 0.28), float4(0.85, 0.95, 1.0, 0.42), border);
}
)";

static const char* TransparentPS = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float2 UV    : TEXCOORD0;
    float4 Color : COLOR;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut)
{
    float stripe = step(0.92, frac((PSIn.UV.x + PSIn.UV.y) * 8.0));
    float alpha = lerp(PSIn.Color.a, 0.50, stripe);
    PSOut.Color = float4(PSIn.Color.rgb, alpha);
}
)";

RefCntAutoPtr<IPipelineState> CreateTransparentPipelineState(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "LandscapeEditor transparent quad PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets                  = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                     = pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                         = pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology                 = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode           = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable      = True;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = False;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthFunc        = COMPARISON_FUNC_LESS_EQUAL;

    auto& RT0 = PSOCreateInfo.GraphicsPipeline.BlendDesc.RenderTargets[0];
    RT0.BlendEnable    = True;
    RT0.SrcBlend       = BLEND_FACTOR_SRC_ALPHA;
    RT0.DestBlend      = BLEND_FACTOR_INV_SRC_ALPHA;
    RT0.SrcBlendAlpha  = BLEND_FACTOR_ZERO;
    RT0.DestBlendAlpha = BLEND_FACTOR_ONE;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage                  = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.EntryPoint                      = "main";
    ShaderCI.CompileFlags                    = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.Desc.Name       = "LandscapeEditor transparent quad VS";
        ShaderCI.Source          = TransparentVS;
        pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "LandscapeEditor transparent quad PS";
        ShaderCI.Source          = TransparentPS;
        pDevice->CreateShader(ShaderCI, &pPS);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    RefCntAutoPtr<IPipelineState> pTransparentPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pTransparentPSO);
    return pTransparentPSO;
}

} // namespace

void TransparentRenderer::Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache)
{
    m_pTransparentPSO = PSOCache.GetOrCreate("ForwardTransparent.TestQuad.AlphaBlend", [&]() {
        return CreateTransparentPipelineState(pDevice, pSwapChain);
    });
    m_pTransparentPSO->CreateShaderResourceBinding(&m_pTransparentSRB, true);
}

void TransparentRenderer::Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources)
{
    (void)View;

    auto* pCameraConstants = m_pTransparentSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants");
    VERIFY_EXPR(pCameraConstants != nullptr);
    pCameraConstants->Set(FrameResources.GetCameraConstantsBuffer());

    pContext->SetPipelineState(m_pTransparentPSO);
    pContext->CommitShaderResources(m_pTransparentSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 6;
    DrawAttrs.Flags       = DRAW_FLAG_VERIFY_ALL;
    pContext->Draw(DrawAttrs);

    ++m_PassCount;
}

} // namespace Diligent
