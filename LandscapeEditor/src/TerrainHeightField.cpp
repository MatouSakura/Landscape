#include "TerrainHeightField.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>

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

void Replace(std::string& Text, const std::string& Token, const std::string& Value)
{
    size_t Pos = 0;
    while ((Pos = Text.find(Token, Pos)) != std::string::npos)
    {
        Text.replace(Pos, Token.length(), Value);
        Pos += Value.length();
    }
}

std::string ExpandHeightmapTilePathPattern(const std::string& Pattern, Uint32 TileX, Uint32 TileZ)
{
    std::string Path = Pattern;
    Replace(Path, "{x}", std::to_string(TileX));
    Replace(Path, "{z}", std::to_string(TileZ));
    return Path;
}

bool ReadRawR16Samples(const std::string& Path,
                       Uint32 SampleCountPerAxis,
                       std::vector<Uint16>& RawSamples,
                       std::string* ErrorMessage)
{
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

    RawSamples.resize(static_cast<size_t>(SampleCount));
    HeightFile.seekg(0, std::ios::beg);
    HeightFile.read(reinterpret_cast<char*>(RawSamples.data()), static_cast<std::streamsize>(ExpectedByteCount));
    if (!HeightFile)
    {
        if (ErrorMessage != nullptr)
            *ErrorMessage = "failed to read RawR16 heightmap samples";
        return false;
    }

    return true;
}

} // namespace

void TerrainHeightField::GenerateProcedural(const TerrainHeightFieldDesc& Desc)
{
    InitializeGrid(Desc);

    for (Uint32 Z = 0; Z < m_SampleCountZ; ++Z)
    {
        const float WorldZ = -m_Desc.Extent + m_CellSizeZ * static_cast<float>(Z);
        for (Uint32 X = 0; X < m_SampleCountX; ++X)
        {
            const float WorldX = -m_Desc.Extent + m_CellSizeX * static_cast<float>(X);
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
    RawDesc.CellCountX = RawDesc.CellCount;
    RawDesc.CellCountZ = RawDesc.CellCount;

    std::vector<Uint16> RawSamples;
    if (!ReadRawR16Samples(Path, SampleCountPerAxis, RawSamples, ErrorMessage))
        return false;

    InitializeGrid(RawDesc);
    for (Uint32 Z = 0; Z < m_SampleCountZ; ++Z)
    {
        for (Uint32 X = 0; X < m_SampleCountX; ++X)
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

bool TerrainHeightField::LoadRawR16Tiles(const TerrainHeightFieldDesc& Desc,
                                         const std::string& Pattern,
                                         Uint32 TileCountX,
                                         Uint32 TileCountZ,
                                         Uint32 TileSampleCountPerAxis,
                                         std::string* ErrorMessage)
{
    if (ErrorMessage != nullptr)
        ErrorMessage->clear();

    if (Pattern.empty())
    {
        if (ErrorMessage != nullptr)
            *ErrorMessage = "RAW R16 tile path pattern is empty";
        return false;
    }
    if (TileCountX == 0u || TileCountZ == 0u)
    {
        if (ErrorMessage != nullptr)
            *ErrorMessage = "RAW R16 tile counts must be at least 1";
        return false;
    }
    if (TileSampleCountPerAxis < 2u)
    {
        if (ErrorMessage != nullptr)
            *ErrorMessage = "RAW R16 tile sample count must be at least 2";
        return false;
    }
    if ((TileCountX > 1u || TileCountZ > 1u) &&
        (Pattern.find("{x}") == std::string::npos || Pattern.find("{z}") == std::string::npos))
    {
        if (ErrorMessage != nullptr)
            *ErrorMessage = "RAW R16 tile path pattern must contain {x} and {z}";
        return false;
    }

    const Uint32 TileCellCount = TileSampleCountPerAxis - 1u;
    const Uint32 CombinedCellCountX = TileCellCount * TileCountX;
    const Uint32 CombinedCellCountZ = TileCellCount * TileCountZ;
    const Uint32 CombinedSampleCountX = CombinedCellCountX + 1u;
    const Uint32 CombinedSampleCountZ = CombinedCellCountZ + 1u;

    TerrainHeightFieldDesc RawDesc = Desc;
    RawDesc.CellCount = std::max(CombinedCellCountX, CombinedCellCountZ);
    RawDesc.CellCountX = CombinedCellCountX;
    RawDesc.CellCountZ = CombinedCellCountZ;
    InitializeGrid(RawDesc);

    std::vector<Uint16> RawSamples;
    for (Uint32 TileZ = 0; TileZ < TileCountZ; ++TileZ)
    {
        for (Uint32 TileX = 0; TileX < TileCountX; ++TileX)
        {
            const Uint32 FirstSampleX = TileX * TileCellCount;
            const Uint32 FirstSampleZ = TileZ * TileCellCount;
            const std::string TilePath = ExpandHeightmapTilePathPattern(Pattern, TileX, TileZ);

            std::string TileError;
            if (!ReadRawR16Samples(TilePath, TileSampleCountPerAxis, RawSamples, &TileError))
            {
                if (ErrorMessage != nullptr)
                {
                    std::ostringstream Str;
                    Str << "failed to load RawR16 tile (" << TileX << ", " << TileZ << ") '" << TilePath << "': " << TileError;
                    *ErrorMessage = Str.str();
                }
                return false;
            }

            for (Uint32 LocalZ = 0; LocalZ < TileSampleCountPerAxis; ++LocalZ)
            {
                for (Uint32 LocalX = 0; LocalX < TileSampleCountPerAxis; ++LocalX)
                {
                    const Uint32 DestinationX = FirstSampleX + LocalX;
                    const Uint32 DestinationZ = FirstSampleZ + LocalZ;
                    if (DestinationX >= CombinedSampleCountX || DestinationZ >= CombinedSampleCountZ)
                        continue;

                    const Uint16 Sample = RawSamples[LocalZ * TileSampleCountPerAxis + LocalX];
                    const float NormalizedHeight = static_cast<float>(Sample) / 65535.0f;
                    m_Heights[GetIndex(DestinationX, DestinationZ)] = (NormalizedHeight * 2.0f - 1.0f) * m_Desc.HeightScale;
                }
            }
        }
    }

    UpdateStats();
    RebuildNormals();
    m_Source = TerrainHeightFieldSource::RawR16Tiles;
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
        case TerrainHeightFieldSource::RawR16Tiles:
            return "raw_r16_tiles";
    }

    return "unknown";
}

void TerrainHeightField::InitializeGrid(const TerrainHeightFieldDesc& Desc)
{
    m_Desc             = Desc;
    m_Desc.CellCount  = std::max<Uint32>(m_Desc.CellCount, 1u);
    m_Desc.CellCountX = m_Desc.CellCountX > 0u ? m_Desc.CellCountX : m_Desc.CellCount;
    m_Desc.CellCountZ = m_Desc.CellCountZ > 0u ? m_Desc.CellCountZ : m_Desc.CellCount;
    m_Desc.CellCountX = std::max<Uint32>(m_Desc.CellCountX, 1u);
    m_Desc.CellCountZ = std::max<Uint32>(m_Desc.CellCountZ, 1u);
    m_Desc.CellCount  = std::max(m_Desc.CellCount, std::max(m_Desc.CellCountX, m_Desc.CellCountZ));
    m_Desc.Extent     = std::max(m_Desc.Extent, 1.0f);
    m_Desc.HeightScale = std::max(m_Desc.HeightScale, 0.001f);
    m_SampleCountX = m_Desc.CellCountX + 1u;
    m_SampleCountZ = m_Desc.CellCountZ + 1u;
    m_CellSizeX    = (2.0f * m_Desc.Extent) / static_cast<float>(m_Desc.CellCountX);
    m_CellSizeZ    = (2.0f * m_Desc.Extent) / static_cast<float>(m_Desc.CellCountZ);

    const Uint32 SampleCount = m_SampleCountX * m_SampleCountZ;
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
    for (Uint32 Z = 0; Z < m_SampleCountZ; ++Z)
    {
        for (Uint32 X = 0; X < m_SampleCountX; ++X)
        {
            const float HeightL = GetClampedHeight(static_cast<Int32>(X) - 1, static_cast<Int32>(Z));
            const float HeightR = GetClampedHeight(static_cast<Int32>(X) + 1, static_cast<Int32>(Z));
            const float HeightD = GetClampedHeight(static_cast<Int32>(X), static_cast<Int32>(Z) - 1);
            const float HeightU = GetClampedHeight(static_cast<Int32>(X), static_cast<Int32>(Z) + 1);

            m_Normals[GetIndex(X, Z)] = normalize(float3{
                (HeightL - HeightR) * m_CellSizeZ,
                2.0f * m_CellSizeX * m_CellSizeZ,
                (HeightD - HeightU) * m_CellSizeX});
        }
    }
}

float2 TerrainHeightField::GetUV(Uint32 X, Uint32 Z) const
{
    const float InvMaxCoordX = m_SampleCountX > 1 ? 1.0f / static_cast<float>(m_SampleCountX - 1u) : 0.0f;
    const float InvMaxCoordZ = m_SampleCountZ > 1 ? 1.0f / static_cast<float>(m_SampleCountZ - 1u) : 0.0f;
    return float2{static_cast<float>(X) * InvMaxCoordX, static_cast<float>(Z) * InvMaxCoordZ};
}

float TerrainHeightField::GetClampedHeight(Int32 X, Int32 Z) const
{
    const Int32 MaxCoordX = static_cast<Int32>(m_SampleCountX) - 1;
    const Int32 MaxCoordZ = static_cast<Int32>(m_SampleCountZ) - 1;
    const Uint32 ClampedX = static_cast<Uint32>(std::clamp(X, 0, MaxCoordX));
    const Uint32 ClampedZ = static_cast<Uint32>(std::clamp(Z, 0, MaxCoordZ));
    return m_Heights[GetIndex(ClampedX, ClampedZ)];
}

} // namespace Diligent
