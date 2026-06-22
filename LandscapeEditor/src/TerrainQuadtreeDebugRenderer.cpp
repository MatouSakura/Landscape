#include "TerrainQuadtreeDebugRenderer.hpp"

#include "DeviceContext.h"
#include "Errors.hpp"
#include "FrameResources.hpp"
#include "MapHelper.hpp"
#include "PSOCache.hpp"
#include "RenderDevice.h"
#include "RenderView.hpp"
#include "Shader.h"
#include "ShaderResourceBinding.h"
#include "SwapChain.h"
#include "TerrainQuadtree.hpp"

#include <algorithm>
#include <vector>

namespace Diligent
{

namespace
{

struct TerrainQuadtreeDebugVertex
{
    float3 Pos;
    float3 Color;
};

static constexpr float OverlayY = 0.08f;

static const char* QuadtreeDebugVS = R"(
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
    float3 Pos   : ATTRIB0;
    float3 Color : ATTRIB1;
};

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in VSInput VSIn, out PSInput PSIn)
{
    PSIn.Pos   = mul(float4(VSIn.Pos, 1.0), ViewProj);
    PSIn.Color = VSIn.Color;
}
)";

static const char* QuadtreeDebugPS = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut)
{
    PSOut.Color = float4(PSIn.Color, 1.0);
}
)";

static const char* QuadtreeDebugVS_GL = R"(
layout(location = 0) in vec3 VSIn_Pos;
layout(location = 1) in vec3 VSIn_Color;

uniform CameraConstants
{
    mat4 View;
    mat4 Projection;
    mat4 ViewProj;
    vec4 CameraPositionAndNear;
    vec4 ViewportSizeAndFar;
};

out vec3 VSOut_Color;

#ifndef GL_ES
out gl_PerVertex
{
    vec4 gl_Position;
};
#endif

void main()
{
    gl_Position = vec4(VSIn_Pos, 1.0) * ViewProj;
    VSOut_Color = VSIn_Color;
}
)";

static const char* QuadtreeDebugPS_GL = R"(
in vec3 VSOut_Color;

layout(location = 0) out vec4 PSOut_Color;

void main()
{
    PSOut_Color = vec4(VSOut_Color, 1.0);
}
)";

float3 GetLodColor(Uint32 Level)
{
    static constexpr float3 Colors[] =
    {
        float3{3.00f, 2.40f, 0.10f},
        float3{0.10f, 3.00f, 0.35f},
        float3{0.10f, 1.30f, 3.20f},
        float3{3.00f, 0.75f, 0.10f},
        float3{2.80f, 0.15f, 2.80f},
    };
    return Colors[std::min<Uint32>(Level, _countof(Colors) - 1u)];
}

void AppendLeafBounds(std::vector<TerrainQuadtreeDebugVertex>& Vertices, const TerrainQuadtreeNode& Node)
{
    const float3 Color = GetLodColor(Node.Level);

    const float3 SW{Node.MinXZ.x, OverlayY, Node.MinXZ.y};
    const float3 SE{Node.MaxXZ.x, OverlayY, Node.MinXZ.y};
    const float3 NW{Node.MinXZ.x, OverlayY, Node.MaxXZ.y};
    const float3 NE{Node.MaxXZ.x, OverlayY, Node.MaxXZ.y};

    Vertices.push_back(TerrainQuadtreeDebugVertex{SW, Color});
    Vertices.push_back(TerrainQuadtreeDebugVertex{SE, Color});
    Vertices.push_back(TerrainQuadtreeDebugVertex{SE, Color});
    Vertices.push_back(TerrainQuadtreeDebugVertex{NE, Color});
    Vertices.push_back(TerrainQuadtreeDebugVertex{NE, Color});
    Vertices.push_back(TerrainQuadtreeDebugVertex{NW, Color});
    Vertices.push_back(TerrainQuadtreeDebugVertex{NW, Color});
    Vertices.push_back(TerrainQuadtreeDebugVertex{SW, Color});
}

RefCntAutoPtr<IPipelineState> CreateQuadtreeDebugPSO(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    const bool IsGL = pDevice->GetDeviceInfo().IsGLDevice();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "LandscapeEditor quadtree debug overlay PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets                  = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                     = pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                         = pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology                 = PRIMITIVE_TOPOLOGY_LINE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode           = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable      = False;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = False;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthFunc        = COMPARISON_FUNC_LESS_EQUAL;

    LayoutElement LayoutElems[] =
    {
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        LayoutElement{1, 0, 3, VT_FLOAT32, False}
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
        ShaderCI.Desc.Name       = "LandscapeEditor quadtree debug VS";
        ShaderCI.Source          = IsGL ? QuadtreeDebugVS_GL : QuadtreeDebugVS;
        pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "LandscapeEditor quadtree debug PS";
        ShaderCI.Source          = IsGL ? QuadtreeDebugPS_GL : QuadtreeDebugPS;
        pDevice->CreateShader(ShaderCI, &pPS);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    return pPSO;
}

} // namespace

void TerrainQuadtreeDebugRenderer::Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache, Uint32 MaxLineVertexCount)
{
    m_MaxLineVertexCount = MaxLineVertexCount;

    BufferDesc VBDesc;
    VBDesc.Name           = "LandscapeEditor quadtree debug vertex buffer";
    VBDesc.Usage          = USAGE_DYNAMIC;
    VBDesc.BindFlags      = BIND_VERTEX_BUFFER;
    VBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    VBDesc.Size           = static_cast<Uint64>(m_MaxLineVertexCount * sizeof(TerrainQuadtreeDebugVertex));
    pDevice->CreateBuffer(VBDesc, nullptr, &m_pVertexBuffer);

    m_pPSO = PSOCache.GetOrCreate("ForwardDebug.TerrainQuadtree.LineList.v1", [&]() {
        return CreateQuadtreeDebugPSO(pDevice, pSwapChain);
    });
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);
}

void TerrainQuadtreeDebugRenderer::Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources, const TerrainQuadtree& Quadtree, const TerrainQuadtreeSelection& Selection)
{
    (void)View;

    const auto& Nodes = Quadtree.GetNodes();
    if (Selection.SelectedNodeIndices.empty() || m_MaxLineVertexCount == 0)
    {
        m_LastLineVertexCount = 0;
        return;
    }

    std::vector<TerrainQuadtreeDebugVertex> Vertices;
    Vertices.reserve(std::min<Uint32>(m_MaxLineVertexCount, static_cast<Uint32>(Selection.SelectedNodeIndices.size() * 8u)));

    for (Uint32 NodeIndex : Selection.SelectedNodeIndices)
    {
        if (NodeIndex >= Nodes.size() || Vertices.size() + 8u > m_MaxLineVertexCount)
            break;
        AppendLeafBounds(Vertices, Nodes[NodeIndex]);
    }

    m_LastLineVertexCount = static_cast<Uint32>(Vertices.size());
    if (m_LastLineVertexCount == 0)
        return;

    {
        MapHelper<TerrainQuadtreeDebugVertex> MappedVertices{pContext, m_pVertexBuffer, MAP_WRITE, MAP_FLAG_DISCARD};
        TerrainQuadtreeDebugVertex* pMappedVertices = MappedVertices;
        std::copy(Vertices.begin(), Vertices.end(), pMappedVertices);
    }

    auto* pCameraConstants = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants");
    VERIFY_EXPR(pCameraConstants != nullptr);
    pCameraConstants->Set(FrameResources.GetCameraConstantsBuffer());

    const Uint64 Offset = 0;
    IBuffer*     VBs[]  = {m_pVertexBuffer};
    pContext->SetVertexBuffers(0, 1, VBs, &Offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    pContext->SetPipelineState(m_pPSO);
    pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = m_LastLineVertexCount;
    DrawAttrs.Flags       = DRAW_FLAG_VERIFY_ALL;
    pContext->Draw(DrawAttrs);
}

} // namespace Diligent
