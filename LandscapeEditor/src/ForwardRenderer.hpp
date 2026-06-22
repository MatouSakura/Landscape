#pragma once

#include "ForwardDebugPipeline.hpp"
#include "PSOCache.hpp"
#include "RenderQueue.hpp"
#include "TerrainPatchRenderer.hpp"

namespace Diligent
{

struct IDeviceContext;
struct IRenderDevice;
struct ISwapChain;
class FrameResources;
struct RenderView;

struct ForwardRendererStats final
{
    Uint32 OpaqueItemCount    = 0;
    Uint32 TerrainTriangleCount = 0;
    Uint32 DebugItemCount     = 0;
    size_t PSOCount           = 0;
    size_t PSOCreationCount = 0;
};

class ForwardRenderer final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain);
    void Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources);

    const ForwardRendererStats& GetStats() const { return m_Stats; }

private:
    ForwardDebugPipeline  m_ForwardDebugPipeline;
    TerrainPatchRenderer  m_TerrainPatchRenderer;
    RenderQueue           m_RenderQueue;
    PSOCache              m_PSOCache;
    ForwardRendererStats  m_Stats;
};

} // namespace Diligent
