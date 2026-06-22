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

static const char* SkyVS_GL = R"(
out vec2 VSOut_UV;

#ifndef GL_ES
out gl_PerVertex
{
    vec4 gl_Position;
};
#endif

void main()
{
    vec2 Pos[3];
    Pos[0] = vec2(-1.0, -1.0);
    Pos[1] = vec2(-1.0,  3.0);
    Pos[2] = vec2( 3.0, -1.0);

    vec2 pos = Pos[gl_VertexID % 3];
    VSOut_UV = pos * 0.5 + vec2(0.5);
    gl_Position = vec4(pos, 1.0, 1.0);
}
)";

static const char* SkyPS_GL = R"(
in vec2 VSOut_UV;

layout(location = 0) out vec4 PSOut_Color;

void main()
{
    float horizon = clamp(1.0 - VSOut_UV.y, 0.0, 1.0);
    float zenith  = clamp(VSOut_UV.y, 0.0, 1.0);
    vec3 skyTop = vec3(0.18, 0.34, 0.62);
    vec3 skyLow = vec3(0.48, 0.62, 0.74);
    vec3 color  = mix(skyLow, skyTop, pow(zenith, 0.75));
    color += vec3(0.08, 0.06, 0.03) * pow(horizon, 4.0);
    PSOut_Color = vec4(color, 1.0);
}
)";

RefCntAutoPtr<IPipelineState> CreateSkyPipelineState(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    const bool IsGL = pDevice->GetDeviceInfo().IsGLDevice();

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
    ShaderCI.SourceLanguage                  = IsGL ? SHADER_SOURCE_LANGUAGE_GLSL : SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.EntryPoint                      = "main";
    ShaderCI.CompileFlags                    = IsGL ? SHADER_COMPILE_FLAG_NONE : SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.Desc.Name       = "LandscapeEditor procedural sky VS";
        ShaderCI.Source          = IsGL ? SkyVS_GL : SkyVS;
        pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "LandscapeEditor procedural sky PS";
        ShaderCI.Source          = IsGL ? SkyPS_GL : SkyPS;
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
