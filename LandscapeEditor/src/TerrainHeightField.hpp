#pragma once

#include "BasicMath.hpp"
#include "BasicTypes.h"

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

class TerrainHeightField final
{
public:
    void GenerateProcedural(const TerrainHeightFieldDesc& Desc);

    Uint32 GetCellCount() const { return m_Desc.CellCount; }
    Uint32 GetSampleCountPerAxis() const { return m_SampleCountPerAxis; }
    float  GetHeight(Uint32 X, Uint32 Z) const { return m_Heights[GetIndex(X, Z)]; }
    float3 GetNormal(Uint32 X, Uint32 Z) const { return m_Normals[GetIndex(X, Z)]; }
    float2 GetUV(Uint32 X, Uint32 Z) const;

    const TerrainHeightSampleStats& GetStats() const { return m_Stats; }

private:
    Uint32 GetIndex(Uint32 X, Uint32 Z) const { return Z * m_SampleCountPerAxis + X; }
    float  GetClampedHeight(Int32 X, Int32 Z) const;

private:
    TerrainHeightFieldDesc        m_Desc;
    TerrainHeightSampleStats      m_Stats;
    std::vector<float>            m_Heights;
    std::vector<float3>           m_Normals;
    Uint32                        m_SampleCountPerAxis = 0;
    float                         m_CellSize           = 0.0f;
};

} // namespace Diligent
