#include "LandscapeEditor.hpp"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new LandscapeEditor();
}

void LandscapeEditor::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    m_ForwardDebugPipeline.Initialize(m_pDevice, m_pSwapChain);
}

void LandscapeEditor::Render()
{
    const float ClearColor[] = {0.08f, 0.09f, 0.10f, 1.0f};

    ITextureView* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    ITextureView* pDSV = m_pSwapChain->GetDepthBufferDSV();
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    m_ForwardDebugPipeline.Render(m_pImmediateContext);
}

void LandscapeEditor::Update(double CurrTime, double ElapsedTime, bool DoUpdateUI)
{
    SampleBase::Update(CurrTime, ElapsedTime, DoUpdateUI);
}

} // namespace Diligent
