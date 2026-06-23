#pragma once

#include "FirstPersonCamera.hpp"
#include "FrameResources.hpp"
#include "ForwardRenderer.hpp"
#include "RenderView.hpp"
#include "SampleBase.hpp"

namespace Diligent
{

class LandscapeEditor final : public SampleBase
{
public:
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime, bool DoUpdateUI) override final;
    virtual void WindowResize(Uint32 Width, Uint32 Height) override final;

    virtual const Char* GetSampleName() const override final { return "LandscapeEditor"; }

private:
    void UpdateRenderView();

private:
    FirstPersonCamera    m_Camera;
    RenderView           m_RenderView;
    FrameResources       m_FrameResources;
    ForwardRenderer      m_ForwardRenderer;
    bool                 m_ShowQuadtreeOverlay = true;
    bool                 m_EnableTerrainSkirts = true;
};

} // namespace Diligent
