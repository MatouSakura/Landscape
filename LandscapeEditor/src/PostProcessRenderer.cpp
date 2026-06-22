#include "PostProcessRenderer.hpp"

#include "DeviceContext.h"
#include "Errors.hpp"
#include "PSOCache.hpp"
#include "RenderDevice.h"
#include "Shader.h"
#include "ShaderResourceBinding.h"
#include "SwapChain.h"
#include "Texture.h"

namespace Diligent
{

namespace
{

namespace HLSL
{

static const char* PostProcessVS = R"(
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

    PSIn.Pos = float4(Pos[VertId], 0.0, 1.0);
    PSIn.UV  = Pos[VertId] * float2(0.5, -0.5) + 0.5;
}
)";

static const char* PostProcessPS = R"(
Texture2D    g_SceneColor;
SamplerState g_SceneColor_sampler;

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
    float3 color = g_SceneColor.SampleLevel(g_SceneColor_sampler, PSIn.UV, 0.0).rgb;
    float3 mapped = 1.0 - exp(-color * 1.05);
    mapped = pow(saturate(mapped), 1.0 / 2.2);
    PSOut.Color = float4(mapped, 1.0);
}
)";

} // namespace HLSL

namespace GLSL
{

static const char* PostProcessVS = R"(
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
    VSOut_UV = pos * vec2(0.5, 0.5) + vec2(0.5, 0.5);
    gl_Position = vec4(pos, -1.0, 1.0);
}
)";

static const char* PostProcessPS = R"(
uniform sampler2D g_SceneColor;

in vec2 VSOut_UV;

layout(location = 0) out vec4 PSOut_Color;

void main()
{
    vec3 color = texture(g_SceneColor, VSOut_UV).rgb;
    vec3 mapped = 1.0 - exp(-color * 1.05);
    mapped = pow(clamp(mapped, vec3(0.0), vec3(1.0)), vec3(1.0 / 2.2));
    PSOut_Color = vec4(mapped, 1.0);
}
)";

} // namespace GLSL

RefCntAutoPtr<IPipelineState> CreatePostProcessPipelineState(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    const bool IsGL = pDevice->GetDeviceInfo().IsGLDevice();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "LandscapeEditor tone map postprocess PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage                  = IsGL ? SHADER_SOURCE_LANGUAGE_GLSL : SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.EntryPoint                      = "main";
    ShaderCI.CompileFlags                    = IsGL ? SHADER_COMPILE_FLAG_NONE : SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.Desc.Name       = "LandscapeEditor postprocess fullscreen VS";
        ShaderCI.Source          = IsGL ? GLSL::PostProcessVS : HLSL::PostProcessVS;
        pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "LandscapeEditor tone map postprocess PS";
        ShaderCI.Source          = IsGL ? GLSL::PostProcessPS : HLSL::PostProcessPS;
        pDevice->CreateShader(ShaderCI, &pPS);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    SamplerDesc SceneSampler;
    SceneSampler.MinFilter = FILTER_TYPE_LINEAR;
    SceneSampler.MagFilter = FILTER_TYPE_LINEAR;
    SceneSampler.MipFilter = FILTER_TYPE_LINEAR;
    SceneSampler.AddressU  = TEXTURE_ADDRESS_CLAMP;
    SceneSampler.AddressV  = TEXTURE_ADDRESS_CLAMP;
    SceneSampler.AddressW  = TEXTURE_ADDRESS_CLAMP;

    ImmutableSamplerDesc ImmutableSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_SceneColor", SceneSampler}
    };
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImmutableSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImmutableSamplers);

    RefCntAutoPtr<IPipelineState> pPostProcessPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPostProcessPSO);
    return pPostProcessPSO;
}

} // namespace

void PostProcessRenderer::Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache)
{
    m_pDevice = pDevice;
    m_pPostProcessPSO = PSOCache.GetOrCreate("PostProcess.ToneMap.Fullscreen", [&]() {
        return CreatePostProcessPipelineState(pDevice, pSwapChain);
    });
    m_pPostProcessPSO->CreateShaderResourceBinding(&m_pPostProcessSRB, true);
    EnsureSceneTarget(pSwapChain);
}

ITextureView* PostProcessRenderer::PrepareSceneTarget(IDeviceContext* pContext, ISwapChain* pSwapChain, const float ClearColor[4])
{
    EnsureSceneTarget(pSwapChain);
    ITextureView* pSceneRTV = m_pSceneColorRTV;
    pContext->SetRenderTargets(1, &pSceneRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(pSceneRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    return pSceneRTV;
}

void PostProcessRenderer::Render(IDeviceContext* pContext, ISwapChain* pSwapChain)
{
    ITextureView* pRTV = pSwapChain->GetCurrentBackBufferRTV();
    pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    auto* pSceneColor = m_pPostProcessSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_SceneColor");
    VERIFY_EXPR(pSceneColor != nullptr);
    pSceneColor->Set(m_pSceneColorSRV);

    pContext->SetPipelineState(m_pPostProcessPSO);
    pContext->CommitShaderResources(m_pPostProcessSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 3;
    DrawAttrs.Flags       = DRAW_FLAG_VERIFY_ALL;
    pContext->Draw(DrawAttrs);

    pContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);

    ++m_PassCount;
}

void PostProcessRenderer::EnsureSceneTarget(ISwapChain* pSwapChain)
{
    VERIFY_EXPR(m_pDevice != nullptr);

    const auto& Desc = pSwapChain->GetDesc();
    if (m_pSceneColor && m_Width == Desc.Width && m_Height == Desc.Height && m_ColorFormat == Desc.ColorBufferFormat)
        return;

    m_pSceneColor.Release();
    m_pSceneColorRTV.Release();
    m_pSceneColorSRV.Release();

    m_Width       = Desc.Width;
    m_Height      = Desc.Height;
    m_ColorFormat = Desc.ColorBufferFormat;

    TextureDesc SceneDesc;
    SceneDesc.Name      = "LandscapeEditor scene color";
    SceneDesc.Type      = RESOURCE_DIM_TEX_2D;
    SceneDesc.Width     = Desc.Width != 0 ? Desc.Width : 1;
    SceneDesc.Height    = Desc.Height != 0 ? Desc.Height : 1;
    SceneDesc.Format    = Desc.ColorBufferFormat;
    SceneDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

    m_pDevice->CreateTexture(SceneDesc, nullptr, &m_pSceneColor);
    m_pSceneColorRTV = m_pSceneColor->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    m_pSceneColorSRV = m_pSceneColor->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
}

} // namespace Diligent
