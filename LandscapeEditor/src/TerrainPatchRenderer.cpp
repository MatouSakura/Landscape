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
    float2 UV;
};

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
    float2 UV     : ATTRIB2;
};

struct PSInput
{
    float4 Pos      : SV_POSITION;
    float3 Normal   : NORMAL;
    float3 WorldPos : TEXCOORD0;
    float2 UV       : TEXCOORD1;
};

void main(in VSInput VSIn, out PSInput PSIn)
{
    PSIn.Pos      = mul(float4(VSIn.Pos, 1.0), ViewProj);
    PSIn.Normal   = normalize(VSIn.Normal);
    PSIn.WorldPos = VSIn.Pos;
    PSIn.UV       = VSIn.UV;
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
    float2 UV       : TEXCOORD1;
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

    float heightBlend = saturate((PSIn.WorldPos.y + 1.8) / 3.6);
    float slopeBlend = 1.0 - saturate(normal.y);
    float checker = fmod(floor(PSIn.UV.x * 16.0) + floor(PSIn.UV.y * 16.0), 2.0);
    float contour = smoothstep(0.47, 0.53, frac(PSIn.WorldPos.y * 1.20));

    float3 lowColor = float3(0.09, 0.24, 0.15);
    float3 highColor = float3(0.42, 0.38, 0.24);
    float3 slopeColor = float3(0.20, 0.19, 0.17);
    float3 baseColor = lerp(lowColor, highColor, heightBlend);
    baseColor = lerp(baseColor, slopeColor, slopeBlend * 0.45);
    baseColor *= lerp(0.94, 1.06, checker);
    baseColor = lerp(baseColor, float3(0.82, 0.78, 0.60), contour * 0.06);

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
    float2 UV     : ATTRIB2;
};

void main(in VSInput VSIn, out float4 Pos : SV_POSITION)
{
    Pos = mul(float4(VSIn.Pos, 1.0), ShadowViewProj);
}
)";

static const char* TerrainVS_GL = R"(
layout(location = 0) in vec3 VSIn_Pos;
layout(location = 1) in vec3 VSIn_Normal;
layout(location = 2) in vec2 VSIn_UV;

uniform CameraConstants
{
    mat4 View;
    mat4 Projection;
    mat4 ViewProj;
    vec4 CameraPositionAndNear;
    vec4 ViewportSizeAndFar;
};

out vec3 VSOut_Normal;
out vec3 VSOut_WorldPos;
out vec2 VSOut_UV;

#ifndef GL_ES
out gl_PerVertex
{
    vec4 gl_Position;
};
#endif

void main()
{
    gl_Position    = vec4(VSIn_Pos, 1.0) * ViewProj;
    VSOut_Normal   = normalize(VSIn_Normal);
    VSOut_WorldPos = VSIn_Pos;
    VSOut_UV       = VSIn_UV;
}
)";

static const char* TerrainPS_GL = R"(
uniform LightConstants
{
    vec4 SunDirection;
    vec4 SunColor;
    vec4 AmbientColor;
    mat4 ShadowViewProj[4];
    vec4 CascadeSplits;
    vec4 ShadowParams;
};

uniform sampler2D g_ShadowMap0;

in vec3 VSOut_Normal;
in vec3 VSOut_WorldPos;
in vec2 VSOut_UV;

layout(location = 0) out vec4 PSOut_Color;

float saturate(float value)
{
    return clamp(value, 0.0, 1.0);
}

void main()
{
    vec3 normal = normalize(VSOut_Normal);
    float ndotl = saturate(dot(normal, -SunDirection.xyz));

    vec4 shadowClip = vec4(VSOut_WorldPos, 1.0) * ShadowViewProj[0];
    vec3 shadowNDC = shadowClip.xyz / shadowClip.w;
    vec2 shadowUV = shadowNDC.xy * vec2(0.5, -0.5) + vec2(0.5);
    float shadowDepth = texture(g_ShadowMap0, shadowUV).r;
    float receiverDepth = shadowNDC.z - ShadowParams.z;
    float insideShadowMap = step(0.0, shadowUV.x) * step(0.0, shadowUV.y) * step(shadowUV.x, 1.0) * step(shadowUV.y, 1.0);
    float shadowTerm = mix(1.0, receiverDepth > shadowDepth ? 0.62 : 1.0, insideShadowMap);

    float heightBlend = saturate((VSOut_WorldPos.y + 1.8) / 3.6);
    float slopeBlend = 1.0 - saturate(normal.y);
    float checker = mod(floor(VSOut_UV.x * 16.0) + floor(VSOut_UV.y * 16.0), 2.0);
    float contour = smoothstep(0.47, 0.53, fract(VSOut_WorldPos.y * 1.20));

    vec3 lowColor = vec3(0.09, 0.24, 0.15);
    vec3 highColor = vec3(0.42, 0.38, 0.24);
    vec3 slopeColor = vec3(0.20, 0.19, 0.17);
    vec3 baseColor = mix(lowColor, highColor, heightBlend);
    baseColor = mix(baseColor, slopeColor, slopeBlend * 0.45);
    baseColor *= mix(0.94, 1.06, checker);
    baseColor = mix(baseColor, vec3(0.82, 0.78, 0.60), contour * 0.06);

    vec3 lighting = AmbientColor.rgb + SunColor.rgb * ndotl * shadowTerm;
    PSOut_Color = vec4(baseColor * lighting, 1.0);
}
)";

static const char* TerrainShadowVS_GL = R"(
layout(location = 0) in vec3 VSIn_Pos;
layout(location = 1) in vec3 VSIn_Normal;
layout(location = 2) in vec2 VSIn_UV;

uniform ShadowPassConstants
{
    mat4 ShadowViewProj;
};

#ifndef GL_ES
out gl_PerVertex
{
    vec4 gl_Position;
};
#endif

void main()
{
    gl_Position = vec4(VSIn_Pos, 1.0) * ShadowViewProj;
}
)";

static RefCntAutoPtr<IPipelineState> CreateTerrainPipelineState(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    const bool IsGL = pDevice->GetDeviceInfo().IsGLDevice();

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
        LayoutElement{1, 0, 3, VT_FLOAT32, False},
        LayoutElement{2, 0, 2, VT_FLOAT32, False}
    };
    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage                  = IsGL ? SHADER_SOURCE_LANGUAGE_GLSL : SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.EntryPoint                      = "main";
    ShaderCI.CompileFlags                    = IsGL ? SHADER_COMPILE_FLAG_NONE : SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.Desc.Name       = "LandscapeEditor terrain patch VS";
        ShaderCI.Source          = IsGL ? TerrainVS_GL : TerrainVS;
        pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "LandscapeEditor terrain patch PS";
        ShaderCI.Source          = IsGL ? TerrainPS_GL : TerrainPS;
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
    const bool IsGL = pDevice->GetDeviceInfo().IsGLDevice();

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
        LayoutElement{1, 0, 3, VT_FLOAT32, False},
        LayoutElement{2, 0, 2, VT_FLOAT32, False}
    };
    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage                  = IsGL ? SHADER_SOURCE_LANGUAGE_GLSL : SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.EntryPoint                      = "main";
    ShaderCI.CompileFlags                    = IsGL ? SHADER_COMPILE_FLAG_NONE : SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;
    ShaderCI.Desc.ShaderType                 = SHADER_TYPE_VERTEX;
    ShaderCI.Desc.Name                       = "LandscapeEditor terrain shadow VS";
    ShaderCI.Source                          = IsGL ? TerrainShadowVS_GL : TerrainShadowVS;

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
    TerrainHeightFieldDesc HeightFieldDesc;
    m_HeightField.GenerateProcedural(HeightFieldDesc);

    const Uint32 CellCount   = m_HeightField.GetCellCount();
    const Uint32 SampleCount = m_HeightField.GetSampleCountPerAxis();

    std::vector<TerrainVertex> Vertices;
    Vertices.reserve(SampleCount * SampleCount);

    for (Uint32 z = 0; z < SampleCount; ++z)
    {
        const float Z = -HeightFieldDesc.Extent + 2.0f * HeightFieldDesc.Extent * static_cast<float>(z) / static_cast<float>(CellCount);
        for (Uint32 x = 0; x < SampleCount; ++x)
        {
            const float X = -HeightFieldDesc.Extent + 2.0f * HeightFieldDesc.Extent * static_cast<float>(x) / static_cast<float>(CellCount);
            Vertices.push_back(TerrainVertex{
                float3{X, m_HeightField.GetHeight(x, z), Z},
                m_HeightField.GetNormal(x, z),
                m_HeightField.GetUV(x, z)});
        }
    }

    std::vector<Uint32> Indices;
    Indices.reserve(CellCount * CellCount * 6u);

    const Uint32 RowStride = SampleCount;
    for (Uint32 z = 0; z < CellCount; ++z)
    {
        for (Uint32 x = 0; x < CellCount; ++x)
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

    m_pTerrainPSO = PSOCache.GetOrCreate("ForwardOpaque.TerrainPatch.Heightfield.v1", [&]() {
        return CreateTerrainPipelineState(pDevice, pSwapChain);
    });
    m_pTerrainPSO->CreateShaderResourceBinding(&m_pTerrainSRB, true);

    m_pShadowPSO = PSOCache.GetOrCreate("Shadow.TerrainPatch.Heightfield.v1", [&]() {
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
