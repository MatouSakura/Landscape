#pragma once

#include "BasicMath.hpp"
#include "BasicTypes.h"

#include <string>
#include <vector>

namespace Diligent
{

struct TerrainHeightFieldDesc final
{
    Uint32 CellCount   = 64;
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
    RawR16
};

class TerrainHeightField final
{
public:
    void GenerateProcedural(const TerrainHeightFieldDesc& Desc);
    bool LoadRawR16(const TerrainHeightFieldDesc& Desc,
                    const std::string& Path,
                    Uint32 SampleCountPerAxis,
                    std::string* ErrorMessage = nullptr);

    Uint32 GetCellCount() const { return m_Desc.CellCount; }
    Uint32 GetSampleCountPerAxis() const { return m_SampleCountPerAxis; }
    float  GetExtent() const { return m_Desc.Extent; }
    float  GetHeightScale() const { return m_Desc.HeightScale; }
    float  GetHeight(Uint32 X, Uint32 Z) const { return m_Heights[GetIndex(X, Z)]; }
    float3 GetNormal(Uint32 X, Uint32 Z) const { return m_Normals[GetIndex(X, Z)]; }
    float2 GetUV(Uint32 X, Uint32 Z) const;

    const TerrainHeightSampleStats& GetStats() const { return m_Stats; }
    TerrainHeightFieldSource GetSource() const { return m_Source; }
    const char* GetSourceName() const;
    bool IsHeightmapLoaded() const { return m_Source == TerrainHeightFieldSource::RawR16; }

private:
    void   InitializeGrid(const TerrainHeightFieldDesc& Desc);
    void   UpdateStats();
    void   RebuildNormals();
    Uint32 GetIndex(Uint32 X, Uint32 Z) const { return Z * m_SampleCountPerAxis + X; }
    float  GetClampedHeight(Int32 X, Int32 Z) const;

private:
    TerrainHeightFieldDesc        m_Desc;
    TerrainHeightSampleStats      m_Stats;
    std::vector<float>            m_Heights;
    std::vector<float3>           m_Normals;
    Uint32                        m_SampleCountPerAxis = 0;
    float                         m_CellSize           = 0.0f;
    TerrainHeightFieldSource      m_Source             = TerrainHeightFieldSource::Procedural;
};

} // namespace Diligent
