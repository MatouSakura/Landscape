#include "TerrainHeightField.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
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
    InitializeGrid(Desc);

    for (Uint32 Z = 0; Z < m_SampleCountPerAxis; ++Z)
    {
        const float WorldZ = -m_Desc.Extent + m_CellSize * static_cast<float>(Z);
        for (Uint32 X = 0; X < m_SampleCountPerAxis; ++X)
        {
            const float WorldX = -m_Desc.Extent + m_CellSize * static_cast<float>(X);
            const float Height = EvaluateProceduralHeight(WorldX, WorldZ, m_Desc.HeightScale);

            m_Heights[GetIndex(X, Z)] = Height;
        }
    }

    UpdateStats();
    RebuildNormals();
    m_Source = TerrainHeightFieldSource::Procedural;
}

bool TerrainHeightField::LoadRawR16(const TerrainHeightFieldDesc& Desc,
                                    const std::string& Path,
                                    Uint32 SampleCountPerAxis,
                                    std::string* ErrorMessage)
{
    if (ErrorMessage != nullptr)
        ErrorMessage->clear();

    if (SampleCountPerAxis < 2u)
    {
        if (ErrorMessage != nullptr)
            *ErrorMessage = "RAW R16 heightmap sample count must be at least 2";
        return false;
    }

    TerrainHeightFieldDesc RawDesc = Desc;
    RawDesc.CellCount = SampleCountPerAxis - 1u;

    const Uint64 SampleCount = static_cast<Uint64>(SampleCountPerAxis) * static_cast<Uint64>(SampleCountPerAxis);
    const Uint64 ExpectedByteCount = SampleCount * sizeof(Uint16);

    std::ifstream HeightFile{Path, std::ios::binary | std::ios::ate};
    if (!HeightFile)
    {
        if (ErrorMessage != nullptr)
            *ErrorMessage = "failed to open RawR16 heightmap file";
        return false;
    }

    const std::streamoff FileSize = HeightFile.tellg();
    if (FileSize != static_cast<std::streamoff>(ExpectedByteCount))
    {
        if (ErrorMessage != nullptr)
            *ErrorMessage = "RawR16 heightmap byte size does not match sample count";
        return false;
    }

    std::vector<Uint16> RawSamples(static_cast<size_t>(SampleCount));
    HeightFile.seekg(0, std::ios::beg);
    HeightFile.read(reinterpret_cast<char*>(RawSamples.data()), static_cast<std::streamsize>(ExpectedByteCount));
    if (!HeightFile)
    {
        if (ErrorMessage != nullptr)
            *ErrorMessage = "failed to read RawR16 heightmap samples";
        return false;
    }

    InitializeGrid(RawDesc);
    for (Uint32 Z = 0; Z < m_SampleCountPerAxis; ++Z)
    {
        for (Uint32 X = 0; X < m_SampleCountPerAxis; ++X)
        {
            const Uint16 Sample = RawSamples[GetIndex(X, Z)];
            const float NormalizedHeight = static_cast<float>(Sample) / 65535.0f;
            m_Heights[GetIndex(X, Z)] = (NormalizedHeight * 2.0f - 1.0f) * m_Desc.HeightScale;
        }
    }

    UpdateStats();
    RebuildNormals();
    m_Source = TerrainHeightFieldSource::RawR16;
    return true;
}

const char* TerrainHeightField::GetSourceName() const
{
    switch (m_Source)
    {
        case TerrainHeightFieldSource::Procedural:
            return "procedural";
        case TerrainHeightFieldSource::RawR16:
            return "raw_r16";
    }

    return "unknown";
}

void TerrainHeightField::InitializeGrid(const TerrainHeightFieldDesc& Desc)
{
    m_Desc             = Desc;
    m_Desc.CellCount  = std::max<Uint32>(m_Desc.CellCount, 1u);
    m_Desc.Extent     = std::max(m_Desc.Extent, 1.0f);
    m_Desc.HeightScale = std::max(m_Desc.HeightScale, 0.001f);
    m_SampleCountPerAxis = m_Desc.CellCount + 1u;
    m_CellSize           = (2.0f * m_Desc.Extent) / static_cast<float>(m_Desc.CellCount);

    const Uint32 SampleCount = m_SampleCountPerAxis * m_SampleCountPerAxis;
    m_Heights.assign(SampleCount, 0.0f);
    m_Normals.assign(SampleCount, float3{0.0f, 1.0f, 0.0f});
}

void TerrainHeightField::UpdateStats()
{
    if (m_Heights.empty())
    {
        m_Stats = {};
        return;
    }

    m_Stats.MinHeight     = std::numeric_limits<float>::max();
    m_Stats.MaxHeight     = std::numeric_limits<float>::lowest();
    m_Stats.AverageHeight = 0.0f;

    double HeightSum = 0.0;
    for (float Height : m_Heights)
    {
        m_Stats.MinHeight = std::min(m_Stats.MinHeight, Height);
        m_Stats.MaxHeight = std::max(m_Stats.MaxHeight, Height);
        HeightSum += Height;
    }

    m_Stats.AverageHeight = static_cast<float>(HeightSum / static_cast<double>(m_Heights.size()));
}

void TerrainHeightField::RebuildNormals()
{
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
