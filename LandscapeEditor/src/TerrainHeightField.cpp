#include "TerrainHeightField.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Diligent
{

namespace
{

float EvaluateProceduralHeight(float WorldX, float WorldZ, float HeightScale)
{
    const float WaveA = 0.45f * std::sin(WorldX * 0.22f) * std::cos(WorldZ * 0.18f);
    const float WaveB = 0.25f * std::sin((WorldX + WorldZ) * 0.14f);
    const float RadialDistance = std::sqrt(WorldX * WorldX + WorldZ * WorldZ);
    const float Radial = 0.10f * std::cos(RadialDistance * 0.20f);
    const float Height = WaveA + WaveB + Radial;
    return HeightScale * Height;
}

} // namespace

void TerrainHeightField::GenerateProcedural(const TerrainHeightFieldDesc& Desc)
{
    m_Desc             = Desc;
    m_Desc.CellCount  = std::max<Uint32>(m_Desc.CellCount, 1u);
    m_Desc.Extent     = std::max(m_Desc.Extent, 1.0f);
    m_SampleCountPerAxis = m_Desc.CellCount + 1u;
    m_CellSize           = (2.0f * m_Desc.Extent) / static_cast<float>(m_Desc.CellCount);

    const Uint32 SampleCount = m_SampleCountPerAxis * m_SampleCountPerAxis;
    m_Heights.assign(SampleCount, 0.0f);
    m_Normals.assign(SampleCount, float3{0.0f, 1.0f, 0.0f});

    m_Stats.MinHeight     = std::numeric_limits<float>::max();
    m_Stats.MaxHeight     = std::numeric_limits<float>::lowest();
    m_Stats.AverageHeight = 0.0f;

    double HeightSum = 0.0;
    for (Uint32 Z = 0; Z < m_SampleCountPerAxis; ++Z)
    {
        const float WorldZ = -m_Desc.Extent + m_CellSize * static_cast<float>(Z);
        for (Uint32 X = 0; X < m_SampleCountPerAxis; ++X)
        {
            const float WorldX = -m_Desc.Extent + m_CellSize * static_cast<float>(X);
            const float Height = EvaluateProceduralHeight(WorldX, WorldZ, m_Desc.HeightScale);

            m_Heights[GetIndex(X, Z)] = Height;
            m_Stats.MinHeight         = std::min(m_Stats.MinHeight, Height);
            m_Stats.MaxHeight         = std::max(m_Stats.MaxHeight, Height);
            HeightSum += Height;
        }
    }

    m_Stats.AverageHeight = static_cast<float>(HeightSum / static_cast<double>(SampleCount));

    for (Uint32 Z = 0; Z < m_SampleCountPerAxis; ++Z)
    {
        for (Uint32 X = 0; X < m_SampleCountPerAxis; ++X)
        {
            const float HeightL = GetClampedHeight(static_cast<Int32>(X) - 1, static_cast<Int32>(Z));
            const float HeightR = GetClampedHeight(static_cast<Int32>(X) + 1, static_cast<Int32>(Z));
            const float HeightD = GetClampedHeight(static_cast<Int32>(X), static_cast<Int32>(Z) - 1);
            const float HeightU = GetClampedHeight(static_cast<Int32>(X), static_cast<Int32>(Z) + 1);

            m_Normals[GetIndex(X, Z)] = normalize(float3{HeightL - HeightR, 2.0f * m_CellSize, HeightD - HeightU});
        }
    }
}

float2 TerrainHeightField::GetUV(Uint32 X, Uint32 Z) const
{
    const float InvMaxCoord = m_SampleCountPerAxis > 1 ? 1.0f / static_cast<float>(m_SampleCountPerAxis - 1u) : 0.0f;
    return float2{static_cast<float>(X) * InvMaxCoord, static_cast<float>(Z) * InvMaxCoord};
}

float TerrainHeightField::GetClampedHeight(Int32 X, Int32 Z) const
{
    const Int32 MaxCoord = static_cast<Int32>(m_SampleCountPerAxis) - 1;
    const Uint32 ClampedX = static_cast<Uint32>(std::clamp(X, 0, MaxCoord));
    const Uint32 ClampedZ = static_cast<Uint32>(std::clamp(Z, 0, MaxCoord));
    return m_Heights[GetIndex(ClampedX, ClampedZ)];
}

} // namespace Diligent
