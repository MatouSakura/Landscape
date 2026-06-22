#include "ForwardDebugPipeline.hpp"

#include "DeviceContext.h"
#include "RenderDevice.h"
#include "Shader.h"
#include "SwapChain.h"

namespace Diligent
{

namespace
{

static const char* ForwardDebugTriangleVS = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn)
{
    float4 Pos[3];
    Pos[0] = float4(-0.55, -0.45, 0.0, 1.0);
    Pos[1] = float4( 0.00, +0.55, 0.0, 1.0);
    Pos[2] = float4(+0.55, -0.45, 0.0, 1.0);

    float3 Col[3];
    Col[0] = float3(0.10, 0.80, 0.35);
    Col[1] = float3(0.95, 0.86, 0.25);
    Col[2] = float3(0.20, 0.55, 1.00);

    PSIn.Pos   = Pos[VertId];
    PSIn.Color = Col[VertId];
}
)";

static const char* ForwardDebugTrianglePS = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    PSOut.Color = float4(PSIn.Color, 1.0);
}
)";

} // namespace

void ForwardDebugPipeline::Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "LandscapeEditor forward debug triangle PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage                  = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.EntryPoint                      = "main";

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.Desc.Name       = "LandscapeEditor forward debug triangle VS";
        ShaderCI.Source          = ForwardDebugTriangleVS;
        pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "LandscapeEditor forward debug triangle PS";
        ShaderCI.Source          = ForwardDebugTrianglePS;
        pDevice->CreateShader(ShaderCI, &pPS);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pTrianglePSO);
}

void ForwardDebugPipeline::Render(IDeviceContext* pContext)
{
    pContext->SetPipelineState(m_pTrianglePSO);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 3;
    pContext->Draw(DrawAttrs);
}

} // namespace Diligent
