#include "ForwardDebugPipeline.hpp"

#include "DeviceContext.h"
#include "RenderDevice.h"
#include "Shader.h"
#include "SwapChain.h"

namespace Diligent
{

namespace
{

static constexpr Uint32 GridResolution  = 10;
static constexpr Uint32 GridVertexCount = (GridResolution + 1) * 4;

static const char* ForwardDebugGridVS = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn)
{
    static const uint  GridResolution = 10;
    static const uint  LinesPerAxis   = GridResolution + 1;
    static const uint  VertLinesEnd   = LinesPerAxis * 2;
    static const float GridExtent     = 0.80;

    uint LineVertex = VertId & 1u;
    uint LineIndex  = VertId >> 1u;

    bool DrawsVerticalLine = VertId < VertLinesEnd;
    uint LocalLineIndex    = DrawsVerticalLine ? LineIndex : LineIndex - LinesPerAxis;

    float Coord  = -GridExtent + GridExtent * 2.0 * float(LocalLineIndex) / float(GridResolution);
    float MinPos = -GridExtent;
    float MaxPos = +GridExtent;

    float2 Pos = DrawsVerticalLine ?
        float2(Coord, LineVertex == 0u ? MinPos : MaxPos) :
        float2(LineVertex == 0u ? MinPos : MaxPos, Coord);

    bool IsAxis = LocalLineIndex == GridResolution / 2u;

    PSIn.Pos   = float4(Pos, 0.0, 1.0);
    PSIn.Color = IsAxis ? float3(0.88, 0.92, 0.95) : float3(0.24, 0.28, 0.30);
}
)";

static const char* ForwardDebugGridPS = R"(
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

    PSOCreateInfo.PSODesc.Name         = "LandscapeEditor forward debug grid PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_LINE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage                  = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.EntryPoint                      = "main";

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.Desc.Name       = "LandscapeEditor forward debug grid VS";
        ShaderCI.Source          = ForwardDebugGridVS;
        pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "LandscapeEditor forward debug grid PS";
        ShaderCI.Source          = ForwardDebugGridPS;
        pDevice->CreateShader(ShaderCI, &pPS);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pGridPSO);
}

void ForwardDebugPipeline::Render(IDeviceContext* pContext)
{
    pContext->SetPipelineState(m_pGridPSO);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = GridVertexCount;
    pContext->Draw(DrawAttrs);
}

} // namespace Diligent
