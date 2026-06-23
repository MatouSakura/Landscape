#pragma once

#include "Buffer.h"
#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"
#include "RenderQueue.hpp"
#include "TerrainHeightField.hpp"
#include "TerrainHeightmapTileSet.hpp"
#include "TerrainLODIndexStitching.hpp"
#include "TerrainQuadtree.hpp"

#include <string>
#include <vector>

namespace Diligent
{

struct IDeviceContext;
struct IRenderDevice;
struct IShaderResourceBinding;
struct ISwapChain;
struct ITextureView;
class FrameResources;
class PSOCache;
struct RenderView;

struct TerrainTileMeshRange final
{
    TerrainDrawRegion Region;
    Uint32            VertexCount = 0;
};

struct TerrainPatchRendererDesc final
{
    TerrainHeightFieldDesc HeightField;
    std::string            HeightmapRawR16Path;
    Uint32                 HeightmapSampleCountPerAxis = 65;
    Uint32                 HeightmapTileCountX = 1;
    Uint32                 HeightmapTileCountZ = 1;
};

class TerrainPatchRenderer final
{
public:
    void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache, const std::vector<TerrainQuadtreeNode>& QuadtreeNodes, const TerrainPatchRendererDesc& Desc = {});
    TerrainDrawRegion BuildDrawRegion(const TerrainQuadtreeNode& Node) const;
    void BeginFrameStats();
    void SetEnableSkirts(bool Enable) { m_EnableSkirts = Enable; }
    bool GetEnableSkirts() const { return m_EnableSkirts; }
    float GetSkirtDepth() const { return m_SkirtDepth; }
    void PrepareLODIndexStitching(IDeviceContext* pContext, TerrainLODIndexStitching& Stitching);
    void Render(IDeviceContext* pContext, const RenderView& View, FrameResources& FrameResources, const TerrainDrawRegion& Region, ITextureView* pShadowMapSRV);
    void RenderShadow(IDeviceContext* pContext, IBuffer* pShadowConstants, const TerrainDrawRegion& Region);

    Uint32 GetTriangleCount() const { return m_IndexCount / 3u; }
    Uint32 GetLastRenderedTriangleCount() const { return m_LastRenderedIndexCount / 3u; }
    Uint32 GetLastRenderedCellCount() const { return m_LastRenderedCellCount; }
    Uint32 GetLastRenderedMeshCellCount() const { return m_LastRenderedMeshCellCount; }
    Uint32 GetLastForwardDrawCallCount() const { return m_LastForwardDrawCallCount; }
    Uint32 GetLastShadowDrawCallCount() const { return m_LastShadowDrawCallCount; }
    Uint32 GetTileMeshCount() const { return static_cast<Uint32>(m_TileMeshRanges.size()); }
    Uint32 GetPackedTileVertexCount() const { return m_PackedTileVertexCount; }
    Uint32 GetPackedTileIndexCount() const { return m_PackedTileIndexCount; }
    Uint32 GetPackedTileSkirtVertexCount() const { return m_PackedTileSkirtVertexCount; }
    Uint32 GetPackedTileSkirtIndexCount() const { return m_PackedTileSkirtIndexCount; }
    Uint32 GetMinLODSampleStep() const { return m_MinLODSampleStep; }
    Uint32 GetMaxLODSampleStep() const { return m_MaxLODSampleStep; }
    Uint32 GetCellCount() const { return m_HeightField.GetCellCount(); }
    Uint32 GetSampleCountPerAxis() const { return m_HeightField.GetSampleCountPerAxis(); }
    float  GetMinHeight() const { return m_HeightField.GetStats().MinHeight; }
    float  GetMaxHeight() const { return m_HeightField.GetStats().MaxHeight; }
    float  GetAverageHeight() const { return m_HeightField.GetStats().AverageHeight; }
    const char* GetHeightSourceName() const { return m_HeightField.GetSourceName(); }
    bool IsHeightmapLoaded() const { return m_HeightField.IsHeightmapLoaded(); }
    const char* GetHeightmapLayoutName() const { return m_HeightmapTileSet.GetLayoutName(); }
    Uint32 GetHeightmapTileCountX() const { return m_HeightmapTileSet.GetStats().TileCountX; }
    Uint32 GetHeightmapTileCountZ() const { return m_HeightmapTileSet.GetStats().TileCountZ; }
    Uint32 GetHeightmapTileCount() const { return m_HeightmapTileSet.GetStats().TileCount; }
    Uint32 GetHeightmapTileSampleCountPerAxis() const { return m_HeightmapTileSet.GetStats().TileSampleCountPerAxis; }
    Uint32 GetHeightmapTileCellCount() const { return m_HeightmapTileSet.GetStats().TileCellCount; }
    Uint32 GetHeightmapPackageCellCountX() const { return m_HeightmapTileSet.GetStats().TotalCellCountX; }
    Uint32 GetHeightmapPackageCellCountZ() const { return m_HeightmapTileSet.GetStats().TotalCellCountZ; }
    float  GetHeightmapTileWorldSizeX() const { return m_HeightmapTileSet.GetStats().TileWorldSizeX; }
    float  GetHeightmapTileWorldSizeZ() const { return m_HeightmapTileSet.GetStats().TileWorldSizeZ; }

private:
    void BuildPackedTileMeshCache(IRenderDevice* pDevice, const std::vector<TerrainQuadtreeNode>& QuadtreeNodes);
    void DrawTileMeshIndexed(IDeviceContext* pContext, const TerrainDrawRegion& Region) const;

private:
    TerrainHeightField                    m_HeightField;
    TerrainHeightmapTileSet               m_HeightmapTileSet;
    TerrainPatchRendererDesc              m_Desc;
    std::vector<TerrainTileMeshRange>      m_TileMeshRanges;
    RefCntAutoPtr<IBuffer>                m_pVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_pIndexBuffer;
    RefCntAutoPtr<IBuffer>                m_pStitchedIndexBuffer;
    RefCntAutoPtr<IPipelineState>         m_pTerrainPSO;
    RefCntAutoPtr<IPipelineState>         m_pShadowPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pTerrainSRB;
    RefCntAutoPtr<IShaderResourceBinding> m_pShadowSRB;
    Uint32                                m_IndexCount = 0;
    Uint32                                m_LastRenderedCellCount = 0;
    Uint32                                m_LastRenderedMeshCellCount = 0;
    Uint32                                m_LastRenderedIndexCount = 0;
    Uint32                                m_LastForwardDrawCallCount = 0;
    Uint32                                m_LastShadowDrawCallCount = 0;
    Uint32                                m_PackedTileVertexCount = 0;
    Uint32                                m_PackedTileIndexCount = 0;
    Uint32                                m_PackedTileSkirtVertexCount = 0;
    Uint32                                m_PackedTileSkirtIndexCount = 0;
    Uint32                                m_StitchedIndexBufferCapacity = 0;
    Uint32                                m_MinLODSampleStep = 1;
    Uint32                                m_MaxLODSampleStep = 1;
    float                                 m_TerrainExtent = 20.0f;
    float                                 m_SkirtDepth = 1.25f;
    bool                                  m_EnableSkirts = true;
};

} // namespace Diligent
