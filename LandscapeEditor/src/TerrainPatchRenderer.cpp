#include "TerrainPatchRenderer.hpp"

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

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
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

static constexpr float SkirtDepth = 1.25f;

TerrainDrawRegion BuildTerrainDrawRegion(const TerrainHeightField& HeightField, float TerrainExtent, const TerrainQuadtreeNode& Node)
{
    const Uint32 CellCount = HeightField.GetCellCount();
    const float  InvSize   = CellCount > 0 ? static_cast<float>(CellCount) / (2.0f * TerrainExtent) : 0.0f;

    auto ToCellBoundary = [&](float WorldCoord) {
        const float CellCoord = (WorldCoord + TerrainExtent) * InvSize;
        const Int32 Rounded = static_cast<Int32>(std::lround(CellCoord));
        return static_cast<Uint32>(std::clamp<Int32>(Rounded, 0, static_cast<Int32>(CellCount)));
    };

    const Uint32 MinCellX = ToCellBoundary(Node.MinXZ.x);
    const Uint32 MaxCellX = ToCellBoundary(Node.MaxXZ.x);
    const Uint32 MinCellZ = ToCellBoundary(Node.MinXZ.y);
    const Uint32 MaxCellZ = ToCellBoundary(Node.MaxXZ.y);

    TerrainDrawRegion Region;
    Region.NodeIndex  = Node.NodeIndex;
    Region.Level      = Node.Level;
    Region.MinXZ      = Node.MinXZ;
    Region.MaxXZ      = Node.MaxXZ;
    Region.FirstCellX = std::min(MinCellX, CellCount > 0 ? CellCount - 1u : 0u);
    Region.FirstCellZ = std::min(MinCellZ, CellCount > 0 ? CellCount - 1u : 0u);

    const Uint32 EndCellX = std::clamp(MaxCellX, Region.FirstCellX + 1u, CellCount);
    const Uint32 EndCellZ = std::clamp(MaxCellZ, Region.FirstCellZ + 1u, CellCount);
    Region.CellCountX = EndCellX - Region.FirstCellX;
    Region.CellCountZ = EndCellZ - Region.FirstCellZ;
    Region.LODSampleStep   = 1;
    Region.MeshCellCountX  = Region.CellCountX;
    Region.MeshCellCountZ  = Region.CellCountZ;
    Region.MeshSampleCountX = Region.CellCountX + 1u;
    Region.MeshSampleCountZ = Region.CellCountZ + 1u;
    return Region;
}

Uint32 AppendSkirtEdge(std::vector<TerrainVertex>& Vertices,
                       std::vector<Uint32>&        Indices,
                       Uint32                      BaseVertex,
                       const std::vector<Uint32>&  TopLocalIndices,
                       const float3&               SkirtNormal,
                       float                       SkirtDepthValue)
{
    const Uint32 FirstBottomVertex = static_cast<Uint32>(Vertices.size());
    const Uint32 FirstBottomLocalVertex = FirstBottomVertex - BaseVertex;
    for (Uint32 TopLocalIndex : TopLocalIndices)
    {
        TerrainVertex BottomVertex = Vertices[BaseVertex + TopLocalIndex];
        const float3   TopPos = BottomVertex.Pos;
        BottomVertex.Pos.y    = TopPos.y - SkirtDepthValue;
        BottomVertex.Normal   = SkirtNormal;
        Vertices.push_back(BottomVertex);
    }

    for (Uint32 Segment = 0; Segment + 1u < TopLocalIndices.size(); ++Segment)
    {
        const Uint32 TopA = TopLocalIndices[Segment];
        const Uint32 TopB = TopLocalIndices[Segment + 1u];
        const Uint32 BottomA = FirstBottomLocalVertex + Segment;
        const Uint32 BottomB = FirstBottomLocalVertex + Segment + 1u;

        Indices.push_back(TopA);
        Indices.push_back(BottomA);
        Indices.push_back(TopB);
        Indices.push_back(TopB);
        Indices.push_back(BottomA);
        Indices.push_back(BottomB);
    }

    return static_cast<Uint32>(Vertices.size()) - FirstBottomVertex;
}

Uint32 AppendTileSkirts(std::vector<TerrainVertex>& Vertices,
                        std::vector<Uint32>&        Indices,
                        Uint32                      BaseVertex,
                        Uint32                      SampleCountX,
                        Uint32                      SampleCountZ,
                        float                       SkirtDepthValue)
{
    auto LocalVertex = [&](Uint32 LocalX, Uint32 LocalZ) {
        return LocalZ * SampleCountX + LocalX;
    };

    std::vector<Uint32> West;
    std::vector<Uint32> East;
    std::vector<Uint32> South;
    std::vector<Uint32> North;
    West.reserve(SampleCountZ);
    East.reserve(SampleCountZ);
    South.reserve(SampleCountX);
    North.reserve(SampleCountX);

    for (Uint32 LocalZ = 0; LocalZ < SampleCountZ; ++LocalZ)
    {
        West.push_back(LocalVertex(0, LocalZ));
        East.push_back(LocalVertex(SampleCountX - 1u, LocalZ));
    }
    for (Uint32 LocalX = 0; LocalX < SampleCountX; ++LocalX)
    {
        South.push_back(LocalVertex(LocalX, 0));
        North.push_back(LocalVertex(LocalX, SampleCountZ - 1u));
    }

    Uint32 SkirtVertexCount = 0;
    SkirtVertexCount += AppendSkirtEdge(Vertices, Indices, BaseVertex, West,  float3{-1.0f, 0.0f, 0.0f}, SkirtDepthValue);
    SkirtVertexCount += AppendSkirtEdge(Vertices, Indices, BaseVertex, East,  float3{+1.0f, 0.0f, 0.0f}, SkirtDepthValue);
    SkirtVertexCount += AppendSkirtEdge(Vertices, Indices, BaseVertex, South, float3{0.0f, 0.0f, -1.0f}, SkirtDepthValue);
    SkirtVertexCount += AppendSkirtEdge(Vertices, Indices, BaseVertex, North, float3{0.0f, 0.0f, +1.0f}, SkirtDepthValue);
    return SkirtVertexCount;
}

void AppendSurfaceCell(std::vector<Uint32>& Indices, Uint32 LocalRowStride, Uint32 LocalX, Uint32 LocalZ)
{
    const Uint32 LocalI0 = LocalZ * LocalRowStride + LocalX;
    const Uint32 LocalI1 = LocalI0 + 1u;
    const Uint32 LocalI2 = LocalI0 + LocalRowStride;
    const Uint32 LocalI3 = LocalI2 + 1u;

    Indices.push_back(LocalI0);
    Indices.push_back(LocalI2);
    Indices.push_back(LocalI1);
    Indices.push_back(LocalI1);
    Indices.push_back(LocalI2);
    Indices.push_back(LocalI3);
}

std::vector<Uint32> BuildStitchStops(Uint32 SampleCount, Uint32 StitchRatio)
{
    std::vector<Uint32> Stops;
    if (SampleCount == 0)
        return Stops;

    const Uint32 Last = SampleCount - 1u;
    const Uint32 Step = std::max(StitchRatio, 1u);
    Stops.push_back(0);
    for (Uint32 Sample = Step; Sample < Last; Sample += Step)
        Stops.push_back(Sample);
    if (Stops.back() != Last)
        Stops.push_back(Last);
    return Stops;
}

void AppendStitchedEdgeBand(std::vector<Uint32>& Indices,
                            const TerrainDrawRegion& Region,
                            TerrainLODStitchEdgeMask Edge,
                            Uint32 StitchRatio)
{
    if (Region.MeshSampleCountX < 2u || Region.MeshSampleCountZ < 2u)
        return;

    auto LocalVertex = [&](Uint32 LocalX, Uint32 LocalZ) {
        return LocalZ * Region.MeshSampleCountX + LocalX;
    };

    const bool VerticalEdge = Edge == TerrainLODStitchEdgeMask::West || Edge == TerrainLODStitchEdgeMask::East;
    const Uint32 SampleCount = VerticalEdge ? Region.MeshSampleCountZ : Region.MeshSampleCountX;
    const std::vector<Uint32> Stops = BuildStitchStops(SampleCount, StitchRatio);
    if (Stops.size() < 2u)
        return;

    auto BoundaryVertex = [&](Uint32 Sample) {
        if (Edge == TerrainLODStitchEdgeMask::West)
            return LocalVertex(0, Sample);
        if (Edge == TerrainLODStitchEdgeMask::East)
            return LocalVertex(Region.MeshSampleCountX - 1u, Sample);
        if (Edge == TerrainLODStitchEdgeMask::South)
            return LocalVertex(Sample, 0);
        return LocalVertex(Sample, Region.MeshSampleCountZ - 1u);
    };

    auto InnerVertex = [&](Uint32 Sample) {
        if (Edge == TerrainLODStitchEdgeMask::West)
            return LocalVertex(1u, Sample);
        if (Edge == TerrainLODStitchEdgeMask::East)
            return LocalVertex(Region.MeshSampleCountX - 2u, Sample);
        if (Edge == TerrainLODStitchEdgeMask::South)
            return LocalVertex(Sample, 1u);
        return LocalVertex(Sample, Region.MeshSampleCountZ - 2u);
    };

    for (size_t StopIndex = 0; StopIndex + 1u < Stops.size(); ++StopIndex)
    {
        const Uint32 Start = Stops[StopIndex];
        const Uint32 End   = Stops[StopIndex + 1u];
        const Uint32 BoundaryStart = BoundaryVertex(Start);
        const Uint32 BoundaryEnd   = BoundaryVertex(End);

        Indices.push_back(BoundaryStart);
        Indices.push_back(BoundaryEnd);
        Indices.push_back(InnerVertex(End));

        for (Uint32 Sample = End; Sample > Start; --Sample)
        {
            Indices.push_back(BoundaryStart);
            Indices.push_back(InnerVertex(Sample));
            Indices.push_back(InnerVertex(Sample - 1u));
        }
    }
}

void AppendStitchedSurfaceIndices(std::vector<Uint32>& Indices,
                                  const TerrainDrawRegion& Region,
                                  const TerrainLODStitchedDrawRegion& StitchedRegion)
{
    const TerrainLODStitchEdgeMask EdgeMask = StitchedRegion.EdgeMask;
    const bool StitchWest  = HasTerrainLODStitchEdge(EdgeMask, TerrainLODStitchEdgeMask::West);
    const bool StitchEast  = HasTerrainLODStitchEdge(EdgeMask, TerrainLODStitchEdgeMask::East);
    const bool StitchSouth = HasTerrainLODStitchEdge(EdgeMask, TerrainLODStitchEdgeMask::South);
    const bool StitchNorth = HasTerrainLODStitchEdge(EdgeMask, TerrainLODStitchEdgeMask::North);

    const Uint32 StartCellX = StitchWest ? 1u : 0u;
    const Uint32 StartCellZ = StitchSouth ? 1u : 0u;
    const Uint32 EndCellX = Region.MeshCellCountX > (StitchEast ? 1u : 0u) ? Region.MeshCellCountX - (StitchEast ? 1u : 0u) : 0u;
    const Uint32 EndCellZ = Region.MeshCellCountZ > (StitchNorth ? 1u : 0u) ? Region.MeshCellCountZ - (StitchNorth ? 1u : 0u) : 0u;

    if (StartCellX < EndCellX && StartCellZ < EndCellZ)
    {
        for (Uint32 LocalZ = StartCellZ; LocalZ < EndCellZ; ++LocalZ)
        {
            for (Uint32 LocalX = StartCellX; LocalX < EndCellX; ++LocalX)
                AppendSurfaceCell(Indices, Region.MeshSampleCountX, LocalX, LocalZ);
        }
    }

    if (StitchWest)
        AppendStitchedEdgeBand(Indices, Region, TerrainLODStitchEdgeMask::West, StitchedRegion.WestStitchRatio);
    if (StitchEast)
        AppendStitchedEdgeBand(Indices, Region, TerrainLODStitchEdgeMask::East, StitchedRegion.EastStitchRatio);
    if (StitchSouth)
        AppendStitchedEdgeBand(Indices, Region, TerrainLODStitchEdgeMask::South, StitchedRegion.SouthStitchRatio);
    if (StitchNorth)
        AppendStitchedEdgeBand(Indices, Region, TerrainLODStitchEdgeMask::North, StitchedRegion.NorthStitchRatio);
}

void AppendSkirtEdgeIndices(std::vector<Uint32>&       Indices,
                            const std::vector<Uint32>& TopLocalIndices,
                            Uint32                     FirstBottomLocalVertex)
{
    for (Uint32 Segment = 0; Segment + 1u < TopLocalIndices.size(); ++Segment)
    {
        const Uint32 TopA = TopLocalIndices[Segment];
        const Uint32 TopB = TopLocalIndices[Segment + 1u];
        const Uint32 BottomA = FirstBottomLocalVertex + Segment;
        const Uint32 BottomB = FirstBottomLocalVertex + Segment + 1u;

        Indices.push_back(TopA);
        Indices.push_back(BottomA);
        Indices.push_back(TopB);
        Indices.push_back(TopB);
        Indices.push_back(BottomA);
        Indices.push_back(BottomB);
    }
}

void AppendTileSkirtIndices(std::vector<Uint32>& Indices, Uint32 SampleCountX, Uint32 SampleCountZ)
{
    auto LocalVertex = [&](Uint32 LocalX, Uint32 LocalZ) {
        return LocalZ * SampleCountX + LocalX;
    };

    std::vector<Uint32> West;
    std::vector<Uint32> East;
    std::vector<Uint32> South;
    std::vector<Uint32> North;
    West.reserve(SampleCountZ);
    East.reserve(SampleCountZ);
    South.reserve(SampleCountX);
    North.reserve(SampleCountX);

    for (Uint32 LocalZ = 0; LocalZ < SampleCountZ; ++LocalZ)
    {
        West.push_back(LocalVertex(0, LocalZ));
        East.push_back(LocalVertex(SampleCountX - 1u, LocalZ));
    }
    for (Uint32 LocalX = 0; LocalX < SampleCountX; ++LocalX)
    {
        South.push_back(LocalVertex(LocalX, 0));
        North.push_back(LocalVertex(LocalX, SampleCountZ - 1u));
    }

    Uint32 FirstBottomLocalVertex = SampleCountX * SampleCountZ;
    AppendSkirtEdgeIndices(Indices, West, FirstBottomLocalVertex);
    FirstBottomLocalVertex += SampleCountZ;
    AppendSkirtEdgeIndices(Indices, East, FirstBottomLocalVertex);
    FirstBottomLocalVertex += SampleCountZ;
    AppendSkirtEdgeIndices(Indices, South, FirstBottomLocalVertex);
    FirstBottomLocalVertex += SampleCountX;
    AppendSkirtEdgeIndices(Indices, North, FirstBottomLocalVertex);
}

Uint32 GetTerrainRegionDrawIndexCount(const TerrainDrawRegion& Region, bool EnableSkirts)
{
    if (Region.UseStitchedIndexBuffer)
        return EnableSkirts ? Region.StitchedNumIndices : Region.StitchedMainNumIndices;
    return EnableSkirts ? Region.NumIndices : Region.MainNumIndices;
}

Uint32 GetMaxQuadtreeLevel(const std::vector<TerrainQuadtreeNode>& QuadtreeNodes)
{
    Uint32 MaxLevel = 0;
    for (const TerrainQuadtreeNode& Node : QuadtreeNodes)
        MaxLevel = std::max(MaxLevel, Node.Level);
    return MaxLevel;
}

Uint32 ComputeTerrainLODSampleStep(Uint32 NodeLevel, Uint32 MaxQuadtreeLevel)
{
    if (NodeLevel >= MaxQuadtreeLevel)
        return 1u;

    const Uint32 Shift = MaxQuadtreeLevel - NodeLevel;
    return 1u << Shift;
}

std::vector<Uint32> BuildSampledCellCoordinates(Uint32 FirstCell, Uint32 CellCount, Uint32 LODSampleStep)
{
    std::vector<Uint32> Cells;
    const Uint32        EndCell = FirstCell + CellCount;
    const Uint32        Step    = std::max(LODSampleStep, 1u);

    Cells.push_back(FirstCell);
    for (Uint32 Cell = FirstCell + Step; Cell < EndCell; Cell += Step)
        Cells.push_back(Cell);

    if (Cells.back() != EndCell)
        Cells.push_back(EndCell);

    return Cells;
}

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

TerrainDrawRegion TerrainPatchRenderer::BuildDrawRegion(const TerrainQuadtreeNode& Node) const
{
    if (Node.NodeIndex < m_TileMeshRanges.size())
        return m_TileMeshRanges[Node.NodeIndex].Region;

    return BuildTerrainDrawRegion(m_HeightField, m_TerrainExtent, Node);
}

void TerrainPatchRenderer::BeginFrameStats()
{
    m_LastRenderedCellCount    = 0;
    m_LastRenderedMeshCellCount = 0;
    m_LastRenderedIndexCount   = 0;
    m_LastForwardDrawCallCount = 0;
    m_LastShadowDrawCallCount  = 0;
}

void TerrainPatchRenderer::BuildPackedTileMeshCache(IRenderDevice* pDevice, const std::vector<TerrainQuadtreeNode>& QuadtreeNodes)
{
    std::vector<TerrainVertex> Vertices;
    std::vector<Uint32>        Indices;
    m_TileMeshRanges.clear();
    m_TileMeshRanges.resize(QuadtreeNodes.size());
    m_PackedTileSkirtVertexCount = 0;
    m_PackedTileSkirtIndexCount  = 0;
    m_MinLODSampleStep = QuadtreeNodes.empty() ? 1u : std::numeric_limits<Uint32>::max();
    m_MaxLODSampleStep = 1;

    const Uint32 TerrainCellCount = m_HeightField.GetCellCount();
    const float  CellSize = TerrainCellCount > 0 ? (2.0f * m_TerrainExtent) / static_cast<float>(TerrainCellCount) : 0.0f;
    const Uint32 MaxQuadtreeLevel = GetMaxQuadtreeLevel(QuadtreeNodes);

    for (const TerrainQuadtreeNode& Node : QuadtreeNodes)
    {
        TerrainDrawRegion Region = BuildTerrainDrawRegion(m_HeightField, m_TerrainExtent, Node);
        Region.LODSampleStep = ComputeTerrainLODSampleStep(Node.Level, MaxQuadtreeLevel);
        Region.BaseVertex         = static_cast<Uint32>(Vertices.size());
        Region.FirstIndexLocation = static_cast<Uint32>(Indices.size());

        const std::vector<Uint32> SampleCellXs = BuildSampledCellCoordinates(Region.FirstCellX, Region.CellCountX, Region.LODSampleStep);
        const std::vector<Uint32> SampleCellZs = BuildSampledCellCoordinates(Region.FirstCellZ, Region.CellCountZ, Region.LODSampleStep);
        Region.MeshSampleCountX = static_cast<Uint32>(SampleCellXs.size());
        Region.MeshSampleCountZ = static_cast<Uint32>(SampleCellZs.size());
        Region.MeshCellCountX = Region.MeshSampleCountX > 0 ? Region.MeshSampleCountX - 1u : 0u;
        Region.MeshCellCountZ = Region.MeshSampleCountZ > 0 ? Region.MeshSampleCountZ - 1u : 0u;

        const Uint32 SampleCountX = Region.MeshSampleCountX;
        const Uint32 SampleCountZ = Region.MeshSampleCountZ;
        Vertices.reserve(Vertices.size() + SampleCountX * SampleCountZ);
        Indices.reserve(Indices.size() + Region.MeshCellCountX * Region.MeshCellCountZ * 6u);
        m_MinLODSampleStep = std::min(m_MinLODSampleStep, Region.LODSampleStep);
        m_MaxLODSampleStep = std::max(m_MaxLODSampleStep, Region.LODSampleStep);

        for (Uint32 SampleZ : SampleCellZs)
        {
            const float WorldZ = -m_TerrainExtent + CellSize * static_cast<float>(SampleZ);
            for (Uint32 SampleX : SampleCellXs)
            {
                const float WorldX = -m_TerrainExtent + CellSize * static_cast<float>(SampleX);
                Vertices.push_back(TerrainVertex{
                    float3{WorldX, m_HeightField.GetHeight(SampleX, SampleZ), WorldZ},
                    m_HeightField.GetNormal(SampleX, SampleZ),
                    m_HeightField.GetUV(SampleX, SampleZ)});
            }
        }

        const Uint32 LocalRowStride = SampleCountX;
        for (Uint32 LocalZ = 0; LocalZ < Region.MeshCellCountZ; ++LocalZ)
        {
            for (Uint32 LocalX = 0; LocalX < Region.MeshCellCountX; ++LocalX)
                AppendSurfaceCell(Indices, LocalRowStride, LocalX, LocalZ);
        }

        Region.MainNumIndices = static_cast<Uint32>(Indices.size()) - Region.FirstIndexLocation;
        const Uint32 SkirtFirstIndex = static_cast<Uint32>(Indices.size());
        const Uint32 SkirtVertexCount = AppendTileSkirts(Vertices, Indices, Region.BaseVertex, SampleCountX, SampleCountZ, SkirtDepth);
        Region.NumIndices = static_cast<Uint32>(Indices.size()) - Region.FirstIndexLocation;
        Region.SkirtIndexCount = static_cast<Uint32>(Indices.size()) - SkirtFirstIndex;

        TerrainTileMeshRange Range;
        Range.Region      = Region;
        Range.VertexCount = static_cast<Uint32>(Vertices.size()) - Region.BaseVertex;
        m_PackedTileSkirtVertexCount += SkirtVertexCount;
        m_PackedTileSkirtIndexCount += Region.SkirtIndexCount;

        if (Node.NodeIndex < m_TileMeshRanges.size())
            m_TileMeshRanges[Node.NodeIndex] = Range;
    }

    if (m_MinLODSampleStep == std::numeric_limits<Uint32>::max())
        m_MinLODSampleStep = 1;

    m_PackedTileVertexCount = static_cast<Uint32>(Vertices.size());
    m_PackedTileIndexCount  = static_cast<Uint32>(Indices.size());
    m_IndexCount            = m_PackedTileIndexCount;

    BufferDesc VBDesc;
    VBDesc.Name      = "LandscapeEditor packed terrain tile vertex buffer";
    VBDesc.Usage     = USAGE_IMMUTABLE;
    VBDesc.BindFlags = BIND_VERTEX_BUFFER;
    VBDesc.Size      = static_cast<Uint64>(Vertices.size() * sizeof(TerrainVertex));
    BufferData VBData;
    VBData.pData    = Vertices.data();
    VBData.DataSize = VBDesc.Size;
    pDevice->CreateBuffer(VBDesc, &VBData, &m_pVertexBuffer);

    BufferDesc IBDesc;
    IBDesc.Name      = "LandscapeEditor packed terrain tile index buffer";
    IBDesc.Usage     = USAGE_IMMUTABLE;
    IBDesc.BindFlags = BIND_INDEX_BUFFER;
    IBDesc.Size      = static_cast<Uint64>(Indices.size() * sizeof(Uint32));
    BufferData IBData;
    IBData.pData    = Indices.data();
    IBData.DataSize = IBDesc.Size;
    pDevice->CreateBuffer(IBDesc, &IBData, &m_pIndexBuffer);

    BufferDesc StitchedIBDesc;
    StitchedIBDesc.Name           = "LandscapeEditor dynamic terrain LOD stitched index buffer";
    StitchedIBDesc.Usage          = USAGE_DYNAMIC;
    StitchedIBDesc.BindFlags      = BIND_INDEX_BUFFER;
    StitchedIBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    StitchedIBDesc.Size           = static_cast<Uint64>(std::max<Uint32>(m_PackedTileIndexCount, 1u) * sizeof(Uint32));
    pDevice->CreateBuffer(StitchedIBDesc, nullptr, &m_pStitchedIndexBuffer);
    m_StitchedIndexBufferCapacity = std::max<Uint32>(m_PackedTileIndexCount, 1u);
}

void TerrainPatchRenderer::Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache, const std::vector<TerrainQuadtreeNode>& QuadtreeNodes)
{
    TerrainHeightFieldDesc HeightFieldDesc;
    m_HeightField.GenerateProcedural(HeightFieldDesc);
    m_TerrainExtent = HeightFieldDesc.Extent;
    m_SkirtDepth    = SkirtDepth;

    BuildPackedTileMeshCache(pDevice, QuadtreeNodes);

    m_pTerrainPSO = PSOCache.GetOrCreate("ForwardOpaque.TerrainPatch.Heightfield.v1", [&]() {
        return CreateTerrainPipelineState(pDevice, pSwapChain);
    });
    m_pTerrainPSO->CreateShaderResourceBinding(&m_pTerrainSRB, true);

    m_pShadowPSO = PSOCache.GetOrCreate("Shadow.TerrainPatch.Heightfield.v1", [&]() {
        return CreateTerrainShadowPipelineState(pDevice);
    });
    m_pShadowPSO->CreateShaderResourceBinding(&m_pShadowSRB, true);
}

void TerrainPatchRenderer::PrepareLODIndexStitching(IDeviceContext* pContext, TerrainLODIndexStitching& Stitching)
{
    std::vector<Uint32> Indices;
    Indices.reserve(m_StitchedIndexBufferCapacity);

    for (TerrainLODStitchedDrawRegion& StitchedRegion : Stitching.GetRegions())
    {
        if (StitchedRegion.NodeIndex >= m_TileMeshRanges.size())
            continue;

        const TerrainDrawRegion& Region = m_TileMeshRanges[StitchedRegion.NodeIndex].Region;
        StitchedRegion.FirstIndexLocation = static_cast<Uint32>(Indices.size());
        AppendStitchedSurfaceIndices(Indices, Region, StitchedRegion);
        StitchedRegion.MainNumIndices = static_cast<Uint32>(Indices.size()) - StitchedRegion.FirstIndexLocation;
        AppendTileSkirtIndices(Indices, Region.MeshSampleCountX, Region.MeshSampleCountZ);
        StitchedRegion.NumIndices = static_cast<Uint32>(Indices.size()) - StitchedRegion.FirstIndexLocation;
    }

    VERIFY(Indices.size() <= m_StitchedIndexBufferCapacity, "Dynamic terrain LOD stitched index buffer capacity is too small");
    if (!Indices.empty())
    {
        MapHelper<Uint32> MappedIndices{pContext, m_pStitchedIndexBuffer, MAP_WRITE, MAP_FLAG_DISCARD};
        std::memcpy(MappedIndices, Indices.data(), Indices.size() * sizeof(Uint32));
    }

    Stitching.SetGeneratedIndexStats(static_cast<Uint32>(Indices.size()));
}

void TerrainPatchRenderer::Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources, const TerrainDrawRegion& Region, ITextureView* pShadowMapSRV)
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

    pContext->SetPipelineState(m_pTerrainPSO);
    pContext->CommitShaderResources(m_pTerrainSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawTileMeshIndexed(pContext, Region);
    const Uint32 DrawIndexCount = GetTerrainRegionDrawIndexCount(Region, m_EnableSkirts);
    m_LastForwardDrawCallCount += DrawIndexCount > 0 ? 1u : 0u;
    m_LastRenderedCellCount += Region.CellCountX * Region.CellCountZ;
    m_LastRenderedMeshCellCount += Region.MeshCellCountX * Region.MeshCellCountZ;
    m_LastRenderedIndexCount += DrawIndexCount;
}

void TerrainPatchRenderer::RenderShadow(IDeviceContext* pContext, IBuffer* pShadowConstants, const TerrainDrawRegion& Region)
{
    auto* pShadowConstantsVar = m_pShadowSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ShadowPassConstants");
    VERIFY_EXPR(pShadowConstantsVar != nullptr);
    pShadowConstantsVar->Set(pShadowConstants);

    const Uint64 Offset = 0;
    IBuffer*     VBs[]  = {m_pVertexBuffer};
    pContext->SetVertexBuffers(0, 1, VBs, &Offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    pContext->SetPipelineState(m_pShadowPSO);
    pContext->CommitShaderResources(m_pShadowSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawTileMeshIndexed(pContext, Region);
    m_LastShadowDrawCallCount += GetTerrainRegionDrawIndexCount(Region, m_EnableSkirts) > 0 ? 1u : 0u;
}

void TerrainPatchRenderer::DrawTileMeshIndexed(IDeviceContext* pContext, const TerrainDrawRegion& Region) const
{
    const bool UseStitchedIndexBuffer = Region.UseStitchedIndexBuffer && m_pStitchedIndexBuffer;
    IBuffer* pIndexBuffer = UseStitchedIndexBuffer ? m_pStitchedIndexBuffer.RawPtr() : m_pIndexBuffer.RawPtr();
    const Uint32 FirstIndexLocation = UseStitchedIndexBuffer ? Region.StitchedFirstIndexLocation : Region.FirstIndexLocation;
    const Uint32 DrawIndexCount = UseStitchedIndexBuffer ?
        (m_EnableSkirts ? Region.StitchedNumIndices : Region.StitchedMainNumIndices) :
        (m_EnableSkirts ? Region.NumIndices : Region.MainNumIndices);
    if (DrawIndexCount == 0)
        return;

    pContext->SetIndexBuffer(pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs DrawAttrs;
    DrawAttrs.IndexType          = VT_UINT32;
    DrawAttrs.NumIndices         = DrawIndexCount;
    DrawAttrs.FirstIndexLocation = FirstIndexLocation;
    DrawAttrs.BaseVertex         = Region.BaseVertex;
    DrawAttrs.Flags              = DRAW_FLAG_VERIFY_ALL;
    pContext->DrawIndexed(DrawAttrs);
}

} // namespace Diligent
