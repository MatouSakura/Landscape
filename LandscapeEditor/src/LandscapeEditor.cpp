#include "LandscapeEditor.hpp"

#include "imgui.h"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new LandscapeEditor();
}

void LandscapeEditor::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    m_Camera.SetPos(float3{0.0f, 6.0f, -14.0f});
    m_Camera.SetRotation(0.0f, -0.4f);
    m_Camera.SetMoveSpeed(8.0f);
    m_Camera.SetSpeedUpScales(4.0f, 12.0f);
    m_Camera.Update(GetInputController(), 0.0f);

    m_FrameResources.Initialize(m_pDevice);
    m_ForwardRenderer.Initialize(m_pDevice, m_pSwapChain);
    m_ForwardRenderer.SetShowQuadtreeOverlay(m_ShowQuadtreeOverlay);
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
        ImGui::Text("Quadtree nodes: %u", Stats.TerrainQuadtreeNodeCount);
        ImGui::Text("Selected leaves: %u", Stats.TerrainQuadtreeSelectedLeafCount);
        ImGui::Text("Max quadtree depth: %u", Stats.TerrainQuadtreeMaxDepth);
        ImGui::Text("Max selected LOD: %u", Stats.TerrainQuadtreeMaxSelectedLevel);
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
