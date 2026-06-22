#pragma once

#include "ForwardDebugPipeline.hpp"
#include "SampleBase.hpp"

namespace Diligent
{

class LandscapeEditor final : public SampleBase
{
public:
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime, bool DoUpdateUI) override final;

    virtual const Char* GetSampleName() const override final { return "LandscapeEditor"; }

private:
    ForwardDebugPipeline m_ForwardDebugPipeline;
};

} // namespace Diligent
