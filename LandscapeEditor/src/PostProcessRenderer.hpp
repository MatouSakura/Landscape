#pragma once

#include "GraphicsTypes.h"
#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

struct IDeviceContext;
struct IRenderDevice;
struct IShaderResourceBinding;
struct ISwapChain;
struct ITexture;
struct ITextureView;
class PSOCache;

class PostProcessRenderer final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache);

    ITextureView* PrepareSceneTarget(IDeviceContext* pContext, ISwapChain* pSwapChain, const float ClearColor[4]);
    void Render(IDeviceContext* pContext, ISwapChain* pSwapChain);

    Uint32 GetPassCount() const { return m_PassCount; }

private:
    void EnsureSceneTarget(ISwapChain* pSwapChain);

private:
    IRenderDevice*                         m_pDevice = nullptr;
    RefCntAutoPtr<IPipelineState>          m_pPostProcessPSO;
    RefCntAutoPtr<IShaderResourceBinding>  m_pPostProcessSRB;
    RefCntAutoPtr<ITexture>                m_pSceneColor;
    RefCntAutoPtr<ITextureView>            m_pSceneColorRTV;
    RefCntAutoPtr<ITextureView>            m_pSceneColorSRV;
    Uint32                                 m_Width       = 0;
    Uint32                                 m_Height      = 0;
    TEXTURE_FORMAT                         m_ColorFormat = TEX_FORMAT_UNKNOWN;
    Uint32                                 m_PassCount   = 0;
};

} // namespace Diligent
