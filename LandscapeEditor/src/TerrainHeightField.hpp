#pragma once

#include "BasicMath.hpp"
#include "BasicTypes.h"

#include <algorithm>
#include <string>
#include <vector>

namespace Diligent
{

struct TerrainHeightFieldDesc final
{
    Uint32 CellCount   = 64;
    Uint32 CellCountX  = 0;
    Uint32 CellCountZ  = 0;
    float  Extent      = 20.0f;
    float  HeightScale = 2.5f;
};

struct TerrainHeightSampleStats final
{
    float MinHeight     = 0.0f;
    float MaxHeight     = 0.0f;
    float AverageHeight = 0.0f;
};

enum class TerrainHeightFieldSource
{
    Procedural,
    RawR16,
    RawR16Tiles
};

class TerrainHeightField final
{
public:
    void GenerateProcedural(const TerrainHeightFieldDesc& Desc);
    bool LoadRawR16(const TerrainHeightFieldDesc& Desc,
                    const std::string& Path,
                    Uint32 SampleCountPerAxis,
                    std::string* ErrorMessage = nullptr);
    bool LoadRawR16Tiles(const TerrainHeightFieldDesc& Desc,
                         const std::string& Pattern,
                         Uint32 TileCountX,
                         Uint32 TileCountZ,
                         Uint32 TileSampleCountPerAxis,
                         std::string* ErrorMessage = nullptr);

    Uint32 GetCellCount() const { return std::max(m_Desc.CellCountX, m_Desc.CellCountZ); }
    Uint32 GetCellCountX() const { return m_Desc.CellCountX; }
    Uint32 GetCellCountZ() const { return m_Desc.CellCountZ; }
    Uint32 GetSampleCountPerAxis() const { return std::max(m_SampleCountX, m_SampleCountZ); }
    Uint32 GetSampleCountX() const { return m_SampleCountX; }
    Uint32 GetSampleCountZ() const { return m_SampleCountZ; }
    float  GetCellSizeX() const { return m_CellSizeX; }
    float  GetCellSizeZ() const { return m_CellSizeZ; }
    float  GetExtent() const { return m_Desc.Extent; }
    float  GetHeightScale() const { return m_Desc.HeightScale; }
    float  GetHeight(Uint32 X, Uint32 Z) const { return m_Heights[GetIndex(X, Z)]; }
    float3 GetNormal(Uint32 X, Uint32 Z) const { return m_Normals[GetIndex(X, Z)]; }
    float2 GetUV(Uint32 X, Uint32 Z) const;

    const TerrainHeightSampleStats& GetStats() const { return m_Stats; }
    TerrainHeightFieldSource GetSource() const { return m_Source; }
    const char* GetSourceName() const;
    bool IsHeightmapLoaded() const { return m_Source == TerrainHeightFieldSource::RawR16 || m_Source == TerrainHeightFieldSource::RawR16Tiles; }

private:
    void   InitializeGrid(const TerrainHeightFieldDesc& Desc);
    void   UpdateStats();
    void   RebuildNormals();
    Uint32 GetIndex(Uint32 X, Uint32 Z) const { return Z * m_SampleCountX + X; }
    float  GetClampedHeight(Int32 X, Int32 Z) const;

private:
    TerrainHeightFieldDesc        m_Desc;
    TerrainHeightSampleStats      m_Stats;
    std::vector<float>            m_Heights;
    std::vector<float3>           m_Normals;
    Uint32                        m_SampleCountX = 0;
    Uint32                        m_SampleCountZ = 0;
    float                         m_CellSizeX    = 0.0f;
    float                         m_CellSizeZ    = 0.0f;
    TerrainHeightFieldSource      m_Source             = TerrainHeightFieldSource::Procedural;
};

} // namespace Diligent
