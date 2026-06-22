#include "TerrainPatchRenderer.hpp"

#include "DeviceContext.h"
#include "Errors.hpp"
#include "FrameResources.hpp"
#include "PSOCache.hpp"
#include "RenderDevice.h"
#include "RenderView.hpp"
#include "Shader.h"
#include "ShaderResourceBinding.h"
#include "SwapChain.h"

#include <vector>

namespace Diligent
{

namespace
{

struct TerrainVertex
{
    float3 Pos;
    float3 Normal;
};

static constexpr Uint32 PatchResolution = 64;
static constexpr float  PatchExtent     = 20.0f;
static constexpr float  PatchHeight     = -0.025f;

static const char* TerrainVS = R"(
cbuffer CameraConstants
{
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProj;
    float4   CameraPositionAndNear;
    float4   ViewportSizeAndFar;
};

struct VSInput
{
    float3 Pos    : ATTRIB0;
    float3 Normal : ATTRIB1;
};

struct PSInput
{
    float4 Pos      : SV_POSITION;
    float3 Normal   : NORMAL;
    float3 WorldPos : TEXCOORD0;
};

void main(in VSInput VSIn, out PSInput PSIn)
{
    PSIn.Pos      = mul(float4(VSIn.Pos, 1.0), ViewProj);
    PSIn.Normal   = normalize(VSIn.Normal);
    PSIn.WorldPos = VSIn.Pos;
}
)";

static const char* TerrainPS = R"(
cbuffer LightConstants
{
    float4   SunDirection;
    float4   SunColor;
    float4   AmbientColor;
    float4x4 ShadowViewProj[4];
    float4   CascadeSplits;
    float4   ShadowParams;
};

Texture2D<float> g_ShadowMap0;
SamplerState     g_ShadowMap0_sampler;

struct PSInput
{
    float4 Pos      : SV_POSITION;
    float3 Normal   : NORMAL;
    float3 WorldPos : TEXCOORD0;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut)
{
    float3 normal = normalize(PSIn.Normal);
    float ndotl = saturate(dot(normal, -SunDirection.xyz));

    float4 shadowClip = mul(float4(PSIn.WorldPos, 1.0), ShadowViewProj[0]);
    float3 shadowNDC = shadowClip.xyz / shadowClip.w;
    float2 shadowUV = shadowNDC.xy * float2(0.5, -0.5) + 0.5;
    float shadowDepth = g_ShadowMap0.SampleLevel(g_ShadowMap0_sampler, shadowUV, 0.0).r;
    float receiverDepth = shadowNDC.z - ShadowParams.z;
    float insideShadowMap = step(0.0, shadowUV.x) * step(0.0, shadowUV.y) * step(shadowUV.x, 1.0) * step(shadowUV.y, 1.0);
    float shadowTerm = lerp(1.0, receiverDepth > shadowDepth ? 0.62 : 1.0, insideShadowMap);

    float checker = fmod(floor(PSIn.WorldPos.x * 0.5) + floor(PSIn.WorldPos.z * 0.5), 2.0);
    float3 baseColor = lerp(float3(0.10, 0.27, 0.18), float3(0.13, 0.34, 0.23), checker);
    float3 lighting = AmbientColor.rgb + SunColor.rgb * ndotl * shadowTerm;
    PSOut.Color = float4(baseColor * lighting, 1.0);
}
)";

static const char* TerrainShadowVS = R"(
cbuffer ShadowPassConstants
{
    float4x4 ShadowViewProj;
};

struct VSInput
{
    float3 Pos    : ATTRIB0;
    float3 Normal : ATTRIB1;
};

void main(in VSInput VSIn, out float4 Pos : SV_POSITION)
{
    Pos = mul(float4(VSIn.Pos, 1.0), ShadowViewProj);
}
)";

static RefCntAutoPtr<IPipelineState> CreateTerrainPipelineState(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "LandscapeEditor forward opaque terrain patch PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

    LayoutElement LayoutElems[] =
    {
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        LayoutElement{1, 0, 3, VT_FLOAT32, False}
    };
    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage                  = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.EntryPoint                      = "main";
    ShaderCI.CompileFlags                    = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.Desc.Name       = "LandscapeEditor terrain patch VS";
        ShaderCI.Source          = TerrainVS;
        pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "LandscapeEditor terrain patch PS";
        ShaderCI.Source          = TerrainPS;
        pDevice->CreateShader(ShaderCI, &pPS);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    SamplerDesc ShadowSampler;
    ShadowSampler.MinFilter = FILTER_TYPE_LINEAR;
    ShadowSampler.MagFilter = FILTER_TYPE_LINEAR;
    ShadowSampler.MipFilter = FILTER_TYPE_LINEAR;
    ShadowSampler.AddressU  = TEXTURE_ADDRESS_CLAMP;
    ShadowSampler.AddressV  = TEXTURE_ADDRESS_CLAMP;
    ShadowSampler.AddressW  = TEXTURE_ADDRESS_CLAMP;

    ImmutableSamplerDesc ImmutableSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_ShadowMap0", ShadowSampler}
    };
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImmutableSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImmutableSamplers);

    RefCntAutoPtr<IPipelineState> pTerrainPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pTerrainPSO);
    return pTerrainPSO;
}

static RefCntAutoPtr<IPipelineState> CreateTerrainShadowPipelineState(IRenderDevice* pDevice)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "LandscapeEditor terrain shadow PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 0;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = TEX_FORMAT_D16_UNORM;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

    LayoutElement LayoutElems[] =
    {
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        LayoutElement{1, 0, 3, VT_FLOAT32, False}
    };
    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage                  = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.EntryPoint                      = "main";
    ShaderCI.CompileFlags                    = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;
    ShaderCI.Desc.ShaderType                 = SHADER_TYPE_VERTEX;
    ShaderCI.Desc.Name                       = "LandscapeEditor terrain shadow VS";
    ShaderCI.Source                          = TerrainShadowVS;

    RefCntAutoPtr<IShader> pVS;
    pDevice->CreateShader(ShaderCI, &pVS);

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    RefCntAutoPtr<IPipelineState> pShadowPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pShadowPSO);
    return pShadowPSO;
}

} // namespace

void TerrainPatchRenderer::Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache)
{
    std::vector<TerrainVertex> Vertices;
    Vertices.reserve((PatchResolution + 1u) * (PatchResolution + 1u));

    for (Uint32 z = 0; z <= PatchResolution; ++z)
    {
        const float Z = -PatchExtent + 2.0f * PatchExtent * static_cast<float>(z) / static_cast<float>(PatchResolution);
        for (Uint32 x = 0; x <= PatchResolution; ++x)
        {
            const float X = -PatchExtent + 2.0f * PatchExtent * static_cast<float>(x) / static_cast<float>(PatchResolution);
            Vertices.push_back(TerrainVertex{float3{X, PatchHeight, Z}, float3{0.0f, 1.0f, 0.0f}});
        }
    }

    std::vector<Uint32> Indices;
    Indices.reserve(PatchResolution * PatchResolution * 6u);

    const Uint32 RowStride = PatchResolution + 1u;
    for (Uint32 z = 0; z < PatchResolution; ++z)
    {
        for (Uint32 x = 0; x < PatchResolution; ++x)
        {
            const Uint32 I0 = z * RowStride + x;
            const Uint32 I1 = I0 + 1u;
            const Uint32 I2 = I0 + RowStride;
            const Uint32 I3 = I2 + 1u;

            Indices.push_back(I0);
            Indices.push_back(I2);
            Indices.push_back(I1);
            Indices.push_back(I1);
            Indices.push_back(I2);
            Indices.push_back(I3);
        }
    }
    m_IndexCount = static_cast<Uint32>(Indices.size());

    BufferDesc VBDesc;
    VBDesc.Name      = "LandscapeEditor terrain patch vertex buffer";
    VBDesc.Usage     = USAGE_IMMUTABLE;
    VBDesc.BindFlags = BIND_VERTEX_BUFFER;
    VBDesc.Size      = static_cast<Uint64>(Vertices.size() * sizeof(TerrainVertex));
    BufferData VBData;
    VBData.pData    = Vertices.data();
    VBData.DataSize = VBDesc.Size;
    pDevice->CreateBuffer(VBDesc, &VBData, &m_pVertexBuffer);

    BufferDesc IBDesc;
    IBDesc.Name      = "LandscapeEditor terrain patch index buffer";
    IBDesc.Usage     = USAGE_IMMUTABLE;
    IBDesc.BindFlags = BIND_INDEX_BUFFER;
    IBDesc.Size      = static_cast<Uint64>(Indices.size() * sizeof(Uint32));
    BufferData IBData;
    IBData.pData    = Indices.data();
    IBData.DataSize = IBDesc.Size;
    pDevice->CreateBuffer(IBDesc, &IBData, &m_pIndexBuffer);

    m_pTerrainPSO = PSOCache.GetOrCreate("ForwardOpaque.TerrainPatch.TriangleList", [&]() {
        return CreateTerrainPipelineState(pDevice, pSwapChain);
    });
    m_pTerrainPSO->CreateShaderResourceBinding(&m_pTerrainSRB, true);

    m_pShadowPSO = PSOCache.GetOrCreate("Shadow.TerrainPatch.Depth", [&]() {
        return CreateTerrainShadowPipelineState(pDevice);
    });
    m_pShadowPSO->CreateShaderResourceBinding(&m_pShadowSRB, true);
}

void TerrainPatchRenderer::Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources, ITextureView* pShadowMapSRV)
{
    (void)View;

    auto* pCameraConstants = m_pTerrainSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants");
    VERIFY_EXPR(pCameraConstants != nullptr);
    pCameraConstants->Set(FrameResources.GetCameraConstantsBuffer());

    auto* pLightConstants = m_pTerrainSRB->GetVariableByName(SHADER_TYPE_PIXEL, "LightConstants");
    VERIFY_EXPR(pLightConstants != nullptr);
    pLightConstants->Set(FrameResources.GetLightConstantsBuffer());

    auto* pShadowMap = m_pTerrainSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap0");
    VERIFY_EXPR(pShadowMap != nullptr);
    pShadowMap->Set(pShadowMapSRV);

    const Uint64 Offset = 0;
    IBuffer*     VBs[]  = {m_pVertexBuffer};
    pContext->SetVertexBuffers(0, 1, VBs, &Offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(m_pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(m_pTerrainPSO);
    pContext->CommitShaderResources(m_pTerrainSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs DrawAttrs;
    DrawAttrs.IndexType  = VT_UINT32;
    DrawAttrs.NumIndices = m_IndexCount;
    DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
    pContext->DrawIndexed(DrawAttrs);
}

void TerrainPatchRenderer::RenderShadow(IDeviceContext* pContext, IBuffer* pShadowConstants)
{
    auto* pShadowConstantsVar = m_pShadowSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ShadowPassConstants");
    VERIFY_EXPR(pShadowConstantsVar != nullptr);
    pShadowConstantsVar->Set(pShadowConstants);

    const Uint64 Offset = 0;
    IBuffer*     VBs[]  = {m_pVertexBuffer};
    pContext->SetVertexBuffers(0, 1, VBs, &Offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(m_pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(m_pShadowPSO);
    pContext->CommitShaderResources(m_pShadowSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs DrawAttrs;
    DrawAttrs.IndexType  = VT_UINT32;
    DrawAttrs.NumIndices = m_IndexCount;
    DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
    pContext->DrawIndexed(DrawAttrs);
}

} // namespace Diligent
