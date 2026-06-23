#include "LandscapeEditor.hpp"

#include "CommandLineParser.hpp"
#include "imgui.h"

#include <algorithm>
#include <string>

namespace Diligent
{

SampleBase* CreateSample()
{
    return new LandscapeEditor();
}

LandscapeEditor::CommandLineStatus LandscapeEditor::ProcessCommandLine(int argc, const char* const* argv)
{
    CommandLineParser ArgsParser{argc, argv};

    std::string Preset;
    if (!ArgsParser.Parse("landscape_camera_preset", Preset))
        return CommandLineStatus::OK;

    if (Preset == "default")
    {
        m_CameraPreset = LandscapeCameraPreset::Default;
    }
    else if (Preset == "mixed_lod")
    {
        m_CameraPreset = LandscapeCameraPreset::MixedLOD;
    }
    else if (Preset == "off_frustum")
    {
        m_CameraPreset = LandscapeCameraPreset::OffFrustum;
    }
    else
    {
        LOG_ERROR_MESSAGE("Invalid landscape_camera_preset '", Preset, "'. Expected one of: default, mixed_lod, off_frustum.");
        return CommandLineStatus::Error;
    }

    return CommandLineStatus::OK;
}

void LandscapeEditor::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    ApplyCameraPreset();

    m_FrameResources.Initialize(m_pDevice);
    m_ForwardRenderer.Initialize(m_pDevice, m_pSwapChain);
    m_ForwardRenderer.SetShowQuadtreeOverlay(m_ShowQuadtreeOverlay);
    m_ForwardRenderer.SetShowSkirtEdgeOverlay(m_ShowSkirtEdgeOverlay);
    m_ForwardRenderer.SetShowLODTransitionOverlay(m_ShowLODTransitionOverlay);
    m_ForwardRenderer.SetTerrainFrustumCullingEnabled(m_EnableTerrainFrustumCulling);
    m_ForwardRenderer.SetTerrainLODIndexStitchingEnabled(m_EnableTerrainLODIndexStitching);
    m_ForwardRenderer.SetTerrainLODDistanceScale(m_TerrainLODDistanceScale);
    m_ForwardRenderer.SetTerrainMaxSelectedLODLevel(static_cast<Uint32>(m_TerrainMaxSelectedLODLevel));
    UpdateRenderView();
}

void LandscapeEditor::Render()
{
    const float ClearColor[] = {0.08f, 0.09f, 0.10f, 1.0f};

    ITextureView* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    ITextureView* pDSV = m_pSwapChain->GetDepthBufferDSV();
    m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    m_FrameResources.UpdateCameraConstants(m_pImmediateContext, m_RenderView);
    m_ForwardRenderer.Render(m_pImmediateContext, m_RenderView, m_FrameResources);
}

void LandscapeEditor::Update(double CurrTime, double ElapsedTime, bool DoUpdateUI)
{
    SampleBase::Update(CurrTime, ElapsedTime, DoUpdateUI);
    m_Camera.Update(GetInputController(), static_cast<float>(ElapsedTime));
    UpdateRenderView();

    if (DoUpdateUI)
    {
        ImGui::Begin("Landscape Forward");
        ImGui::Text("Mode: Forward Debug");
        ImGui::Text("Camera preset: %s", GetCameraPresetName());
        ImGui::Text("Camera: %.2f %.2f %.2f", m_RenderView.CameraPosition.x, m_RenderView.CameraPosition.y, m_RenderView.CameraPosition.z);
        ImGui::Text("Viewport: %.0f x %.0f", m_RenderView.ViewportSize.x, m_RenderView.ViewportSize.y);
        const auto& Stats = m_ForwardRenderer.GetStats();
        ImGui::Text("Opaque items: %u", Stats.OpaqueItemCount);
        ImGui::Text("Terrain tris: %u", Stats.TerrainTriangleCount);
        ImGui::Text("Terrain cells: %u", Stats.TerrainCellCount);
        ImGui::Text("Terrain render items: %u", Stats.TerrainRenderItemCount);
        ImGui::Text("Terrain rendered cells: %u", Stats.TerrainRenderedCellCount);
        ImGui::Text("Terrain rendered mesh cells: %u", Stats.TerrainRenderedMeshCellCount);
        ImGui::Text("LOD sample step: %u .. %u", Stats.TerrainMinLODSampleStep, Stats.TerrainMaxLODSampleStep);
        ImGui::Text("Terrain draw calls: %u forward / %u shadow", Stats.TerrainForwardDrawCallCount, Stats.TerrainShadowDrawCallCount);
        ImGui::Text("Tile meshes: %u", Stats.TerrainTileMeshCount);
        ImGui::Text("Packed tile vertices: %u", Stats.TerrainPackedVertexCount);
        ImGui::Text("Packed tile indices: %u", Stats.TerrainPackedIndexCount);
        if (ImGui::Checkbox("Enable terrain skirts", &m_EnableTerrainSkirts))
            m_ForwardRenderer.SetTerrainSkirtsEnabled(m_EnableTerrainSkirts);
        ImGui::Text("Skirt depth: %.2f", Stats.TerrainSkirtDepth);
        ImGui::Text("Skirt vertices: %u", Stats.TerrainSkirtVertexCount);
        ImGui::Text("Skirt indices: %u", Stats.TerrainSkirtIndexCount);
        ImGui::Text("Terrain samples/axis: %u", Stats.TerrainSampleCountPerAxis);
        ImGui::Text("Height range: %.2f .. %.2f", Stats.TerrainMinHeight, Stats.TerrainMaxHeight);
        ImGui::Text("Average height: %.2f", Stats.TerrainAverageHeight);
        if (ImGui::Checkbox("Show quadtree overlay", &m_ShowQuadtreeOverlay))
            m_ForwardRenderer.SetShowQuadtreeOverlay(m_ShowQuadtreeOverlay);
        if (ImGui::Checkbox("Show skirt edge overlay", &m_ShowSkirtEdgeOverlay))
            m_ForwardRenderer.SetShowSkirtEdgeOverlay(m_ShowSkirtEdgeOverlay);
        if (ImGui::Checkbox("Show LOD transition overlay", &m_ShowLODTransitionOverlay))
            m_ForwardRenderer.SetShowLODTransitionOverlay(m_ShowLODTransitionOverlay);
        ImGui::Text("Quadtree nodes: %u", Stats.TerrainQuadtreeNodeCount);
        ImGui::Text("Selected leaves: %u", Stats.TerrainQuadtreeSelectedLeafCount);
        if (ImGui::SliderFloat("LOD distance scale", &m_TerrainLODDistanceScale, 0.50f, 4.00f, "%.2f"))
            m_ForwardRenderer.SetTerrainLODDistanceScale(m_TerrainLODDistanceScale);
        const int MaxLODLevel = std::max(1, static_cast<int>(Stats.TerrainQuadtreeMaxDepth));
        m_TerrainMaxSelectedLODLevel = std::clamp(m_TerrainMaxSelectedLODLevel, 1, MaxLODLevel);
        if (ImGui::SliderInt("Max terrain LOD", &m_TerrainMaxSelectedLODLevel, 1, MaxLODLevel))
            m_ForwardRenderer.SetTerrainMaxSelectedLODLevel(static_cast<Uint32>(m_TerrainMaxSelectedLODLevel));
        ImGui::Text("LOD policy: scale %.2f, max level %u", Stats.TerrainLODDistanceScale, Stats.TerrainMaxSelectableLODLevel);
        ImGui::Text("Component size: root %.2f, finest %.2f", Stats.TerrainRootComponentWorldSize, Stats.TerrainFineComponentWorldSize);
        ImGui::Text("Selected component size: %.2f .. %.2f", Stats.TerrainSelectedMinComponentWorldSize, Stats.TerrainSelectedMaxComponentWorldSize);
        ImGui::Text("Selected LOD range: %u .. %u", Stats.TerrainQuadtreeMinSelectedLevel, Stats.TerrainQuadtreeMaxSelectedLevel);
        if (ImGui::Checkbox("Enable terrain frustum culling", &m_EnableTerrainFrustumCulling))
            m_ForwardRenderer.SetTerrainFrustumCullingEnabled(m_EnableTerrainFrustumCulling);
        ImGui::Text("Candidate leaves: %u", Stats.TerrainQuadtreeCandidateLeafCount);
        ImGui::Text("Visible leaves: %u", Stats.TerrainQuadtreeVisibleLeafCount);
        ImGui::Text("Frustum culled leaves: %u", Stats.TerrainFrustumCulledLeafCount);
        ImGui::Text("Max quadtree depth: %u", Stats.TerrainQuadtreeMaxDepth);
        ImGui::Text("Max selected LOD: %u", Stats.TerrainQuadtreeMaxSelectedLevel);
        ImGui::Text("Debug leaf bound lines: %u", Stats.TerrainDebugLeafBoundLineCount);
        ImGui::Text("Debug skirt edges: %u", Stats.TerrainDebugSkirtEdgeCount);
        ImGui::Text("Debug LOD transition edges: %u", Stats.TerrainDebugLODTransitionEdgeCount);
        ImGui::Text("Debug line vertices: %u", Stats.TerrainDebugLineVertexCount);
        ImGui::Text("LOD stitching seams: %u", Stats.TerrainLODStitchingSeamEdgeCount);
        ImGui::Text("LOD stitching max delta: %u, ratio: %u", Stats.TerrainLODStitchingMaxDelta, Stats.TerrainLODStitchingMaxRatio);
        ImGui::Text("LOD stitching length: %.2f", Stats.TerrainLODStitchingTotalLength);
        if (ImGui::Checkbox("Enable LOD index stitching", &m_EnableTerrainLODIndexStitching))
            m_ForwardRenderer.SetTerrainLODIndexStitchingEnabled(m_EnableTerrainLODIndexStitching);
        ImGui::Text("LOD index stitched nodes: %u, edges: %u", Stats.TerrainLODIndexStitchingNodeCount, Stats.TerrainLODIndexStitchingEdgeCount);
        ImGui::Text("LOD index stitched corners: %u", Stats.TerrainLODIndexStitchingCornerCount);
        ImGui::Text("LOD index stitched indices: %u, ratio: %u", Stats.TerrainLODIndexStitchingIndexCount, Stats.TerrainLODIndexStitchingMaxRatio);
        ImGui::Text("LOD index corner indices: %u", Stats.TerrainLODIndexStitchingCornerIndexCount);
        ImGui::Text("Shadow cascades: %u", Stats.ShadowCascadeCount);
        ImGui::Text("Shadow map: %u", Stats.ShadowMapSize);
        ImGui::Text("Sky passes: %u", Stats.SkyPassCount);
        ImGui::Text("Transparent items: %u", Stats.TransparentItemCount);
        ImGui::Text("Transparent passes: %u", Stats.TransparentPassCount);
        ImGui::Text("Debug items: %u", Stats.DebugItemCount);
        ImGui::Text("Postprocess passes: %u", Stats.PostProcessPassCount);
        ImGui::Text("PSOs: %zu", Stats.PSOCount);
        ImGui::Text("PSO creations: %zu", Stats.PSOCreationCount);
        ImGui::End();
    }
}

void LandscapeEditor::ApplyCameraPreset()
{
    switch (m_CameraPreset)
    {
        case LandscapeCameraPreset::Default:
            m_Camera.SetPos(float3{0.0f, 6.0f, -14.0f});
            m_Camera.SetRotation(0.0f, -0.4f);
            break;

        case LandscapeCameraPreset::MixedLOD:
            m_Camera.SetPos(float3{15.0f, 7.5f, -20.0f});
            m_Camera.SetRotation(-0.62f, -0.36f);
            break;

        case LandscapeCameraPreset::OffFrustum:
            m_Camera.SetPos(float3{18.0f, 7.5f, -20.0f});
            m_Camera.SetRotation(-0.50f, -0.36f);
            break;
    }

    m_Camera.SetMoveSpeed(8.0f);
    m_Camera.SetSpeedUpScales(4.0f, 12.0f);
    m_Camera.Update(GetInputController(), 0.0f);
}

const Char* LandscapeEditor::GetCameraPresetName() const
{
    switch (m_CameraPreset)
    {
        case LandscapeCameraPreset::Default:
            return "default";
        case LandscapeCameraPreset::MixedLOD:
            return "mixed_lod";
        case LandscapeCameraPreset::OffFrustum:
            return "off_frustum";
    }

    return "unknown";
}

void LandscapeEditor::WindowResize(Uint32 Width, Uint32 Height)
{
    SampleBase::WindowResize(Width, Height);
    UpdateRenderView();
}

void LandscapeEditor::UpdateRenderView()
{
    const auto& SCDesc = m_pSwapChain->GetDesc();
    const float Width  = static_cast<float>(SCDesc.Width != 0 ? SCDesc.Width : 1);
    const float Height = static_cast<float>(SCDesc.Height != 0 ? SCDesc.Height : 1);

    constexpr float NearPlane = 0.1f;
    constexpr float FarPlane  = 1000.0f;
    const float     Aspect    = Width / Height;
    const bool      IsGL      = m_pDevice->GetDeviceInfo().IsGLDevice();

    m_Camera.SetProjAttribs(NearPlane, FarPlane, Aspect, PI_F / 4.0f, SCDesc.PreTransform, IsGL);

    m_RenderView.View           = m_Camera.GetViewMatrix();
    m_RenderView.Projection     = m_Camera.GetProjMatrix();
    m_RenderView.ViewProj       = m_RenderView.View * m_RenderView.Projection;
    m_RenderView.CameraPosition = m_Camera.GetPos();
    m_RenderView.NearPlane      = NearPlane;
    m_RenderView.FarPlane       = FarPlane;
    m_RenderView.ViewportSize   = float2{Width, Height};
    m_RenderView.ColorFormat    = SCDesc.ColorBufferFormat;
    m_RenderView.DepthFormat    = SCDesc.DepthBufferFormat;
    m_RenderView.IsGL           = IsGL;
}

} // namespace Diligent
